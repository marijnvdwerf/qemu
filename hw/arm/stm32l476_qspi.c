#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"
#include "hw/irq.h"

#include "hw/arm/stm32l476_qspi.h"

#define QUADSPI_CR 0x00
#define QUADSPI_DCR 0x04
#define QUADSPI_SR 0x08
#define QUADSPI_FCR 0x0C
#define QUADSPI_DLR 0x10
#define QUADSPI_CCR 0x14
#define QUADSPI_AR 0x18
#define QUADSPI_DR 0x20
#define QUADSPI_PSMKR 0x24
#define QUADSPI_PSMAR 0x28
#define QUADSPI_PIR 0x2C

#ifndef DEBUG_QSPI
#define DEBUG_QSPI 0
#endif

#define DPRINTF(fmt, ...) do {                                                 \
    if (DEBUG_QSPI) {                                                          \
        qemu_log("qspi: " fmt , ## __VA_ARGS__);                               \
    }                                                                          \
} while (0)

typedef enum
{
    Unknown,

    OnWriteCCR,
    OnWriteAR,
    OnWriteDR,
} StartMode;

static StartMode
getStart(STM32L476QspiState *s)
{
    if (s->FMODE == 0b00 || s->FMODE == 0b01)
    {
        // Indirect mode
        if (s->ADMODE == 0b00 && s->DMODE == 0b00)
            return OnWriteCCR;
        if (s->FMODE == 0b01 && s->ADMODE == 0b00)
            return OnWriteCCR;

        if (s->ADMODE != 0b00)
        {
            if (s->FMODE == 0b01 || s->DMODE == 0b00)
                return OnWriteAR;
        }

        if (s->FMODE == 0b00 && s->DMODE != 0b00)
            return OnWriteDR;
    }
    else if (s->FMODE == 0b10)
    {
        if (s->ADMODE == 0b00)
            return OnWriteCCR;

        return OnWriteAR;
    }
    else
    {
        assert(false);
    }

    return Unknown;
}

static void
do_transfer(STM32L476QspiState *s)
{
    // Instruction phase
    qemu_irq_lower(s->cs);
    DPRINTF("Instruction: 0x%02X\n", s->INSTRUCTION);
    ssi_transfer(s->qspi, s->INSTRUCTION);

    // Address phase
    if (s->ADMODE != 0b00)
    {
        switch (s->ADSIZE)
        {
            case 0b10:
            {
                DPRINTF("Address: 0x%06X\n", s->ADDRESS & 0xFFFFFF);
                ssi_transfer(s->qspi, (s->ADDRESS >> 16) & 0xFF);
                ssi_transfer(s->qspi, (s->ADDRESS >> 8) & 0xFF);
                ssi_transfer(s->qspi, (s->ADDRESS >> 0) & 0xFF);
                break;
            }
            default:assert(false);
                break;
        }
    }

    // Alternate-bytes phase
    if (s->ABMODE != 0b00)
    {
        assert(false);
    }

    // Dummy-cycles phase
    if (s->DCYC != 0)
    {
        if(s->INSTRUCTION == 0xEB && s->DCYC == 6) {

            // Argument byte
            uint8_t d = ssi_transfer(s->qspi, 0);
            DPRINTF("- DUMMY: 0x%02X\n", d);

            // 4 cycles
            for (int i = 0; i < 4; i++) {
                uint8_t d = ssi_transfer(s->qspi, 0);
                DPRINTF("- DUMMY: 0x%02X\n", d);
            }
        } else {
            for (int i = 0; i < s->DCYC; i++)
            {
                uint8_t d = ssi_transfer(s->qspi, 0);
                DPRINTF("- DUMMY: 0x%02X\n", d);
            }
        }

    }

    bool leaveCS = false;

    // Data phase
    DPRINTF("Data:\n");
    if (s->DMODE != 0)
    {
        if (s->FMODE == 0b01)
        {
            for (int i = 0; i < s->DL + 1; i++)
            {
                uint8_t d = ssi_transfer(s->qspi, 0);
                fifo8_push(&s->rx_fifo, d);
                DPRINTF("- 0x%02X\n", d);
            }
        }
        else if (s->FMODE == 0b00)
        {
            s->dataPushed = 0;
            leaveCS = true;
        }
        else
        {
            assert(false);
        }
    }

    if (!leaveCS)
    {
        qemu_irq_raise(s->cs);
    }

}

