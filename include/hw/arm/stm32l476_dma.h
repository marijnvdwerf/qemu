#ifndef HW_ARM_STM32L476_DMA_H
#define HW_ARM_STM32L476_DMA_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_DMA "stm32l476-dma"
#define STM32L476_DMA(obj) \
    OBJECT_CHECK(STM32L476DmaState, (obj), TYPE_STM32L476_DMA)


typedef struct STM32L476DmaChannelState {
    uint8_t CxS;
    uint16_t NDTR;
    uint32_t PA;
    uint32_t MA;
} STM32L476DmaChannelState;

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    qemu_irq irq;
    STM32L476DmaChannelState channels[7];

} STM32L476DmaState;

#endif
