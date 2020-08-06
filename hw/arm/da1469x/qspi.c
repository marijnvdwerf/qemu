#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "qspi.h"
#include "qemu/log.h"
#include "hw/ssi/ssi.h"
#include "hw/irq.h"

static const int QSPIC_CTRLBUS_REG = 0x00;
static const int QSPIC_CTRLMODE_REG = 0x04;
static const int QSPIC_BURSTCMDA_REG = 0x0C;
static const int QSPIC_BURSTCMDB_REG = 0x10;
static const int QSPIC_WRITEDATA_REG = 0x18;
static const int QSPIC_READDATA_REG = 0x1C;
static const int QSPIC_ERASECMDA_REG = 0x28;
static const int QSPIC_ERASECMDB_REG = 0x2C;
static const int QSPIC_BURSTBRK_REG = 0x30;
static const int QSPIC_STATUSCMD_REG = 0x34;

static void da1469x_qspi_set_cs(DA1469xQspiState *s, bool low) {
    qemu_set_irq(s->cs_line, !low);
}

static uint64_t da1469x_qspic_read(void *opaque, hwaddr addr, unsigned size) {
    DA1469xQspiState *s = opaque;

    if (addr == QSPIC_READDATA_REG && size == 1) {
        uint32_t ret = ssi_transfer(s->spi, 0);
        printf("[QSPI]   - read -> 0x%X\n", ret);
        return ret;
    }

    switch (addr) {
        case QSPIC_CTRLMODE_REG: {
            int out = 0;
            out |= s->AUTO_MD << 0;
            out |= s->CLK_MD << 1;
            out |= s->IO2_OEN << 2;
            out |= s->IO3_OEN << 3;
            out |= s->IO2_DAT << 4;
            out |= s->IO3_DAT << 5;
            out |= s->HRDY_MD << 6;
            out |= s->RXD_NEG << 7;
            out |= s->RPIPE_EN << 8;
            out |= s->PCLK_MD << 9;

            out |= s->BUF_LIM_EN << 12;
            out |= s->USE_32BA << 13;
            return out;
        }
        case QSPIC_BURSTCMDA_REG:
        case QSPIC_BURSTCMDB_REG:
        case QSPIC_READDATA_REG:
        case QSPIC_ERASECMDB_REG:
        case QSPIC_BURSTBRK_REG:
        case QSPIC_STATUSCMD_REG:
            break;

            // For write
        case QSPIC_WRITEDATA_REG:
            break;

        default:
            printf("READ case 0x%X:\n", addr);
    }
    return 0;
}

static void da1469x_qspic_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) {

    DA1469xQspiState *s = DA1469XQSPI(opaque);

    if (addr == QSPIC_WRITEDATA_REG) {
        for (int i = 0; i < size; i++) {
            uint8_t byte = (value >> (8 * i)) & 0xff;
            uint32_t ret = ssi_transfer(s->spi, byte);
            printf("[QSPI]   - write 0x%X -> 0x%X\n", byte, ret);
        }
        return;
    }

    switch (addr) {
        case QSPIC_CTRLBUS_REG:
            if (value & (1 << 0))
                qemu_log_mask(LOG_GUEST_ERROR, "QSPIC_SET_SINGLE\n");
            if (value & (1 << 1))
                qemu_log_mask(LOG_GUEST_ERROR, "QSPIC_SET_DUAL\n");
            if (value & (1 << 2))
                qemu_log_mask(LOG_GUEST_ERROR, "QSPIC_SET_QUAD\n");
            if (value & (1 << 3)) {
                // QSPIC_EN_CS
                printf("[QSPI] CS_LOW\n");
                da1469x_qspi_set_cs(opaque, true);
            }
            if (value & (1 << 4)) {
                // QSPIC_DIS_CS
                printf("[QSPI] CS_HIGH\n");
                da1469x_qspi_set_cs(opaque, false);
            }
            break;

        case QSPIC_STATUSCMD_REG:
        case QSPIC_CTRLMODE_REG:
        case QSPIC_ERASECMDB_REG:
        case QSPIC_WRITEDATA_REG:
        case QSPIC_BURSTCMDB_REG:
        case QSPIC_ERASECMDA_REG:
        case QSPIC_BURSTBRK_REG:
        case QSPIC_BURSTCMDA_REG: {
            uint64_t old = da1469x_qspic_read(opaque, addr, size);
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%"HWADDR_PRIx": 0x%X -> 0x%X (size: %d)\n", addr, old, value, size);

        }

            break;

        default:
            printf("WRITE case 0x%X:\n", addr);
    }
}

static const MemoryRegionOps da1469x_qspic_ops = {
    .read = da1469x_qspic_read,
    .write = da1469x_qspic_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

#define SPI_OP_READ       0x03    /* Read data bytes (low frequency) */
static uint64_t da1469x_qspi_read(void *opaque, hwaddr addr, unsigned size) {
    DA1469xQspiState *s = DA1469XQSPI(opaque);

    da1469x_qspi_set_cs(opaque, true);

    ssi_transfer(s->spi, SPI_OP_READ);
    ssi_transfer(s->spi, (addr >> 16) & 0xff);
    ssi_transfer(s->spi, (addr >> 8) & 0xff);
    ssi_transfer(s->spi, (addr & 0xff));

    uint64_t ret = 0;
    for (int i = 0; i < size; i++) {
        ret |= ssi_transfer(s->spi, 0x0) << (8 * i);
    }

    da1469x_qspi_set_cs(opaque, false);
    return ret;
}
static void da1469x_qspi_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) {

}

static const MemoryRegionOps da1469x_qspi_ops = {
    .read = da1469x_qspi_read,
    .write = da1469x_qspi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void da1469x_qspi_reset(DeviceState *dev) {
    DA1469xQspiState *s = DA1469XQSPI(dev);

    s->AUTO_MD = 0;
    s->CLK_MD = 0;
    s->IO2_OEN = 0;
    s->IO3_OEN = 0;
    s->IO2_DAT = 0;
    s->IO3_DAT = 0;
    s->HRDY_MD = 0;
    s->RXD_NEG = 0;
    s->RPIPE_EN = 0;
    s->PCLK_MD = 0;
    s->BUF_LIM_EN = 0;
    s->USE_32BA = 0;
}

static void da1469x_qspi_init(Object *obj) {
}

static void da1469x_qspi_realize(DeviceState *dev, Error **errp) {
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    DA1469xQspiState *s = DA1469XQSPI(dev);

    memory_region_init_io(&s->control, dev, &da1469x_qspic_ops, s, "QSPIC", 0x100);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->control);

    memory_region_init_io(&s->iomem, dev, &da1469x_qspi_ops, s, "QSPI", 0x2000000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);

    s->spi = ssi_create_bus(DEVICE(dev), "spi");

    sysbus_init_irq(sbd, &s->_irq);
    ssi_auto_connect_slaves(DEVICE(dev), &s->cs_line, s->spi);
    sysbus_init_irq(sbd, &s->cs_line);
}

static void da1469x_qspi_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = da1469x_qspi_reset;
    dc->realize = da1469x_qspi_realize;
}

static const TypeInfo da1469x_qspi_info = {
    .name  = TYPE_DA1469X_QSPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(DA1469xQspiState),
    .class_init = da1469x_qspi_class_init,
    .instance_init = da1469x_qspi_init,
};

static void da1469x_qspi_register_types(void) {
    type_register_static(&da1469x_qspi_info);
}

type_init(da1469x_qspi_register_types)