static void
start(STM32L476QspiState *s)
{

    if (s->FMODE == 0b01 || s->FMODE == 0b00)
    {
        s->BUSY = true;
        // QUADSPI indirect mode
        do_transfer(s);
        s->BUSY = false;
        if (s->FMODE == 0b01 && !fifo8_is_empty(&s->rx_fifo))
        {
            s->BUSY = true;
        }
        s->TCF = true;
    }
    else if (s->FMODE == 0b10)
    {
        // QUADSPI status flag polling mode
        DPRINTF("QSPI: CMD=0x%02X wait\n", s->INSTRUCTION);
        assert(s->PMM == 0);
        assert(s->APMS == 1);

        qemu_irq_lower(s->cs);
        while (true)
        {
            ssi_transfer(s->qspi, s->INSTRUCTION);
            uint32_t response = 0;
            for (int i = 0; i < 4; i++)
            {
                response |= ssi_transfer(s->qspi, 0) << 8 * i;
            }

            if ((response & s->MASK) == s->MATCH)
            {
                // match found
                s->SMF = true;
                // break
                break;
            }
        }
        qemu_irq_raise(s->cs);
    }
    else
    {
        assert(false);
    }
}

static void
stm32l476_quadspi_reset(DeviceState *dev)
{
    STM32L476QspiState *s = STM32L476_QSPI(dev);
}

static uint64_t
stm32l476_quadspi_read(void *opaque, hwaddr offset,
                       unsigned int size)
{
    STM32L476QspiState *s = opaque;
// 0801ce1e
    switch (offset)
    {
        case QUADSPI_CR:DPRINTF("QSPI: read CR\n");
            return 0;

        case QUADSPI_DCR:DPRINTF("QSPI: read DCR\n");
            return 0;

        case QUADSPI_SR:
        {
            uint32_t out = 0;
            out |= s->TEF << 0;
            out |= s->TCF << 1;

            // FTF
            if (s->FMODE == 0b01)
            {
                out |= (!fifo8_is_empty(&s->rx_fifo)) << 2;
            }
            else if (s->FMODE == 0b00)
            {
                out |= 1 << 2;
            }

            out |= s->SMF << 3;
            out |= s->TOF << 4;
            out |= s->BUSY << 5;
            DPRINTF("QSPI: read SR (0x%X)\n", out);
            return out;
        }

        case QUADSPI_DLR:DPRINTF("QSPI: read DLR\n");
            return s->DL;

        case QUADSPI_CCR:
        {
            uint32_t out = 0;
            out |= s->INSTRUCTION << 0;
            out |= s->IMODE << 8;
            out |= s->ADMODE << 10;
            out |= s->ADSIZE << 12;
            out |= s->ABMODE << 14;
            out |= s->ABSIZE << 16;
            out |= s->DCYC << 18;
            out |= s->DMODE << 24;
            out |= s->FMODE << 26;
            return out;
        }

        case QUADSPI_AR:DPRINTF("QSPI: read AR\n");
            return s->ADDRESS;

        case QUADSPI_DR:
            if (size == 1)
            {
                uint8_t kI = fifo8_pop(&s->rx_fifo);
                DPRINTF("QSPI: read DR (0x%02X)\n", kI);
                if (fifo8_is_empty(&s->rx_fifo))
                {
                    s->BUSY = false;
                    s->TCF = true;
                }
                return kI;
            }
            else
            {
                assert(false);
            }
            break;

        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device read "
                                         "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, offset);
            break;
    }
    return 0;
}

