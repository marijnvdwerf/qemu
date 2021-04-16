
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/arm/stm32l467_soc.h"
#include "hw/arm/boot.h"
#include "hw/loader.h"
#include "hw/ssi/ssi.h"
#include "hw/i2c/i2c.h"

static uint32_t readu32(hwaddr addr) {
    uint32_t ret;
    ARMCPU *cpu = ARM_CPU(first_cpu);
    cpu_memory_rw_debug(cpu, addr, &ret, 4, false);

    return ret;
}

static void bip_init(MachineState *machine) {

    STM32L467State *dev = STM32L467_SOC(qdev_create(NULL, TYPE_STM32L467_SOC));
    object_property_set_bool(OBJECT(dev), true, "realized", &error_fatal);

    SSIBus* spi_bus = (SSIBus *)qdev_get_child_bus(&dev->spi[2], "ssi"); // SPI3
    DeviceState * display = ssi_create_slave_no_init(spi_bus, "jdi-lpm013m126c");
    qdev_init_nofail(display);
    
    /* --- QSPI Flash ---------------------------------------------  */
    SSIBus * qspi = (SSIBus *)qdev_get_child_bus(&dev->qspi, "qspi");
    DeviceState *qspi_flash = ssi_create_slave_no_init(qspi, "w25q64fw");

    DriveInfo *dinfo = drive_get(IF_MTD, 0, 0);
    BlockBackend *blk = dinfo ? blk_by_legacy_dinfo(dinfo) : NULL;
    qdev_prop_set_drive(qspi_flash, "drive", blk, &error_fatal);
    qdev_init_nofail(qspi_flash);

    qemu_irq mx25u_cs = qdev_get_gpio_in_named(qspi_flash, SSI_GPIO_CS, 0);
    sysbus_connect_irq(SYS_BUS_DEVICE(&dev->qspi), 1, mx25u_cs);

    load_image_targphys("/Users/Marijn/Downloads/bip/image.bin",
//    load_image_targphys("/Users/Marijn/Projects/_pebble/Amazfitbip-FreeRTOS/bin/lcd_test.bin",
                        0,
                        FLASH_SIZE);

    I2CBus *i2c = (I2CBus *) qdev_get_child_bus(&dev->i2c[0], "i2c");
    i2c_create_slave(i2c, "it7259", 0x46);

    armv7m_load_kernel(ARM_CPU(first_cpu), NULL, 0);
}

static void bip_machine_init(MachineClass *mc) {
    mc->desc = "Amazfit Bip";
    mc->init = bip_init;
}

DEFINE_MACHINE("amazfitbip", bip_machine_init)
