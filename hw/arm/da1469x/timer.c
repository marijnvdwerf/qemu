#include <stdint.h>

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "timer.h"
#include "qemu/log.h"

#ifndef STM_TIMER_ERR_DEBUG
#define STM_TIMER_ERR_DEBUG 1
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_TIMER_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#define TIMER_CTRL_REG              0x00
#define TIMER_TIMER_VAL_REG         0x04
#define TIMER_STATUS_REG            0x08
#define TIMER_GPIO1_CONF_REG        0x0C
#define TIMER_GPIO2_CONF_REG        0x10
#define TIMER_RELOAD_REG            0x14
// reserved
#define TIMER_PRESCALER_REG         0x1C
#define TIMER_CAPTURE_GPIO1_REG     0x20
#define TIMER_CAPTURE_GPIO2_REG     0x24
#define TIMER_PRESCALER_VAL_REG     0x28
#define TIMER_PWM_FREQ_REG          0x2C
#define TIMER_PWM_DC_REG            0x30
#define TIMER_CLEAR_IRQ_REG         0x34

static void da1469x_timer_set_alarm(DA1469xTimerState *s, int64_t now) {

    int stepsize = NANOSECONDS_PER_SECOND ;
    timer_mod(s->timer, stepsize);
}

static void da1469x_timer_interrupt(void *opaque) {
    DA1469xTimerState *s = opaque;

    qemu_irq_pulse(s->irq);
}
static void da1469x_timer_reset(DeviceState *dev) {

}
static uint64_t da1469x_timer_read(void *opaque, hwaddr offset,
                                   unsigned size) {

    switch (offset) {
        case TIMER_CTRL_REG:
        case TIMER_TIMER_VAL_REG:
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, offset);
    };
    return 0;
}

static void da1469x_timer_write(void *opaque, hwaddr offset,
                                uint64_t val64, unsigned size) {
    DA1469xTimerState *s = opaque;
    uint32_t value = val64;
    DB_PRINT("Write 0x%x, 0x%"HWADDR_PRIx"\n", value, offset);

    switch (offset) {
        case TIMER_CTRL_REG:
            s->en = (value >> 0u) & 1u;
            s->oneshot_mode = (value >> 1u) & 1u;
            s->count_down = (value >> 2u) & 1u;
            s->IN1_EVENT_FALL_EN = (value >> 3u) & 1u;
            s->IN2_EVENT_FALL_EN = (value >> 4u) & 1u;
            s->IRQ_EN = (value >> 5u) & 1u;
            s->FREE_RUN_MODE_EN = (value >> 6u) & 1u;
            s->SYS_CLK_EN = (value >> 7u) & 1u;
            s->CLK_EN = (value >> 8u) & 1u;
            break;
        case TIMER_GPIO1_CONF_REG:
            s->gpio1_conf = value & 0x3Fu;
            break;
        case TIMER_GPIO2_CONF_REG:
            s->gpio2_conf = value & 0x3Fu;
            break;
        case TIMER_RELOAD_REG:
            s->reload = value & 0xFFFFFFu;
            break;
        case TIMER_PRESCALER_REG:
            s->prescaler = value & 0x1Fu;
        case TIMER_CLEAR_IRQ_REG:
            // Write any value clear interrupt
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, offset);
    };

    if (s->en) {
        da1469x_timer_set_alarm(s, s->hit_time);
        da1469x_timer_set_alarm(s, s->hit_time);
    }
}

static const MemoryRegionOps da1469x_timer_ops = {
    .read = da1469x_timer_read,
    .write = da1469x_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Property da1469x_timer_properties[] = {
//    DEFINE_PROP_UINT64("clock-frequency", DA1469xTimerState ,
//    freq_hz, 1000000000),
    DEFINE_PROP_END_OF_LIST(),
};

static void da1469x_timer_init(Object *obj) {
    DA1469xTimerState *s = DA1469XTIMER(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->iomem, obj, &da1469x_timer_ops, s, "da1469x_timer", 0x100);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static void da1469x_timer_realize(DeviceState *dev, Error **errp) {
    DA1469xTimerState *s = DA1469XTIMER(dev);
    s->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, da1469x_timer_interrupt, s);
}

static void da1469x_timer_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = da1469x_timer_reset;
    device_class_set_props(dc, da1469x_timer_properties);
    dc->realize = da1469x_timer_realize;
}

static const TypeInfo da1469x_timer_info = {
    .name          = TYPE_DA1469X_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DA1469xTimerState),
    .instance_init = da1469x_timer_init,
    .class_init    = da1469x_timer_class_init,
};

static void da1469x_timer_register_types(void) {
    type_register_static(&da1469x_timer_info);
}

type_init(da1469x_timer_register_types)
