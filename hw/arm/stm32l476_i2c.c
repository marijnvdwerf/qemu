#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"
#include "hw/i2c/i2c.h"

#include "hw/arm/stm32l476_i2c.h"
#include "hw/qdev-properties.h"

#include "qapi/error.h"

#define I2C_CR1 0x00
#define I2C_CR2 0x04
#define I2C_OAR1 0x08
#define I2C_OAR2 0x0C
#define I2C_TIMINGR 0x10
#define I2C_ISR 0x18
#define I2C_TXDR 0x28

static void
stm32l476_i2c_reset(DeviceState *dev)
{
    STM32L476I2CState *s = STM32L476_I2C(dev);
}

static uint64_t
stm32l476_i2c_read(void *opaque, hwaddr offset,
                   unsigned int size)
{
    STM32L476I2CState *s = opaque;

    switch (offset)
    {
        case I2C_CR2:
        {
            uint32_t out = 0;
            out |= s->SADD << 0;
            out |= s->RD_WRN << 10;
            out |= s->ADD10 << 11;
            out |= s->HEAD10R << 12;
            out |= s->START << 13;
            out |= s->STOP << 14;
            out |= s->NACK << 15;

            out |= s->NBYTES << 16;
            out |= s->RELOAD << 24;
            out |= s->AUTOEND << 25;
            out |= s->PECBYTE << 26;
            return out;
        }

            break;
//        case I2C_CR1:
//        case I2C_OAR1:
//        case I2C_OAR2:
        case I2C_ISR:
        {
            uint32_t out = 0;
            out |= s->TXIS << 1;
            out |= s->BUSY << 15;
            return out;
        }
//            break;
        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device read "
                                         "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, offset);
            break;
    }
    return 0;
}

static void
stm32l476_i2c_write(void *opaque, hwaddr offset,
                    uint64_t val64, unsigned int size)
{
    STM32L476I2CState *s = opaque;
    assert(size == 4);

    switch (offset)
    {
        case I2C_CR2:
        {
            s->SADD = extract64(val64, 0, 10);
            s->RD_WRN = extract64(val64, 10, 1);
            s->ADD10 = extract64(val64, 11, 1);
            s->HEAD10R = extract64(val64, 12, 1);

            if (extract64(val64, 13, 1) == true)
                s->START = true;
            if (extract64(val64, 14, 1) == true)
                s->STOP = true;
            if (extract64(val64, 15, 1) == true)
                s->NACK = true;

            s->NBYTES = extract64(val64, 16, 8);
            s->RELOAD = extract64(val64, 24, 1);
            s->AUTOEND = extract64(val64, 25, 1);

            if (extract64(val64, 26, 1) == true)
                s->PECBYTE = true;

            if (s->START && !s->BUSY)
            {
                s->BUSY = true;
                i2c_start_transfer(s->bus, (s->SADD & 0b0011111110) >> 1, s->RD_WRN == 1);
            }
            s->TXIS = true;
            break;
        }

        case I2C_TXDR:
        {
            printf("[%s] Send data to 0x%X: 0x%02X\n", s->name, (s->SADD & 0b0011111110) >> 1, val64);
            i2c_send(s->bus, val64 & 0xFF);
            break;
        }

        case I2C_CR1:
        case I2C_OAR1:
        case I2C_OAR2:
        case I2C_TIMINGR:break;
        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                         "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, val64, offset);
            break;
    }
}

static const MemoryRegionOps stm32l476_i2c_ops = {
    .read = stm32l476_i2c_read,
    .write = stm32l476_i2c_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void
stm32l476_i2c_realize(DeviceState *dev, Error **errp)
{
    STM32L476I2CState *s = STM32L476_I2C(dev);

    if (s->name == NULL)
    {
        error_setg(errp, "property 'name' not specified");
        return;
    }

    memory_region_init_io(&s->mmio, OBJECT(s), &stm32l476_i2c_ops, s,
                          TYPE_STM32L476_I2C, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    s->bus = i2c_init_bus(dev, "i2c");
}

static Property stm32l476_i2c_properties[] = {
    DEFINE_PROP_STRING("name", STM32L476I2CState, name),
    DEFINE_PROP_END_OF_LIST(),
};

static void
stm32l476_i2c_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_i2c_reset;
    dc->realize = stm32l476_i2c_realize;
    device_class_set_props(dc, stm32l476_i2c_properties);
}

static const TypeInfo stm32l476_i2c_info = {
    .name          = TYPE_STM32L476_I2C,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476I2CState),
    .class_init    = stm32l476_i2c_class_init,
};

static void
stm32l476_i2c_register_types(void)
{
    type_register_static(&stm32l476_i2c_info);
}

type_init(stm32l476_i2c_register_types)
