/*
 * Emulate a QSPI flash device following the mt25q command set.
 * Modelled after the m25p80 emulation found in hw/block/m25p80.c
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/hw.h"
#include "sysemu/block-backend.h"
#include "sysemu/blockdev.h"
#include "hw/qdev-properties.h"
#include "hw/ssi/ssi.h"
#include "migration/vmstate.h"


// TODO: These should be made configurable to support different flash parts
#define FLASH_SECTOR_SIZE (64 * 1024)
#define FLASH_NUM_SECTORS (256)
#define FLASH_PAGE_SIZE (256)
const uint8_t MT25Q_ID[] = { 0x20, 0xbb, 0x19 };

#ifndef MT25Q_ERR_DEBUG
#define MT25Q_ERR_DEBUG 0
#endif

// The usleep() helps MacOS stdout from freezing when printing a lot
#define DB_PRINT_L(level, ...) do { \
    if (MT25Q_ERR_DEBUG > (level)) { \
        fprintf(stderr,  "%d: %s: ", level, __func__); \
        fprintf(stderr, ## __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        usleep(1000); \
    } \
} while (0);


typedef enum {
    READ_EVCR = 0x65,
    WRITE_EVCR = 0x61,

    RESET_ENABLE = 0x66,
    RESET = 0x99,

    WRITE_ENABLE = 0x06,
    WRITE_DISABLE = 0x04,

    READ_STATUS_REG = 0x05,
    READ_FLAG_STATUS_REG = 0x70,

    FAST_READ = 0x0b,
    FAST_READ_DDR = 0x0d,
    READ_QID = 0xaf,

    PAGE_PROGRAM = 0x02,

    ERASE_SUBSECTOR = 0x20, // Erase 4k sector
    ERASE_BLOCK = 0xd8, // Erase 64k block

    ERASE_SUSPEND = 0x75,
    ERASE_RESUME = 0x7a,

    DEEP_SLEEP = 0xb9,
    WAKE = 0xab,

    QUAD_ENABLE = 0x35,
} FlashCmd;

typedef enum {
    STATE_IDLE,

    STATE_COLLECT_CMD_DATA,

    STATE_WRITE,
    STATE_READ,
    STATE_READ_QID,
    STATE_READ_REGISTER,
} CMDState;

#define R_STATUS_BUSY (1 << 0)
#define R_STATUS_WRITE_ENABLE (1 << 1)

#define R_FLAG_STATUS_ERASE_SUSPEND (1 << 6)

typedef struct {
    SSISlave parent_obj;

    //--- Storage ---
    BlockBackend *blk;
    uint8_t *storage;
    uint32_t size;
    int page_size;

    int64_t dirty_page;

    //--- Registers ---
    uint8_t EVCR;
    uint8_t STATUS_REG;
    uint8_t FLAG_STATUS_REG;

    //--- Command state ---
    CMDState state;
    FlashCmd cmd_in_progress;
    uint8_t cmd_data[4]; //! Commands can require up to 4 bytes of additional data
    uint8_t cmd_bytes;   //! Number of bytes required by command [0,4]
    uint32_t len;
    uint32_t pos;
    uint64_t current_address;

    uint8_t *current_register;
    uint8_t register_read_mask; //! mask to apply after reading current_register

    bool reset_enabled;
} Flash;

typedef struct {
    SSISlaveClass parent_class;
} MT25QClass;

#define TYPE_MT25Q "mt25q-generic"
#define MT25Q(obj) \
     OBJECT_CHECK(Flash, (obj), TYPE_MT25Q)
#define MT25Q_CLASS(klass) \
     OBJECT_CLASS_CHECK(MT25QClass, (klass), TYPE_MT25Q)
#define MT25Q_GET_CLASS(obj) \
     OBJECT_GET_CLASS(MT25QClass, (obj), TYPE_MT25Q)

static void blk_sync_complete(void *opaque, int ret)
{
    /* do nothing. Masters do not directly interact with the backing store,
     * only the working copy so no mutexing required.
     */
}

