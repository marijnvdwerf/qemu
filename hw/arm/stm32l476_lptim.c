#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"
#include "hw/irq.h"

#include "hw/arm/stm32l476_lptim.h"


// NOTE: The usleep() helps the MacOS stdout from freezing when we have a lot of print out
#define DPRINTF(fmt, ...) \
    do { \
        qemu_log_mask(LOG_GUEST_ERROR,"LPTIM %s: " fmt , __func__, ## __VA_ARGS__); \
        usleep(100); \
    } while (0)

#define ISR       0x00
#define LPTIM_ICR       0x04
#define LPTIM_IER       0x08
#define LPTIM_CFGR      0x0C
#define LPTIM_CR        0x10
#define CMP       0x14
#define LPTIM_ARR       0x18
#define LPTIM_CNT       0x1C
#define LPTIM_OR        0x20

#define TIMER_NS (1000000000/32768)

static int64_t stm32l47xx_lptim_next_transition(STM32L476LPTimState *s, int64_t current_time) {
    return current_time + TIMER_NS * s->ARR ;
}

static void stm32l476_lptim_interrupt(void *opaque) {
    STM32L476LPTimState *s = opaque;

    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->start = now;
    timer_mod(s->timer, stm32l47xx_lptim_next_transition(s, now));

    if (s->ARRMIE && s->ARRM == false) {
        s->ARRM = true;
        qemu_set_irq(s->irq, 1);
    }

}

static void stm32l476_lptim_reset(DeviceState *dev) {
    STM32L476LPTimState *s = STM32L476_LPTIM(dev);
    s->CNT = 0;
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
        case LPTIM_CNT: {
            int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
            int64_t delta = now - s->start;

            int16_t ticks = delta / TIMER_NS;
            return ticks;
        }

        case LPTIM_IER: {
            uint32_t out = 0;
            out |= s->CMPMIE << 0;
            out |= s->ARRMIE << 1;
            out |= s->EXTTRIGIE << 2;
            out |= s->CMPOKIE << 3;
            out |= s->ARROKIE << 4;
            out |= s->UPIE << 5;
            out |= s->DOWNIE << 6;
            return out;
        }

        case LPTIM_ICR:
        case LPTIM_CFGR:
        case LPTIM_ARR:
            break;

        case LPTIM_CR: {
            uint32_t out = 0;
            out |= s->ENABLE << 0;
            out |= s->SNGSTRT << 1;
            out |= s->CNTSTRT << 2;
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
        case LPTIM_ICR: {
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

        case LPTIM_CR: {
            bool ENABLE = (val64 >> 0) & 1;
            bool SNGSTRT = (val64 >> 1) & 1;
            bool CNTSTRT = (val64 >> 2) & 1;
            if (CNTSTRT == true && s->CNTSTRT == false) {
                DPRINTF("started\n");
                int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
                s->start = now;
                timer_mod(s->timer, stm32l47xx_lptim_next_transition(s, now));
            } else if (CNTSTRT == false && s->CNTSTRT == true) {
                DPRINTF("stopped\n");
                timer_del(s->timer);
            }

            s->ENABLE = ENABLE;
            s->SNGSTRT = SNGSTRT;
            s->CNTSTRT = CNTSTRT;
            DPRINTF("ENABLE:%d SNGSTRT:%d CNTSTRT:%d\n", ENABLE, SNGSTRT, CNTSTRT);
            break;
        }

        case LPTIM_IER: {
            if (s->ENABLE) {
                qemu_log_mask(LOG_GUEST_ERROR,
                              "The LPTIM_IER register must only be modified when the LPTIM is disabled");
                return;
            }
            s->CMPMIE = (val64 >> 0) & 1;
            s->ARRMIE = (val64 >> 1) & 1;
            s->EXTTRIGIE = (val64 >> 2) & 1;
            s->CMPOKIE = (val64 >> 3) & 1;
            s->ARROKIE = (val64 >> 4) & 1;
            s->UPIE = (val64 >> 5) & 1;
            s->DOWNIE = (val64 >> 6) & 1;
        }

//        case LPTIM_CFGR :
//        case LPTIM_OR:
//            break;

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
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);
}

static void stm32l476_lptim_realize(DeviceState *dev, Error **errp) {
    STM32L476LPTimState *s = STM32L476_LPTIM(dev);
    s->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, stm32l476_lptim_interrupt, s);
}

static void stm32l476_lptim_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_lptim_reset;
    dc->realize = stm32l476_lptim_realize;
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
