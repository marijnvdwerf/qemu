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
#include "hw/arm/da1469x/soc.h"
#include "qemu/log.h"

static void bip_s_init(MachineState *machine) {
    DeviceState *dev;
    dev = qdev_create(NULL, TYPE_DA1469X_SOC);
    object_property_set_bool(OBJECT(dev), true, "realized", &error_fatal);

    load_image_targphys("/Users/Marijn/Downloads/tonlesap_202006191826_2.1.1.16_tonlesap.img", 0x0, 0x8192 * 1024);
    load_image_targphys("otp.bin", 0x10080000, 64 * 1024);

    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename,
                       0x8192 * 1024);
}

static void bip_s_machine_class_init(MachineClass *mc)
{
    mc->desc = "Amazfit Bip S";
    mc->init = bip_s_init;
    mc->ignore_memory_transaction_failures = true;
}

DEFINE_MACHINE("bip-s", bip_s_machine_class_init)
