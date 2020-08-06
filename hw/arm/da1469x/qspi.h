#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"

#define TYPE_DA1469X_QSPI "da1469x.qspi"
#define DA1469XQSPI(obj) OBJECT_CHECK(DA1469xQspiState, \
                            (obj), TYPE_DA1469X_QSPI)

typedef struct {
  SysBusDevice parent_obj;
  MemoryRegion control;
  MemoryRegion iomem;
  SSIBus *spi;
  qemu_irq _irq;

  int AUTO_MD;
  int CLK_MD;
  int IO2_OEN;
  int IO3_OEN;
  int IO2_DAT;
  int IO3_DAT;
  int HRDY_MD;

  int RPIPE_EN;
  int RXD_NEG;
  int PCLK_MD;
  int BUF_LIM_EN;
  int USE_32BA;

  bool cs;
  qemu_irq cs_line;
} DA1469xQspiState;
