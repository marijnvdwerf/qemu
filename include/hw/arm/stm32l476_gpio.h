#ifndef HW_ARM_STM32L476_GPIO_H
#define HW_ARM_STM32L476_GPIO_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_GPIO "stm32l476-gpio"
#define STM32L476_GPIO(obj) \
    OBJECT_CHECK(STM32L476GpioState, (obj), TYPE_STM32L476_GPIO)

typedef struct __STM32L476GpioState STM32L476GpioState;
static const int GPIOx_AFRL = 0x20;
#endif