static void mt25q_flash_sync_page(Flash *s, int page)
{
    int blk_sector, nb_sectors;
    QEMUIOVector iov;

    if (!s->blk || blk_is_read_only(s->blk)) {
        return;
    }

    blk_sector = (page * s->page_size) / BDRV_SECTOR_SIZE;
    nb_sectors = DIV_ROUND_UP(s->page_size, BDRV_SECTOR_SIZE);
    qemu_iovec_init(&iov, 1);
    qemu_iovec_add(&iov, s->storage + blk_sector * BDRV_SECTOR_SIZE,
                   nb_sectors * BDRV_SECTOR_SIZE);
    blk_aio_pwritev(s->blk, page * s->page_size, &iov, 0,
                    blk_sync_complete, NULL);
}

static inline void mt25q_flash_sync_area(Flash *s, int64_t off, int64_t len)
{
    int64_t start, end, nb_sectors;
    QEMUIOVector iov;

    if (!s->blk || blk_is_read_only(s->blk)) {
        return;
    }

    assert(!(len % BDRV_SECTOR_SIZE));
    start = off / BDRV_SECTOR_SIZE;
    end = (off + len) / BDRV_SECTOR_SIZE;
    nb_sectors = end - start;
    qemu_iovec_init(&iov, 1);
    qemu_iovec_add(&iov, s->storage + (start * BDRV_SECTOR_SIZE),
                                        nb_sectors * BDRV_SECTOR_SIZE);
    blk_aio_pwritev(s->blk, off, &iov, 0, blk_sync_complete, NULL);
}

static inline void flash_sync_dirty(Flash *s, int64_t newpage)
{
    if (s->dirty_page >= 0 && s->dirty_page != newpage) {
        mt25q_flash_sync_page(s, s->dirty_page);
        s->dirty_page = newpage;
    }
}

static void mt25q_flash_erase(Flash *s, uint32_t offset, FlashCmd cmd)
{
  uint32_t len;

  switch (cmd) {
  case ERASE_SUBSECTOR: // Erase 4k sector
    len = 4 << 10;
    break;
  case ERASE_BLOCK: // Erase 64k block
    len = 64 << 10;
    break;
  default:
    abort();
  }

  DB_PRINT_L(0, "erase offset = %#x, len = %d", offset, len);

  if (!(s->STATUS_REG & R_STATUS_WRITE_ENABLE)) {
    DB_PRINT_L(-1, "erase with write protect!\n");
    qemu_log_mask(LOG_GUEST_ERROR, "MT25Q: erase with write protect!\n");
    return;
  }

  memset(s->storage + offset, 0xff, len);
  mt25q_flash_sync_area(s, offset, len);
}

static void mt25q_decode_new_cmd(Flash *s, uint32_t value)
{
    s->cmd_in_progress = value;
    DB_PRINT_L(2, "decoding new command: 0x%x", value);

    switch (value) {
    case RESET_ENABLE:
        // handled below
        break;

    case RESET:
        assert(s->reset_enabled);
        break;

    case WRITE_ENABLE:
        s->STATUS_REG |= R_STATUS_WRITE_ENABLE;
        s->state = STATE_IDLE;
        break;
    case WRITE_DISABLE:
        s->STATUS_REG &= ~R_STATUS_WRITE_ENABLE;
        s->state = STATE_IDLE;
        break;

    case WRITE_EVCR:
        s->pos = 0;
        s->cmd_bytes = 1;
        s->state = STATE_COLLECT_CMD_DATA;
        break;
    case READ_EVCR:
        s->current_register = &s->EVCR;
        s->state = STATE_READ_REGISTER;
        break;
    case READ_STATUS_REG:
        s->current_register = &s->STATUS_REG;
        s->state = STATE_READ_REGISTER;
        break;
    case READ_FLAG_STATUS_REG:
        s->current_register = &s->FLAG_STATUS_REG;
        s->state = STATE_READ_REGISTER;
        break;

    case FAST_READ:
    case FAST_READ_DDR:
        s->cmd_bytes = 3;
        s->pos = 0;
        s->state = STATE_COLLECT_CMD_DATA;
        break;

    case READ_QID:
        s->cmd_bytes = 0;
        s->state = STATE_READ_QID;
        s->len = 3;
        s->pos = 0;
        break;

    case PAGE_PROGRAM:
        s->pos = 0;
        s->cmd_bytes = 3;
        s->state = STATE_COLLECT_CMD_DATA;
        break;

    case ERASE_SUBSECTOR:
    case ERASE_BLOCK:
        s->pos = 0;
        s->cmd_bytes = 3;
        s->state = STATE_COLLECT_CMD_DATA;
        break;

    case ERASE_SUSPEND:
    case ERASE_RESUME:
        break;

    case DEEP_SLEEP:
    case WAKE:
        break;

    case QUAD_ENABLE:
        break;

    default:
        DB_PRINT_L(-1, "Unknown cmd 0x%x\n", value);
        qemu_log_mask(LOG_GUEST_ERROR, "MT25Q: Unknown cmd 0x%x\n", value);
    }

    s->reset_enabled = (value == RESET_ENABLE);
}

