#ifndef HW_ARM_STM32L476_FLASH_H
#define HW_ARM_STM32L476_FLASH_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_FLASH "stm32l476-flash"
#define STM32L476_FLASH(obj) \
    OBJECT_CHECK(STM32L476FlashState, (obj), TYPE_STM32L476_FLASH)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    uint8_t acr_latency;
} STM32L476FlashState;

#endif