static void
stm32l476_quadspi_write(void *opaque, hwaddr offset,
                        uint64_t val64, unsigned int size)
{
    STM32L476QspiState *s = opaque;
    StartMode kStart;

    switch (offset)
    {
        case QUADSPI_CR:DPRINTF("QSPI: CR = 0x%08X\n", val64);
            s->EN = extract64(val64, 0, 1);
            s->ABORT = extract64(val64, 1, 1);
            s->APMS = extract64(val64, 22, 1);
            s->PMM = extract64(val64, 23, 1);

            if (s->ABORT)
            {
                s->TCF = true;
                s->ABORT = false;
            }

            break;

        case QUADSPI_DCR:DPRINTF("QSPI: DCR = 0x%08X\n", val64);
            break;

        case QUADSPI_FCR:DPRINTF("QSPI: FCR = 0x%08X\n", val64);
            if (val64 & (1 << 0))
                s->TEF = false;
            if (val64 & (1 << 1))
                s->TCF = false;
            if (val64 & (1 << 3))
                s->SMF = false;
            if (val64 & (1 << 4))
                s->TOF = false;
            break;

        case QUADSPI_DLR:s->DL = extract64(val64, 0, 32);
            break;

        case QUADSPI_CCR:s->INSTRUCTION = extract64(val64, 0, 8);
            s->IMODE = extract64(val64, 8, 2);
            s->ADMODE = extract64(val64, 10, 2);
            s->ADSIZE = extract64(val64, 12, 2);
            s->ABMODE = extract64(val64, 14, 2);
            s->ABSIZE = extract64(val64, 16, 2);
            s->DCYC = extract64(val64, 18, 5);
            s->DMODE = extract64(val64, 24, 2);
            s->FMODE = extract64(val64, 26, 2);

            kStart = getStart(s);
            if (kStart == OnWriteCCR)
            {
                start(s);
            }
            else
            {
                DPRINTF("Waiting to start with cmd 0x%X\n", s->INSTRUCTION);
            }

            break;

        case QUADSPI_AR:s->ADDRESS = extract64(val64, 0, 32);
            kStart = getStart(s);
            if (kStart == OnWriteAR)
            {
                start(s);
            }
            break;

        case QUADSPI_DR:
            if (size == 1)
            {
                kStart = getStart(s);
                if (kStart == OnWriteDR)
                {
                    start(s);
                }
                assert(s->FMODE == 0b00);
                ssi_transfer(s->qspi, val64);
                s->dataPushed++;

                if (s->dataPushed - 1 == s->DL)
                {
                    s->TCF = true;
                    s->BUSY = false;
                }
            }
            else
            {
                assert(false);
            }
            break;

        case QUADSPI_PSMKR:
            if (!s->BUSY)
            {
                s->MASK = extract64(val64, 0, 32);
            }
            break;

        case QUADSPI_PSMAR:
            if (!s->BUSY)
            {
                s->MATCH = extract64(val64, 0, 32);
            }
            break;

        case QUADSPI_PIR:
            if (!s->BUSY)
            {
                s->INTERVAL = extract64(val64, 0, 16);
            }
            break;

        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                         "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, val64, offset);
            break;
    }
}

static const MemoryRegionOps stm32l476_quadspi_ops = {
    .read = stm32l476_quadspi_read,
    .write = stm32l476_quadspi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static uint64_t
cache_read(void *opaque, hwaddr offset,
           unsigned int size)
{
    STM32L476QspiState *s = opaque;

    switch (offset)
    {
        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device read "
                                         "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, offset);
            break;
    }
    return 0;
}
static void
cache_write(void *opaque, hwaddr offset, uint64_t val64, unsigned int size)
{

    STM32L476QspiState *s = opaque;

    switch (offset)
    {
        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                         "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, val64, offset);
            break;
    }
}

static const MemoryRegionOps cache_ops = {
    .read = cache_read,
    .write = cache_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void
stm32l476_quadspi_init(DeviceState *dev, Error **pError)
{
    STM32L476QspiState *s = STM32L476_QSPI(dev);

    memory_region_init_io(&s->cache, dev, &cache_ops, s, "QSPI", 0x10000000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->cache);

    memory_region_init_io(&s->mmio, dev, &stm32l476_quadspi_ops, s,
                          "QPSI_R", 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    s->qspi = ssi_create_bus(dev, "qspi");

    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->cs);

    fifo8_create(&s->rx_fifo, 1024 * 1024);
}

static void
stm32l476_quadspi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_quadspi_reset;
    dc->realize = stm32l476_quadspi_init;
}

static const TypeInfo stm32l476_quadspi_info = {
    .name          = TYPE_STM32L476_QSPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476QspiState),
    .class_init    = stm32l476_quadspi_class_init,
};

static void
stm32l476_quadspi_register_types(void)
{
    type_register_static(&stm32l476_quadspi_info);
}

type_init(stm32l476_quadspi_register_types)