static void mt25q_handle_cmd_data(Flash *s)
{
    s->state = STATE_IDLE;

    switch (s->cmd_in_progress) {
    case WRITE_EVCR:
        s->EVCR = s->cmd_data[0];
        s->current_address = 0;
        break;
    case READ_EVCR:
        assert(false);
        break;
    case PAGE_PROGRAM:
        s->current_address = (s->cmd_data[2] << 16) | (s->cmd_data[1] << 8) | (s->cmd_data[0]);
        s->state = STATE_WRITE;
        break;
    case READ_STATUS_REG:
    case READ_FLAG_STATUS_REG:
        assert(false);
        break;
    case FAST_READ:
    case FAST_READ_DDR:
        s->current_address = (s->cmd_data[2] << 16) | (s->cmd_data[1] << 8) | (s->cmd_data[0]);
        DB_PRINT_L(2, "Read From: 0x%"PRIu64, s->current_address);
        s->state = STATE_READ;
        break;
    case ERASE_SUBSECTOR:
    case ERASE_BLOCK:
        s->current_address = (s->cmd_data[2] << 16) | (s->cmd_data[1] << 8) | (s->cmd_data[0]);
        mt25q_flash_erase(s, s->current_address, s->cmd_in_progress);
        s->STATUS_REG |= R_STATUS_BUSY;
        s->register_read_mask = R_STATUS_BUSY;
        break;

    case ERASE_SUSPEND:
    case ERASE_RESUME:
        s->current_address = (s->cmd_data[2] << 16) | (s->cmd_data[1] << 8) | (s->cmd_data[0]);
        break;

    default:
        s->current_address = (s->cmd_data[2] << 16) | (s->cmd_data[1] << 8) | (s->cmd_data[0]);
        DB_PRINT_L(-1, "Unknown cmd data 0x%x\n", s->cmd_in_progress);
        qemu_log_mask(LOG_GUEST_ERROR, "MT25Q: Unknown cmd data 0x%x\n", s->cmd_in_progress);
        break;
    }
}

static void mt25q_write8(Flash *s, uint8_t value)
{
    int64_t page = s->current_address / s->page_size;

    // TODO: Write protection

    uint8_t current = s->storage[s->current_address];
    if (value & ~current) {
        DB_PRINT_L(-1, "Flipping bit from 0 => 1 (addr=0x%llx, value=0x%x, current=0x%x)\n",
                   s->current_address, value, current);
        qemu_log_mask(LOG_GUEST_ERROR, "MT25Q: Flipping bit from 0 => 1\n");
        // if a bit in the flash is already a 0, leave it as a 0
        value &= current;
    }
    DB_PRINT_L(2, "Write 0x%"PRIx8" = 0x%"PRIx64, (uint8_t)value, s->current_address);
    s->storage[s->current_address] = (uint8_t)value;

    flash_sync_dirty(s, page);
    s->dirty_page = page;
}

