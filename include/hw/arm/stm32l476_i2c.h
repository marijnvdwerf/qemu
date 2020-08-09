#ifndef HW_ARM_STM32L476_I2C_H
#define HW_ARM_STM32L476_I2C_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_I2C "stm32l476-i2c"
#define STM32L476_I2C(obj) \
    OBJECT_CHECK(STM32L476I2CState, (obj), TYPE_STM32L476_I2C)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;
    char *name;

    /* <public> */
    MemoryRegion mmio;
    qemu_irq irq;

    // I2C_CR2
    uint64_t SADD;
    uint64_t RD_WRN;
    uint64_t ADD10;
    uint64_t HEAD10R;
    bool START;
    bool STOP;
    bool NACK;
    uint64_t NBYTES;
    uint64_t RELOAD;
    uint64_t AUTOEND;
    bool PECBYTE;

    // I2C_ISR
    bool TXIS;
    bool BUSY;
    I2CBus *bus;
} STM32L476I2CState;

#endif
