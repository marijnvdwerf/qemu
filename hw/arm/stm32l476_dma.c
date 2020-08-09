#include "qemu/osdep.h"
#include "hw/arm/stm32l476_dma.h"
#include "exec/address-spaces.h"
#include "hw/arm/boot.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "sysemu/sysemu.h"

#define DMA_ISR 0x00
#define DMA_IFCR 0x04
#define DMA_CCRx 0x08

#define DMA_CCR2 0x1C
#define DMA_CNDTR2 0x20
#define DMA_CPAR2 0x24
#define DMA_CMAR2 0x28

#define DMA_CSELR 0xA8

static void stm32l476_dma_reset(DeviceState *dev)
{
    STM32L476DmaState *s = STM32L476_DMA(dev);
}

static void update_interrupt(STM32L476DmaState *s)
{
    for (int c = 0; c < 7; c++) {
        int state = 0;
        if (s->channels[c].HTIE && s->channels[c].HTIF) {
            state = 1;
        }
        if (s->channels[c].TCIE && s->channels[c].TCIF) {
            state = 1;
        }
        if (s->channels[c].TEIE && s->channels[c].TEIF) {
            state = 1;
        }

        qemu_set_irq(s->irq[c], state);
    }
}

static void enable_channel(STM32L476DmaState *s, int c, bool enabled)
{
    STM32L476DmaChannelState *channel = &s->channels[c];
    if (enabled == channel->EN) {
        return;
    }

    channel->EN = enabled;
}

static uint64_t stm32l476_dma_read(void *opaque, hwaddr offset,
                                   unsigned int size)
{
    STM32L476DmaState *s = opaque;

    switch (offset) {
    case DMA_ISR: {
        uint32_t out = 0;
        for (int i = 0; i < 7; i++) {
            out |= (s->channels[i].TCIF || s->channels[i].HTIF ||
                    s->channels[i].TEIF)
                   << (0 + 4 * i);
            out |= s->channels[i].TCIF << (1 + 4 * i);
            out |= s->channels[i].HTIF << (2 + 4 * i);
            out |= s->channels[i].TEIF << (3 + 4 * i);
        }
        return out;
    }

    case DMA_CCR2: {
        STM32L476DmaChannelState *channel = &s->channels[1];
        uint32_t out = 0;
        out |= channel->EN << 0;
        out |= channel->TCIE << 1;
        out |= channel->HTIE << 2;
        out |= channel->TEIE << 3;
        out |= channel->DIR << 4;
        out |= channel->CIRC << 5;
        out |= channel->PINC << 6;
        out |= channel->MINC << 7;
        out |= channel->PSIZE << 8;
        out |= channel->MSIZE << 10;
        out |= channel->PL << 12;
        out |= channel->MEM2MEM << 14;
        return out;
    }

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
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented device read "
                      "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                      __func__, size, offset);
        break;
    }
    return 0;
}

static void stm32l476_dma_write(void *opaque, hwaddr offset, uint64_t val64,
                                unsigned int size)
{
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
                /* CGIF1 */
                s->channels[c].TCIF = false;
                s->channels[c].HTIF = false;
                s->channels[c].TEIF = false;
            }
            if (extract64(val64, c * 4 + 1, 1) == 1) {
                /* CTCIF1 */
                s->channels[c].TCIF = false;
            }
            if (extract64(val64, c * 4 + 2, 1) == 1) {
                /* CHTIF1 */
                s->channels[c].HTIF = false;
            }
            if (extract64(val64, c * 4 + 3, 1) == 1) {
                /* CTEIF1 */
                s->channels[c].TEIF = false;
            }
        }
        update_interrupt(s);
        break;

    case DMA_CCR2:
        enable_channel(s, 1, extract64(val64, 0, 1));

        /* not read-only, should warn */
        s->channels[1].TCIE = extract64(val64, 1, 1);
        s->channels[1].HTIE = extract64(val64, 2, 1);
        s->channels[1].TEIE = extract64(val64, 3, 1);
        s->channels[1].CIRC = extract64(val64, 5, 1);

        /* read only, should probably error on difference */
        if (s->channels[1].EN == false) {
            s->channels[1].DIR = extract64(val64, 4, 1);
            s->channels[1].PINC = extract64(val64, 6, 1);
            s->channels[1].MINC = extract64(val64, 7, 1);
            s->channels[1].PSIZE = extract64(val64, 8, 2);
            s->channels[1].MSIZE = extract64(val64, 10, 2);
            s->channels[1].PL = extract64(val64, 12, 2);
            s->channels[1].MEM2MEM = extract64(val64, 14, 1);
        }
        break;
    case DMA_CNDTR2:
        /* read only, should probably error on difference */
        if (s->channels[1].EN == false) {
            s->channels[1].NDTR = extract64(val64, 0, 16);
        }
        break;
    case DMA_CPAR2:
        /* not read-only, should warn */
        s->channels[1].PA = extract64(val64, 0, 32);
        break;
    case DMA_CMAR2:
        /* not read-only, should warn */
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
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented device write "
                      "(size %d, value 0x%" PRIx64 ", offset 0x%" HWADDR_PRIx
                      ")\n",
                      __func__, size, val64, offset);
        break;
    }
}

static const MemoryRegionOps stm32l476_dma_ops = {
    .read = stm32l476_dma_read,
    .write = stm32l476_dma_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void omap_dma_request(void *opaque, int drq, int req)
{
    if (req == 0) {
        return;
    }

    STM32L476DmaChannelState *channel = opaque;
    if (channel->CxS != drq) {
        return;
    }

    assert(channel->EN);

    assert(channel->DIR == 1);
    assert(channel->PSIZE == channel->MSIZE);
    assert(channel->PSIZE == 0);
    assert(channel->PINC == false);
    assert(channel->MINC == true);

    uint8_t *buffer = malloc(channel->NDTR);
    cpu_physical_memory_read(channel->MA, buffer, channel->NDTR);
    for (int i = 0; i < channel->NDTR; i++) {
        cpu_physical_memory_write(channel->PA, &buffer[i], 1);
    }

    free(buffer);

    channel->TCIF = true;
    update_interrupt(channel->parent);
}

static void stm32l476_dma_init(Object *obj)
{
    STM32L476DmaState *s = STM32L476_DMA(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_dma_ops, s,
                          TYPE_STM32L476_DMA, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    for (int i = 0; i < 7; i++) {
        sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq[i]);
    }

    for (int c = 0; c < 7; c++) {
        s->channels[c].parent = s;
        s->channels[c].id = c;
        s->channels[c].drq =
            qemu_allocate_irqs(omap_dma_request, &s->channels[c], 8);
    }
}

static void stm32l476_dma_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_dma_reset;
}

static const TypeInfo stm32l476_dma_info = {
    .name = TYPE_STM32L476_DMA,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476DmaState),
    .instance_init = stm32l476_dma_init,
    .class_init = stm32l476_dma_class_init,
};

static void stm32l476_dma_register_types(void)
{
    type_register_static(&stm32l476_dma_info);
}

type_init(stm32l476_dma_register_types)
