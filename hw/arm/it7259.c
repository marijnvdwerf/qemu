#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "ui/console.h"

#define TYPE_IT7259 "it7259"
#define IT7259(obj) OBJECT_CHECK(IT7259State, (obj), TYPE_IT7259)

typedef struct
{
    I2CSlave parent_obj;

} IT7259State;

static int
it7259_event(I2CSlave *i2c, enum i2c_event event)
{
    IT7259State *s = IT7259(i2c);

    return 0;
}

static uint8_t
it7259_rx(I2CSlave *i2c)
{
    IT7259State *s = IT7259(i2c);

    return 0;
}

static int
it7259_tx(I2CSlave *i2c, uint8_t data)
{
    IT7259State *s = IT7259(i2c);
    printf("I2C: 0x%02X\n", data);

    return 0;
}

static void
it7259_realize(DeviceState *dev, Error **errp)
{
    IT7259State *s = IT7259(dev);
}

static void
it7259_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    dc->realize = it7259_realize;
    k->event = it7259_event;
    k->recv = it7259_rx;
    k->send = it7259_tx;
}

static const TypeInfo it7259_info = {
    .name          = TYPE_IT7259,
    .parent        = TYPE_I2C_SLAVE,
    .instance_size = sizeof(IT7259State),
    .class_init    = it7259_class_init,
};

static void
it7259_register_types(void)
{
    type_register_static(&it7259_info);
}

type_init(it7259_register_types)
