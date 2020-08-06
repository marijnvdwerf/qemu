#ifndef HW_ARM_STM32F205_SOC_H
#define HW_ARM_STM32F205_SOC_H

#include "hw/arm/da1469x/qspi.h"
#include "hw/arm/da1469x/timer.h"

#define TYPE_DA1469X_SOC "da1469x-soc"
#define DA1469X_SOC(obj)                                       \
    OBJECT_CHECK(DA1469xState, obj, TYPE_DA1469X_SOC)

#define DA1469X_NUM_TIMERS 4

typedef struct {
  SysBusDevice parent_obj;

  ARMv7MState armv7m;
  MemoryRegion flash;
  MemoryRegion sysram;

  MemoryRegion crg_aon;
  MemoryRegion sdadc;
  MemoryRegion crg_xtal;
  MemoryRegion otpc_c;

  DA1469xQspiState qspi;
  DA1469xTimerState timer[DA1469X_NUM_TIMERS];
} DA1469xState;

#endif
