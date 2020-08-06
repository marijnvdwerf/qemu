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
#include "hw/ssi/ssi.h"

static void bip_s_init(MachineState *machine) {
    DeviceState *dev;
    dev = qdev_create(NULL, TYPE_DA1469X_SOC);
    object_property_set_bool(OBJECT(dev), true, "realized", &error_fatal);

    DA1469xState *soc = DA1469X_SOC(dev);

    /* --- QSPI Flash ---------------------------------------------  */
    SSIBus *qspi = (SSIBus *) qdev_get_child_bus(DEVICE(&soc->qspi), "spi");
    DeviceState *qspi_flash = ssi_create_slave_no_init(qspi, "GD25LQ64C");
    DriveInfo *dinfo = drive_get(IF_PFLASH, 0, 0);
    if (dinfo) {
        qdev_prop_set_drive(qspi_flash, "drive", blk_by_legacy_dinfo(dinfo), &error_fatal);
    }
    qdev_init_nofail(qspi_flash);

    qemu_irq cs_line = qdev_get_gpio_in_named(qspi_flash, SSI_GPIO_CS, 0);
    sysbus_connect_irq(SYS_BUS_DEVICE(&soc->qspi), 1, cs_line);

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
