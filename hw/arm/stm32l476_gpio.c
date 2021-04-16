#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"
#include "hw/arm/stm32l476_gpio.h"

typedef struct
{
    uint8_t mode;
    uint64_t AFSEL;
    uint64_t OT;
    uint64_t PUPD;
    uint64_t OSPEED;
} GpioPin;

static const int GPIOx_MODER = 0x00;
static const int GPIOx_OTYPER = 0x04;
static const int GPIOx_OSPEEDR = 0x08;
static const int GPIOx_PUPDR = 0x0C;
static const int GPIOx_IDR = 0x10;
static const int GPIOx_BSRR = 0x18;
static const int GPIOx_AFRH = 0x24;
static const int GPIOx_BRR = 0x28;
static const int GPIOx_ASCR = 0x2c;

struct __STM32L476GpioState
{
    /* <private> */
    SysBusDevice parent_obj;

    GpioPin pins[16];

    /* <public> */
    MemoryRegion mmio;
    qemu_irq irq;
};

static void
stm32l476_gpio_reset(DeviceState *dev)
{
    STM32L476GpioState *s = STM32L476_GPIO(dev);
}

static uint64_t
stm32l476_gpio_read(void *opaque, hwaddr offset,
                    unsigned int size)
{
    STM32L476GpioState *s = opaque;

    assert(size == 4);
    switch (offset)
    {
        case GPIOx_MODER:
        {
            uint32_t out = 0;

            for (int i = 0; i < 16; i++)
            {
                out |= s->pins[i].mode << (i * 2);
            }

            return out;
        }

        case GPIOx_OTYPER:
        {
            uint32_t out = 0;

            for (int i = 0; i < 16; i++)
            {
                out |= s->pins[i].OT << i;
            }

            return out;
        }

        case GPIOx_OSPEEDR:
        {
            uint32_t out = 0;

            for (int i = 0; i < 16; i++)
            {
                out |= s->pins[i].OSPEED << (i * 2);
            }

            return out;
        }

        case GPIOx_PUPDR:
        {
            uint32_t out = 0;

            for (int i = 0; i < 16; i++)
            {
                out |= s->pins[i].PUPD << (i * 2);
            }

            return out;
        }

        case GPIOx_AFRL:
        {
            uint32_t out = 0;

            for (int i = 0; i < 8; i++)
            {
                out |= s->pins[i].AFSEL << (i * 4);
            }

            return out;
        }

        case GPIOx_AFRH:
        {
            uint32_t out = 0;

            for (int i = 0; i < 8; i++)
            {
                out |= s->pins[8 + i].AFSEL << (i * 4);
            }

            return out;
        }

        case GPIOx_IDR:
        {
            return 0x20;
        }

        case GPIOx_ASCR:
        default:
        {
            qemu_log_mask(LOG_UNIMP, "%s: unimplemented device read "
                                     "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                          __func__, size, offset);
            break;
        }
    }
    return 0;
}

static void
stm32l476_gpio_write(void *opaque, hwaddr offset,
                     uint64_t val64, unsigned int size)
{
    STM32L476GpioState *s = opaque;

    GpioPin backup[16] = {0};
    memcpy(backup, s->pins, sizeof(s->pins));

    assert(size == 4);
    switch (offset)
    {
        case GPIOx_MODER:
        {
            for (int i = 0; i < 16; i++)
            {
                s->pins[i].mode = extract64(val64, i * 2, 2);
            }
            break;
        }

        case GPIOx_OTYPER:
        {
            for (int i = 0; i < 16; i++)
            {
                s->pins[i].OT = extract64(val64, i, 1);
            }
            break;
        }

        case GPIOx_OSPEEDR:
        {
            for (int i = 0; i < 16; i++)
            {
                s->pins[i].OSPEED = extract64(val64, i * 2, 2);
            }
            break;
        }

        case GPIOx_PUPDR:
        {
            for (int i = 0; i < 16; i++)
            {
                s->pins[i].PUPD = extract64(val64, i * 2, 2);
            }
            break;
        }

        case GPIOx_BSRR:
        {
            //   If there is an attempt to both set and reset a bit in GPIOx_BSRR, the set action takes priority.
            for (int i = 0; i < 16; i++)
            {
                if (extract64(val64, i, 1))
                {
                    //set
                }

                if (extract64(val64, 16 + i, 1))
                {
                    //reset
                }
            }
            break;
        }

        case GPIOx_AFRL:
        {
            for (int i = 0; i < 8; i++)
            {
                s->pins[i].AFSEL = extract64(val64, i * 4, 4);
            }
            break;
        }

        case GPIOx_AFRH:
        {
            for (int i = 0; i < 8; i++)
            {
                s->pins[i + 8].AFSEL = extract64(val64, i * 4, 4);
            }
            break;
        }

        case GPIOx_BRR:
        {
            for (int i = 0; i < 16; i++)
            {
                if (extract64(val64, i, 1))
                {
                    // Reset the corresponding ODx bit
                }
            }
            break;
        }

        case GPIOx_ASCR:
        {
            for (int i = 0; i < 16; i++)
            {
                if (extract64(val64, i, 1))
                {
                    // Connect analog switch to the ADC input
                }
            }
            break;
        };

        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                         "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, val64, offset);
            break;
    }

    for (int i = 0; i < 16; i++)
    {
        GpioPin *x;
        if (memcmp(&backup[i], &s->pins[i], sizeof(s->pins[0])) == 0)
            continue;

        printf("[%s] Pin %d changed to ", DEVICE(s)->id, i);

//        x = &backup[i];
//        printf("[mode:%d AFSEL:%d ospeed:%d ot:%d pupd:%d]", x->mode, x->AFSEL, x->OSPEED, x->OT, x->PUPD);
//        printf(" to ");
        x = &s->pins[i];
        printf("[mode:%d ", x->mode);
        if (x->mode == 0b10)
        {
            printf("AFSEL:%d ", x->AFSEL);
        }
        printf("ospeed:%d ot:%d pupd:%d]", x->OSPEED, x->OT, x->PUPD);
        printf("\n");
    }
}

static const MemoryRegionOps stm32l476_gpio_ops = {
    .read = stm32l476_gpio_read,
    .write = stm32l476_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void
stm32l476_gpio_init(Object *obj)
{
    STM32L476GpioState *s = STM32L476_GPIO(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_gpio_ops, s,
                          TYPE_STM32L476_GPIO, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void
stm32l476_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_gpio_reset;
}

static const TypeInfo stm32l476_gpio_info = {
    .name          = TYPE_STM32L476_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476GpioState),
    .instance_init = stm32l476_gpio_init,
    .class_init    = stm32l476_gpio_class_init,
};

static void
stm32l476_gpio_register_types(void)
{
    type_register_static(&stm32l476_gpio_info);
}

type_init(stm32l476_gpio_register_types)
