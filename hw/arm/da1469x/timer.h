

#include "hw/sysbus.h"
#include "qemu/timer.h"

#define TYPE_DA1469X_TIMER "da1469x-timer"
#define DA1469XTIMER(obj) OBJECT_CHECK(DA1469xTimerState, \
                            (obj), TYPE_DA1469X_TIMER)

typedef struct  {
  SysBusDevice parent_obj;

  QEMUTimer *timer;
  qemu_irq irq;
  MemoryRegion iomem;

  uint32_t gpio1_conf;
  uint32_t gpio2_conf;
  uint32_t reload;
  uint32_t prescaler;
  uint32_t en;
  uint32_t CLK_EN;
  uint32_t FREE_RUN_MODE_EN;
  uint32_t SYS_CLK_EN;
  uint32_t IN2_EVENT_FALL_EN;
  uint32_t IN1_EVENT_FALL_EN;
  uint32_t oneshot_mode;
  uint32_t count_down;
  uint32_t IRQ_EN;

  int64_t hit_time;
} DA1469xTimerState;