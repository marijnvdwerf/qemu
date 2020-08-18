#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"

#include "hw/arm/stm32l476_lptim.h"

#define ISR       0x00
#define ICR       0x04
#define IER       0x08
#define CFGR      0x0C
#define CR        0x10
#define CMP       0x14
#define LPTIM_ARR       0x18
#define CNT       0x1C
#define OR        0x20

static void stm32l476_lptim_reset(DeviceState *dev) {
    STM32L476LPTimState *s = STM32L476_LPTIM(dev);
}

static uint64_t stm32l476_lptim_read(void *opaque, hwaddr offset,
                                     unsigned int size) {
    STM32L476LPTimState *s = opaque;

    switch (offset) {
        case ISR: {
            uint32_t out = 0;
            out |= s->CMPM << 0;
            out |= s->ARRM << 1;
            out |= s->EXTTRIG << 2;
            out |= s->CMPOK << 3;
            out |= s->ARROK << 4;
            out |= s->UP << 5;
            out |= s->DOWN << 6;
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

static void stm32l476_lptim_write(void *opaque, hwaddr offset,
                                  uint64_t val64, unsigned int size) {
    STM32L476LPTimState *s = opaque;

    switch (offset) {
        case ICR: {
            if ((val64 >> 0) & 1)
                s->CMPM = false;
            if ((val64 >> 1) & 1)
                s->ARRM = false;
            if ((val64 >> 2) & 1)
                s->EXTTRIG = false;
            if ((val64 >> 3) & 1)
                s->CMPOK = false;
            if ((val64 >> 4) & 1)
                s->ARROK = false;
            if ((val64 >> 5) & 1)
                s->UP = false;
            if ((val64 >> 6) & 1)
                s->DOWN = false;
            break;
        }

        case LPTIM_ARR: {
            s->ARR = val64 & 0xFFFF;
            s->ARROK = true;
            break;
        }

        default:
            qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                     "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                          __func__, size, val64, offset);
            break;
    }
}

static const MemoryRegionOps stm32l476_lptim_ops = {
    .read = stm32l476_lptim_read,
    .write = stm32l476_lptim_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_lptim_init(Object *obj) {
    STM32L476LPTimState *s = STM32L476_LPTIM(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_lptim_ops, s,
                          TYPE_STM32L476_LPTIM, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32l476_lptim_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_lptim_reset;
}

static const TypeInfo stm32l476_lptim_info = {
    .name          = TYPE_STM32L476_LPTIM,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476LPTimState),
    .instance_init = stm32l476_lptim_init,
    .class_init    = stm32l476_lptim_class_init,
};

static void stm32l476_lptim_register_types(void) {
    type_register_static(&stm32l476_lptim_info);
}

type_init(stm32l476_lptim_register_types)
