#ifndef HW_ARM_STM32L467_SOC_H
#define HW_ARM_STM32L467_SOC_H

#include "hw/arm/armv7m.h"
#include "hw/arm/stm32l476_flash.h"
#include "hw/arm/stm32l476_lptim.h"
#include "hw/arm/stm32l476_pwr.h"
#include "hw/arm/stm32l476_rcc.h"
#include "hw/arm/stm32l476_dma.h"
#include "hw/arm/stm32l476_spi.h"
#include "hw/arm/stm32l476_i2c.h"
#include "hw/arm/stm32l476_qspi.h"

#define TYPE_STM32L467_SOC "stm32l467-soc"
#define STM32L467_SOC(obj) \
    OBJECT_CHECK(STM32L467State, (obj), TYPE_STM32L467_SOC)

#define FLASH_BASE_ADDRESS 0x8000000
#define FLASH_SIZE (1024 * 1024)


typedef struct STM32L467State {
    SysBusDevice parent_obj;

    char *cpu_type;

    ARMv7MState armv7m;
    STM32L476PwrState pwr;
    STM32L476LPTimState lptim1;
    STM32L476RccState rcc;
    STM32L476FlashState flash_r;
    STM32L476DmaState dma;
    STM32L476SpiState spi[3];
    STM32L476QspiState qspi;
    STM32L476I2CState i2c[3];
//    STM32L476TimState tim2;
    MemoryRegion flash_alias;
    MemoryRegion flash;
    MemoryRegion sram1;
    MemoryRegion sram2;
    DeviceState* gpio[8];
    DeviceState *syscfg;
} STM32L467State;

#endif
