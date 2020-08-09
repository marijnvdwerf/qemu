#include "qemu/osdep.h"
#include "hw/arm/stm32l476_pwr.h"
#include "exec/address-spaces.h"
#include "hw/arm/boot.h"
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "sysemu/sysemu.h"

#define CR1 0x00
#define SR2 0x14

static void stm32l476_pwr_reset(DeviceState *dev)
{
    STM32L476PwrState *s = STM32L476_PWR(dev);

    /* CR1 */
    s->LPMS = 0;
    s->LPMS = 0;
    s->LPMS = 0;
    s->LPMS = 0;

    /* SR2 */
    s->REGLPS = 0;
    s->REGLPF = 0;
    s->VOSF = 0;
    s->PVDO = 0;
    s->PVMO1 = 0;
    s->PVMO2 = 0;
    s->PVMO3 = 0;
    s->PVMO4 = 0;
}

static uint64_t stm32l476_pwr_read(void *opaque, hwaddr offset,
                                   unsigned int size)
{
    STM32L476PwrState *s = opaque;

    switch (offset) {
    case CR1: {
        uint32_t out = 0;

        out |= s->LPMS << 0;
        out |= s->DBP << 8;
        out |= s->VOS << 9;
        out |= s->LPR << 14;
        return out;
    }
    case SR2: {
        uint32_t out = 0;

        out |= s->REGLPS << 8;
        out |= s->REGLPF << 9;
        out |= s->VOSF << 10;
        out |= s->PVDO << 11;
        out |= s->PVMO1 << 12;
        out |= s->PVMO2 << 13;
        out |= s->PVMO3 << 14;
        out |= s->PVMO4 << 15;
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

static void stm32l476_pwr_write(void *opaque, hwaddr offset, uint64_t val64,
                                unsigned int size)
{
    STM32L476PwrState *s = opaque;

    switch (offset) {
    case CR1: {
        s->LPMS = (val64 >> 0) & 0b111;
        s->DBP = (val64 >> 8) & 0b1;
        /* TODO: disallow writing 0b00/0b11 to VOS */
        s->VOS = (val64 >> 9) & 0b11;
        s->LPR = (val64 >> 14) & 0b1;
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

static const MemoryRegionOps stm32l476_pwr_ops = {
    .read = stm32l476_pwr_read,
    .write = stm32l476_pwr_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_pwr_init(Object *obj)
{
    STM32L476PwrState *s = STM32L476_PWR(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_pwr_ops, s,
                          TYPE_STM32L476_PWR, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32l476_pwr_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_pwr_reset;
}

static const TypeInfo stm32l476_pwr_info = {
    .name = TYPE_STM32L476_PWR,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476PwrState),
    .instance_init = stm32l476_pwr_init,
    .class_init = stm32l476_pwr_class_init,
};

static void stm32l476_pwr_register_types(void)
{
    type_register_static(&stm32l476_pwr_info);
}

type_init(stm32l476_pwr_register_types)