static uint32_t mt25q_transfer8(SSISlave *ss, uint32_t tx)
{
    Flash *s = MT25Q(ss);
    uint32_t r = 0;

    switch (s->state) {
    case STATE_COLLECT_CMD_DATA:
        DB_PRINT_L(2, "Collected: 0x%"PRIx32, (uint32_t)tx);
        s->cmd_data[s->pos++] = (uint8_t)tx;
        if (s->pos == s->cmd_bytes) {
            mt25q_handle_cmd_data(s);
        }
        break;
    case STATE_WRITE:
        if (s->current_address > s->size) {
          DB_PRINT_L(-1, "MT25Q: Out of bounds flash write to 0x%"PRIx64"\n", s->current_address);
          qemu_log_mask(LOG_GUEST_ERROR,
              "MT25Q: Out of bounds flash write to 0x%"PRIx64"\n", s->current_address);
        } else {
          mt25q_write8(s, tx);
          s->current_address += 1;
        }
        break;
    case STATE_READ:
        if (s->current_address > s->size) {
          DB_PRINT_L(-1, "MT25Q: Out of bounds flash read from 0x%"PRIx64"\n", s->current_address);
          qemu_log_mask(LOG_GUEST_ERROR,
              "MT25Q: Out of bounds flash read from 0x%"PRIx64"\n", s->current_address);
        } else {
          DB_PRINT_L(2, "Read 0x%"PRIx64" = 0x%"PRIx8, s->current_address, (uint8_t)r);
          r = s->storage[s->current_address];
          s->current_address = (s->current_address + 1) % s->size;
        }
        break;
    case STATE_READ_QID:
        r = MT25Q_ID[s->pos];
        DB_PRINT_L(1, "Read QID 0x%x (pos 0x%x)", (uint8_t)r, s->pos);
        ++s->pos;
        if (s->pos == s->len) {
            s->pos = 0;
            s->state = STATE_IDLE;
        }
        break;
    case STATE_READ_REGISTER:
        r = *s->current_register;
        *s->current_register &= ~s->register_read_mask;
        s->register_read_mask = 0;
        s->state = STATE_IDLE;
        DB_PRINT_L(2, "Read register");
        break;
    case STATE_IDLE:
        mt25q_decode_new_cmd(s, tx);
        break;
    }

    return r;
}

static void mt25q_realize(SSISlave *ss, Error **errp)
{
    DriveInfo *dinfo;
    Flash *s = MT25Q(ss);

    s->state = STATE_IDLE;
    s->size = FLASH_SECTOR_SIZE * FLASH_NUM_SECTORS;
    s->page_size = FLASH_PAGE_SIZE;
    s->dirty_page = -1;
    s->STATUS_REG = 0;

    if (s->blk) {
        DB_PRINT_L(0, "Binding to IF_MTD drive");
        s->storage = blk_blockalign(s->blk, s->size);


        if (blk_pread(s->blk, 0, s->storage, s->size) != s->size) {
            error_setg(errp,  "Failed to initialize SPI flash!");
            return;
        }
    } else {
        DB_PRINT_L(-1, "No BDRV - binding to RAM");
        s->storage = blk_blockalign(NULL, s->size);
        memset(s->storage, 0xFF, s->size);
    }
}

static int mt25q_cs(SSISlave *ss, bool select)
{
    Flash *s = MT25Q(ss);

    if (select) {
        s->len = 0;
        s->pos = 0;
        s->state = STATE_IDLE;
        flash_sync_dirty(s, -1);
    }

    DB_PRINT_L(2, "CS %s", select ? "HIGH" : "LOW");

    return 0;
}

static int mt25q_pre_save(void *opaque)
{
    flash_sync_dirty((Flash *)opaque, -1);

    return 0;
}

static const VMStateDescription vmstate_mt25q = {
    .name = "mt25q",
    .version_id = 1,
    .minimum_version_id = 1,
    .pre_save = mt25q_pre_save,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static Property mx251_properties[] = {
    DEFINE_PROP_DRIVE("drive", Flash, blk),
    DEFINE_PROP_END_OF_LIST(),
};

static void mt25q_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    SSISlaveClass *c = SSI_SLAVE_CLASS(class);

    c->realize = mt25q_realize;
    c->transfer = mt25q_transfer8;
    c->set_cs = mt25q_cs;
    c->cs_polarity = SSI_CS_LOW;
    dc->vmsd = &vmstate_mt25q;
    device_class_set_props(dc, mx251_properties);
}

static const TypeInfo mt25q_info = {
    .name           = TYPE_MT25Q,
    .parent         = TYPE_SSI_SLAVE,
    .instance_size  = sizeof(Flash),
    .class_size     = sizeof(MT25QClass),
    .abstract       = true,
};

static void mt25q_register_types(void)
{
    type_register_static(&mt25q_info);

    TypeInfo ti = {
        .name = "mt25q256",
        .parent = TYPE_MT25Q,
        .class_init = mt25q_class_init,
    };
    type_register(&ti);
}

type_init(mt25q_register_types)
