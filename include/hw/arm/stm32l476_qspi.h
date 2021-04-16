#ifndef HW_ARM_STM32L476_QSPI_H
#define HW_ARM_STM32L476_QSPI_H

#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"
#include "qemu/fifo8.h"

#define TYPE_STM32L476_QSPI "stm32l476-qspi"
#define STM32L476_QSPI(obj) \
    OBJECT_CHECK(STM32L476QspiState, (obj), TYPE_STM32L476_QSPI)

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    MemoryRegion cache;

    bool EN;
    bool ABORT;

    bool TCF;
    bool TEF;
    bool SMF;
    bool TOF;

    uint8_t PMM;
    uint8_t APMS;

    uint32_t DL;
    uint8_t INSTRUCTION;
    uint64_t IMODE;
    uint64_t ADMODE;
    uint64_t ADSIZE;
    uint64_t ABMODE;
    uint64_t ABSIZE;
    uint64_t DCYC;
    uint64_t DMODE;
    uint64_t FMODE;
    uint32_t MASK;
    uint32_t MATCH;
    uint16_t INTERVAL;

    uint32_t ADDRESS;
    bool BUSY;

    qemu_irq irq;
    SSIBus *qspi;
    qemu_irq cs;

    Fifo8 rx_fifo;
    int dataPushed;
} STM32L476QspiState;

#endif
