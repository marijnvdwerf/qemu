#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"

typedef struct
{
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    uint8_t EXTI[16];
} STM32L476SysCfgState;

#define TYPE_STM32L476_SYSCFG "stm32l476-syscfg"

#define STM32L476_SYSCFG(obj) \
    OBJECT_CHECK(STM32L476SysCfgState, (obj), TYPE_STM32L476_SYSCFG)

static const int SYSCFG_EXTICR1 = 0x08;
static const int SYSCFG_EXTICR2 = 0x0c;
static const int SYSCFG_EXTICR4 = 0x14;
static const int SYSCFG_EXTICR3 = 0x10;


static void
stm32l476_syscfg_reset(DeviceState *dev)
{
    STM32L476SysCfgState *s = STM32L476_SYSCFG(dev);

    for (int i = 0; i < 16; i++)
    {
        s->EXTI[i] = 0b1111;
    }

}

static uint32_t
getInt(const STM32L476SysCfgState *s, uint32_t offset)
{
    uint32_t out = 0;
    for (int i = 0; i < 4; i++)
    {
        out |= s->EXTI[i + offset] << (i * 4);
    }
    return out;
}
static uint64_t
stm32l476_syscfg_read(void *opaque, hwaddr offset,
                      unsigned size)
{
    STM32L476SysCfgState *s = STM32L476_SYSCFG(opaque);

    switch (offset)
    {
        case 0x04:break;
        case SYSCFG_EXTICR1:
        {
            return getInt(s, 0);
        }
        case SYSCFG_EXTICR2:
        {
            return getInt(s, 4);
        }
        case SYSCFG_EXTICR3:
        {
            return getInt(s, 8);
        }
        case SYSCFG_EXTICR4:
        {
            return getInt(s, 12);
        }

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
stm32l476_syscfg_write(void *opaque, hwaddr offset,
                       uint64_t val64, unsigned size)
{
    switch (offset)
    {
        case 0x04:
//        case SYSCFG_EXTICR1:
//        case SYSCFG_EXTICR2:
//        case SYSCFG_EXTICR4:
            break;

        default:qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                         "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                              __func__, size, val64, offset);
            break;

    }
}

static const MemoryRegionOps stm32l476_syscfg_ops = {
    .read = stm32l476_syscfg_read,
    .write =stm32l476_syscfg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void
stm32l476_syscfg_init(Object *obj)
{
    STM32L476SysCfgState *s = STM32L476_SYSCFG(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_syscfg_ops, s,
                          TYPE_STM32L476_SYSCFG, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void
stm32l476_syscfg_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_syscfg_reset;
}

static const TypeInfo stm32l476_syscfg_info = {
    .name          = TYPE_STM32L476_SYSCFG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476SysCfgState),
    .instance_init = stm32l476_syscfg_init,
    .class_init    = stm32l476_syscfg_class_init,
};

static void
stm32l476_syscfg_register_types(void)
{
    type_register_static(&stm32l476_syscfg_info);
}

type_init(stm32l476_syscfg_register_types)
