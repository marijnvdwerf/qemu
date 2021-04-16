#ifndef HW_ARM_STM32L476_RCC_H
#define HW_ARM_STM32L476_RCC_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_RCC "stm32l476-rcc"
#define STM32L476_RCC(obj) \
    OBJECT_CHECK(STM32L476RccState, (obj), TYPE_STM32L476_RCC)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    uint8_t acr_latency;

    // CFGR
    uint8_t SW;

    // BDCR
    bool LSEON;
    bool LSEBYP;
} STM32L476RccState;

#endif
