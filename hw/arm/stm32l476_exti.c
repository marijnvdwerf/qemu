#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"

#include "hw/arm/stm32l476_rcc.h"

static void stm32l476_rcc_reset(DeviceState *dev) {
    STM32L476RccState *s = STM32L476_RCC(dev);
}

static uint64_t stm32l476_rcc_read(void *opaque, hwaddr offset,
                                   unsigned int size) {
    STM32L476RccState *s = opaque;

    switch (offset) {
        default:
            qemu_log_mask(LOG_UNIMP, "%s: unimplemented device read "
                                     "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                          __func__, size, offset);
            break;
    }
    return 0;
}

static void stm32l476_rcc_write(void *opaque, hwaddr offset,
                                uint64_t val64, unsigned int size) {
    STM32L476RccState *s = opaque;

    switch (offset) {
        default:
            qemu_log_mask(LOG_UNIMP, "%s: unimplemented device write "
                                     "(size %d, value 0x%" PRIx64
                ", offset 0x%" HWADDR_PRIx ")\n",
                          __func__, size, val64, offset);
            break;
    }
}

static const MemoryRegionOps stm32l476_rcc_ops = {
    .read = stm32l476_rcc_read,
    .write = stm32l476_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_rcc_init(Object *obj) {
    STM32L476RccState *s = STM32L476_RCC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_rcc_ops, s,
                          TYPE_STM32L476_RCC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32l476_rcc_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_rcc_reset;
}

static const TypeInfo stm32l476_rcc_info = {
    .name          = TYPE_STM32L476_RCC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476RccState),
    .instance_init = stm32l476_rcc_init,
    .class_init    = stm32l476_rcc_class_init,
};

static void stm32l476_rcc_register_types(void) {
    type_register_static(&stm32l476_rcc_info);
}

type_init(stm32l476_rcc_register_types)
