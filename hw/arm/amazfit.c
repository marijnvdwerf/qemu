
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/arm/stm32l467_soc.h"
#include "hw/arm/boot.h"
#include "hw/loader.h"

static uint32_t readu32(hwaddr addr) {
    uint32_t ret;
    ARMCPU *cpu = ARM_CPU(first_cpu);
    cpu_memory_rw_debug(cpu, addr, &ret, 4, false);

    return ret;
}

static void bip_init(MachineState *machine) {
    DeviceState *dev;

    dev = qdev_create(NULL, TYPE_STM32L467_SOC);
    object_property_set_bool(OBJECT(dev), true, "realized", &error_fatal);

    load_image_targphys("/Users/Marijn/Downloads/bip/image.bin",
                        0,
                        FLASH_SIZE);

    armv7m_load_kernel(ARM_CPU(first_cpu), NULL, 0);
}

static void bip_machine_init(MachineClass *mc) {
    mc->desc = "Amazfit Bip";
    mc->init = bip_init;
}

DEFINE_MACHINE("amazfitbip", bip_machine_init)
