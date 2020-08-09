#include "qemu/osdep.h"
#include "hw/arm/stm32l476_flash.h"
#include "exec/address-spaces.h"
#include "hw/arm/boot.h"
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "sysemu/sysemu.h"

#define ACR 0x00
#define PDKEYR 0x04
#define KEYR 0x08
#define OPTKEYR 0x0C
#define SR 0x10
#define CR 0x14
#define ECCR 0x18
#define RESERVED1 0x1C
#define OPTR 0x20
#define PCROP1SR 0x24
#define PCROP1ER 0x28
#define WRP1AR 0x2C
#define WRP1BR 0x30
#define PCROP2SR 0x44
#define PCROP2ER 0x48
#define WRP2AR 0x4C
#define WRP2BR 0x50

static void stm32l476_flash_reset(DeviceState *dev)
{
    STM32L476FlashState *s = STM32L476_FLASH(dev);
    s->acr_latency = 0;
}

static uint64_t stm32l476_flash_read(void *opaque, hwaddr offset,
                                     unsigned int size)
{
    STM32L476FlashState *s = opaque;

    switch (offset) {
    case ACR:
        return s->acr_latency;
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented device read "
                      "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                      __func__, size, offset);
        break;
    }
    return 0;
}

static void stm32l476_flash_write(void *opaque, hwaddr offset, uint64_t val64,
                                  unsigned int size)
{
    STM32L476FlashState *s = opaque;

    switch (offset) {
    case ACR:
        s->acr_latency = val64 & 0x7;
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

static const MemoryRegionOps stm32l476_flash_ops = {
    .read = stm32l476_flash_read,
    .write = stm32l476_flash_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_flash_init(Object *obj)
{
    STM32L476FlashState *s = STM32L476_FLASH(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_flash_ops, s,
                          TYPE_STM32L476_FLASH, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32l476_flash_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_flash_reset;
}

static const TypeInfo stm32l476_flash_info = {
    .name = TYPE_STM32L476_FLASH,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476FlashState),
    .instance_init = stm32l476_flash_init,
    .class_init = stm32l476_flash_class_init,
};

static void stm32l476_flash_register_types(void)
{
    type_register_static(&stm32l476_flash_info);
}

type_init(stm32l476_flash_register_types)
