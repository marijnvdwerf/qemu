
#include "qemu/osdep.h"
#include "hw/misc/unimp.h"
#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/loader.h"
#include "qemu/log.h"

#include "hw/arm/stm32l467_soc.h"

static uint64_t mv88w8618_wlan_read(void *opaque, hwaddr offset,
                                    unsigned size) {
    printf("read @ 0x%x\n", offset);
    return 0;
}

static void mv88w8618_wlan_write(void *opaque, hwaddr offset,
                                 uint64_t value, unsigned size) {
    printf("write @ 0x%x\n", offset);
}

static const MemoryRegionOps mv88w8618_wlan_ops = {
    .read = mv88w8618_wlan_read,
    .write =mv88w8618_wlan_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static inline void create_unimplemented_layer(const char *name, hwaddr base, hwaddr size) {
    DeviceState *dev = qdev_create(NULL, TYPE_UNIMPLEMENTED_DEVICE);

    qdev_prop_set_string(dev, "name", name);
    qdev_prop_set_uint64(dev, "size", size);
    qdev_init_nofail(dev);

    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(dev), 0, base, -900);
}

static void stm32l467_soc_initfn(Object *obj) {
    STM32L467State *s = STM32L467_SOC(obj);

    sysbus_init_child_obj(obj, "armv7m", &s->armv7m, sizeof(s->armv7m),
                          TYPE_ARMV7M);

}

static void stm32l467_soc_realize(DeviceState *dev_soc, Error **errp) {
    STM32L467State *s = STM32L467_SOC(dev_soc);
    Error *err = NULL;

    MemoryRegion *system_memory = get_system_memory();

    create_unimplemented_layer("IO", 0, 0xFFFFFFFF);

    memory_region_init_rom(&s->flash, OBJECT(dev_soc), "STM32L467.flash",
                           FLASH_SIZE, &error_fatal);

    memory_region_init_alias(&s->flash_alias,
                             OBJECT(dev_soc),
                             "alias",
                             &s->flash,
                             0,
                             FLASH_SIZE);
    memory_region_add_subregion(system_memory, 0, &s->flash);
    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, &s->flash_alias);

    memory_region_init_ram(&s->sram1, OBJECT(dev_soc), "STM32F205.sram1", 96 * 1024, &error_fatal);
    memory_region_add_subregion(system_memory, 0x20000000, &s->sram1);

    memory_region_init_ram(&s->sram2, OBJECT(dev_soc), "STM32F205.sram2", 32 * 1024, &error_fatal);
    memory_region_add_subregion(system_memory, 0x10000000, &s->sram2);

    DeviceState *armv7m = DEVICE(&s->armv7m);
    qdev_prop_set_string(armv7m, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m4"));
    object_property_set_link(OBJECT(&s->armv7m), OBJECT(system_memory),
                             "memory", &error_abort);
    object_property_set_bool(OBJECT(&s->armv7m), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    system_clock_scale = 1000;
}

static Property stm32l467_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", STM32L467State, cpu_type),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32l467_soc_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = stm32l467_soc_realize;
    device_class_set_props(dc, stm32l467_soc_properties);
}

static const TypeInfo stm32l467_soc_info = {
    .name          = TYPE_STM32L467_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L467State),
    .instance_init = stm32l467_soc_initfn,
    .class_init    = stm32l467_soc_class_init,
};

static void stm32l467_soc_types(void) {
    type_register_static(&stm32l467_soc_info);
}

type_init(stm32l467_soc_types)