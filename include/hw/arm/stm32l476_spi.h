#ifndef HW_ARM_STM32L476_SPI_H
#define HW_ARM_STM32L476_SPI_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_SPI "stm32l476-spi"
#define STM32L476_SPI(obj) \
    OBJECT_CHECK(STM32L476SpiState, (obj), TYPE_STM32L476_SPI)


typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    qemu_irq irq;

    SSIBus *spi;

} STM32L476SpiState;

#endif
