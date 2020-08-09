#include "qemu/osdep.h"
#include "hw/arm/stm32l476_spi.h"
#include "exec/address-spaces.h"
#include "hw/arm/boot.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/ssi/ssi.h"
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "sysemu/sysemu.h"

#define SPIx_CR1 0x00
#define SPIx_CR2 0x04
#define SPIx_SR 0x08
#define SPIx_DR 0x0C

static void stm32l476_spi_set_txdmaen(STM32L476SpiState *s, bool enabled)
{
    if (s->txdmaen != enabled) {
        s->txdmaen = enabled;
        qemu_set_irq(s->txdrq, enabled);
    }
}

static void stm32l476_spi_reset(DeviceState *dev)
{
    STM32L476SpiState *s = STM32L476_SPI(dev);
}

static uint64_t stm32l476_spi_read(void *opaque, hwaddr offset,
                                   unsigned int size)
{
    STM32L476SpiState *s = opaque;

    switch (offset) {
    case SPIx_CR1:
        break;
    case SPIx_CR2: {
        uint32_t out = 0;
        out |= s->txdmaen << 1;
        return out;
    }

    case SPIx_SR: {
        uint32_t out = 0;
        out |= 0 << 0;  /* RXNE */
        out |= 1 << 1;  /* TXE */
        out |= 0 << 4;  /* CRCERR */
        out |= 0 << 5;  /* MODF */
        out |= 0 << 6;  /* OVR */
        out |= 0 << 7;  /* BSY */
        out |= 0 << 8;  /* FRE */
        out |= 0 << 9;  /* FRLVL */
        out |= 0 << 11; /* FTLVL */
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

static void stm32l476_spi_write(void *opaque, hwaddr offset, uint64_t val64,
                                unsigned int size)
{
    STM32L476SpiState *s = opaque;

    switch (offset) {
    case SPIx_CR1:
        break;
    case SPIx_CR2:
        stm32l476_spi_set_txdmaen(s, extract64(val64, 1, 1));
        break;
    case SPIx_DR: {
        if (size == 1) {
            ssi_transfer(s->spi, val64);
        } else {
            ssi_transfer(s->spi, val64 & 0xFF);
            ssi_transfer(s->spi, (val64 >> 8) & 0xFF);
        }
        break;
    }

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented device write "
                      "(size %d, value 0x%" PRIx64 ", offset 0x%" HWADDR_PRIx
                      ")\n",
                      __func__, size, val64, offset);
        break;
    }
}

static const MemoryRegionOps stm32l476_spi_ops = {
    .read = stm32l476_spi_read,
    .write = stm32l476_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_spi_init(Object *obj)
{
    STM32L476SpiState *s = STM32L476_SPI(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_spi_ops, s,
                          TYPE_STM32L476_SPI, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    s->spi = ssi_create_bus(DEVICE(obj), "ssi");
}

static Property intel_hda_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32l476_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_spi_reset;
    device_class_set_props(dc, intel_hda_properties);
}

static const TypeInfo stm32l476_spi_info = {
    .name = TYPE_STM32L476_SPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476SpiState),
    .instance_init = stm32l476_spi_init,
    .class_init = stm32l476_spi_class_init,
};

static void stm32l476_spi_register_types(void)
{
    type_register_static(&stm32l476_spi_info);
}

type_init(stm32l476_spi_register_types)
