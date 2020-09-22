#ifndef HW_ARM_STM32L476_DMA_H
#define HW_ARM_STM32L476_DMA_H

#include "hw/arm/armv7m.h"

#define TYPE_STM32L476_DMA "stm32l476-dma"
#define STM32L476_DMA(obj) \
    OBJECT_CHECK(STM32L476DmaState, (obj), TYPE_STM32L476_DMA)

typedef struct _STM32L476DmaState STM32L476DmaState;

typedef struct STM32L476DmaChannelState {
    STM32L476DmaState *parent;
    uint8_t CxS;
    uint16_t NDTR;
    uint32_t PA;
    uint32_t MA;
    qemu_irq *drq;
    bool EN;
    bool TCIE;
    bool HTIE;
    bool TEIE;
    bool CIRC;
    bool PINC;
    bool MINC;
    bool MEM2MEM;
    uint64_t DIR;
    uint64_t PSIZE;
    uint64_t MSIZE;
    uint64_t PL;
    int id;
    bool TCIF;
    bool HTIF;
    bool TEIF;
} STM32L476DmaChannelState;

struct _STM32L476DmaState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    qemu_irq irq[7];
    STM32L476DmaChannelState channels[7];

};

#endif
