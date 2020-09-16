#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"

#include "hw/arm/stm32l476_dma.h"

#define DMA_IFCR 0x04
#define DMA_CCRx 0x08

#define DMA_CCR2 0x1C
#define DMA_CNDTR2 0x20
#define DMA_CPAR2 0x24
#define DMA_CMAR2 0x28

#define DMA_CSELR 0xA8

static void stm32l476_dma_reset(DeviceState *dev) {
    STM32L476DmaState *s = STM32L476_DMA(dev);
}

static uint64_t stm32l476_dma_read(void *opaque, hwaddr offset,
                                   unsigned int size) {
    STM32L476DmaState *s = opaque;

    switch (offset) {
        case DMA_CCR2:
            break;

        case DMA_CSELR: {
            uint32_t out = 0;
            out |= s->channels[0].CxS << 0;
            out |= s->channels[1].CxS << 4;
            out |= s->channels[2].CxS << 8;
            out |= s->channels[3].CxS << 12;
            out |= s->channels[4].CxS << 16;
            out |= s->channels[5].CxS << 20;
            out |= s->channels[6].CxS << 24;
            return out;
        }

        default:
            qemu_log_mask(LOG_UNIMP, "%s: unimplemented device read "
                                     "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                          __func__, size, offset);
            break;
    }
    return 0;
}

static void stm32l476_dma_write(void *opaque, hwaddr offset,
                                uint64_t val64, unsigned int size) {
    STM32L476DmaState *s = opaque;

    if (offset >= DMA_CCRx) {
        hwaddr pos = offset - DMA_CCRx;
        int channel = pos / 0x14;
        int reg = pos % 0x14;

    }

    switch (offset) {
        case DMA_IFCR:
            for (int c = 0; c < 7; c++) {
                if (extract64(val64, c * 4 + 0, 1) == 1) {
                    // CGIF1
                }
                if (extract64(val64, c * 4 + 1, 1) == 1) {
                    // CTCIF1
                }
                if (extract64(val64, c * 4 + 2, 1) == 1) {
                    // CHTIF1
                }
                if (extract64(val64, c * 4 + 3, 1) == 1) {
                    // CTEIF1
                }
            }
            break;

        case DMA_CCR2:
            break;
        case DMA_CNDTR2:
            s->channels[1].NDTR = extract64(val64, 0, 16);
            break;
        case DMA_CPAR2:
            s->channels[1].PA = extract64(val64, 0, 32);
            break;
        case DMA_CMAR2:
            s->channels[1].MA = extract64(val64, 0, 32);
            break;

        case DMA_CSELR:
            s->channels[0].CxS = extract64(val64, 0, 4);
            s->channels[1].CxS = extract64(val64, 4, 4);
            s->channels[2].CxS = extract64(val64, 8, 4);
            s->channels[3].CxS = extract64(val64, 12, 4);
            s->channels[4].CxS = extract64(val64, 16, 4);
            s->channels[5].CxS = extract64(val64, 20, 4);
            s->channels[6].CxS = extract64(val64, 24, 4);
            break;

        default:
            qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                     "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                          __func__, size, val64, offset);
            break;
    }
}

static const MemoryRegionOps stm32l476_dma_ops = {
    .read = stm32l476_dma_read,
    .write = stm32l476_dma_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_dma_init(Object *obj) {
    STM32L476DmaState *s = STM32L476_DMA(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_dma_ops, s,
                          TYPE_STM32L476_DMA, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32l476_dma_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_dma_reset;
}

static const TypeInfo stm32l476_dma_info = {
    .name          = TYPE_STM32L476_DMA,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476DmaState),
    .instance_init = stm32l476_dma_init,
    .class_init    = stm32l476_dma_class_init,
};

static void stm32l476_dma_register_types(void) {
    type_register_static(&stm32l476_dma_info);
}

type_init(stm32l476_dma_register_types)
