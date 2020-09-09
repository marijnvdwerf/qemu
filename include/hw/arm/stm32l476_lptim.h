#ifndef HW_ARM_STM32L476_LPTIM_H
#define HW_ARM_STM32L476_LPTIM_H

#include "hw/arm/armv7m.h"
#include "hw/ptimer.h"

#define TYPE_STM32L476_LPTIM "stm32l476-lptim"
#define STM32L476_LPTIM(obj) \
    OBJECT_CHECK(STM32L476LPTimState, (obj), TYPE_STM32L476_LPTIM)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    qemu_irq irq;

    int64_t start;
    QEMUTimer *timer;
    ptimer_state * ptimer;

    // ISR
    bool CMPM;
    bool ARRM;
    bool EXTTRIG;
    bool CMPOK;
    bool ARROK;
    bool UP;
    bool DOWN;

    // IER:
    bool CMPMIE;
    bool ARRMIE;
    bool EXTTRIGIE;
    bool CMPOKIE;
    bool ARROKIE;
    bool UPIE;
    bool DOWNIE;

    // CR
    bool ENABLE;
    bool SNGSTRT;
    bool CNTSTRT;

    uint16_t CNT;
    uint16_t ARR;
} STM32L476LPTimState;

#endif
