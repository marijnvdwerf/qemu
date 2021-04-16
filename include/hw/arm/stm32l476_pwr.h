#ifndef HW_ARM_STM32L476_PWR_H
#define HW_ARM_STM32L476_PWR_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_PWR "stm32l476-pwr"
#define STM32L476_PWR(obj) \
    OBJECT_CHECK(STM32L476PwrState, (obj), TYPE_STM32L476_PWR)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

    // CR1
    uint8_t LPMS;
    bool DBP;
    uint8_t VOS;
    bool LPR;

    // SR2
    bool REGLPS;
    bool REGLPF;
    bool VOSF;
    bool PVDO;
    bool PVMO1;
    bool PVMO2;
    bool PVMO3;
    bool PVMO4;
} STM32L476PwrState;

#endif
