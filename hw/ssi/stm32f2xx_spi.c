<<<<<<< HEAD
/*
 * STM32F405 SPI
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
=======
/*-
 * Copyright (c) 2013
>>>>>>> 919b29ba7d... Pebble Qemu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
<<<<<<< HEAD
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/ssi/stm32f2xx_spi.h"
#include "migration/vmstate.h"

#ifndef STM_SPI_ERR_DEBUG
#define STM_SPI_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_SPI_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

static void stm32f2xx_spi_reset(DeviceState *dev)
{
    STM32F2XXSPIState *s = STM32F2XX_SPI(dev);

    s->spi_cr1 = 0x00000000;
    s->spi_cr2 = 0x00000000;
    s->spi_sr = 0x0000000A;
    s->spi_dr = 0x0000000C;
    s->spi_crcpr = 0x00000007;
    s->spi_rxcrcr = 0x00000000;
    s->spi_txcrcr = 0x00000000;
    s->spi_i2scfgr = 0x00000000;
    s->spi_i2spr = 0x00000002;
}

static void stm32f2xx_spi_transfer(STM32F2XXSPIState *s)
{
    DB_PRINT("Data to send: 0x%x\n", s->spi_dr);

    s->spi_dr = ssi_transfer(s->ssi, s->spi_dr);
    s->spi_sr |= STM_SPI_SR_RXNE;

    DB_PRINT("Data received: 0x%x\n", s->spi_dr);
}

static uint64_t stm32f2xx_spi_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F2XXSPIState *s = opaque;

    DB_PRINT("Address: 0x%" HWADDR_PRIx "\n", addr);

    switch (addr) {
    case STM_SPI_CR1:
        return s->spi_cr1;
    case STM_SPI_CR2:
        qemu_log_mask(LOG_UNIMP, "%s: Interrupts and DMA are not implemented\n",
                      __func__);
        return s->spi_cr2;
    case STM_SPI_SR:
        return s->spi_sr;
    case STM_SPI_DR:
        stm32f2xx_spi_transfer(s);
        s->spi_sr &= ~STM_SPI_SR_RXNE;
        return s->spi_dr;
    case STM_SPI_CRCPR:
        qemu_log_mask(LOG_UNIMP, "%s: CRC is not implemented, the registers " \
                      "are included for compatibility\n", __func__);
        return s->spi_crcpr;
    case STM_SPI_RXCRCR:
        qemu_log_mask(LOG_UNIMP, "%s: CRC is not implemented, the registers " \
                      "are included for compatibility\n", __func__);
        return s->spi_rxcrcr;
    case STM_SPI_TXCRCR:
        qemu_log_mask(LOG_UNIMP, "%s: CRC is not implemented, the registers " \
                      "are included for compatibility\n", __func__);
        return s->spi_txcrcr;
    case STM_SPI_I2SCFGR:
        qemu_log_mask(LOG_UNIMP, "%s: I2S is not implemented, the registers " \
                      "are included for compatibility\n", __func__);
        return s->spi_i2scfgr;
    case STM_SPI_I2SPR:
        qemu_log_mask(LOG_UNIMP, "%s: I2S is not implemented, the registers " \
                      "are included for compatibility\n", __func__);
        return s->spi_i2spr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }

    return 0;
}

static void stm32f2xx_spi_write(void *opaque, hwaddr addr,
                                uint64_t val64, unsigned int size)
{
    STM32F2XXSPIState *s = opaque;
    uint32_t value = val64;

    DB_PRINT("Address: 0x%" HWADDR_PRIx ", Value: 0x%x\n", addr, value);

    switch (addr) {
    case STM_SPI_CR1:
        s->spi_cr1 = value;
        return;
    case STM_SPI_CR2:
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "Interrupts and DMA are not implemented\n", __func__);
        s->spi_cr2 = value;
        return;
    case STM_SPI_SR:
        /* Read only register, except for clearing the CRCERR bit, which
         * is not supported
         */
        return;
    case STM_SPI_DR:
        s->spi_dr = value;
        stm32f2xx_spi_transfer(s);
        return;
    case STM_SPI_CRCPR:
        qemu_log_mask(LOG_UNIMP, "%s: CRC is not implemented\n", __func__);
        return;
    case STM_SPI_RXCRCR:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Read only register: " \
                      "0x%" HWADDR_PRIx "\n", __func__, addr);
        return;
    case STM_SPI_TXCRCR:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Read only register: " \
                      "0x%" HWADDR_PRIx "\n", __func__, addr);
        return;
    case STM_SPI_I2SCFGR:
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "I2S is not implemented\n", __func__);
        return;
    case STM_SPI_I2SPR:
        qemu_log_mask(LOG_UNIMP, "%s: " \
                      "I2S is not implemented\n", __func__);
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%" HWADDR_PRIx "\n", __func__, addr);
=======
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * QEMU model of the stm32f2xx SPI controller.
 */

#include "hw/sysbus.h"
#include "hw/arm/stm32.h"
#include "hw/ssi.h"

#define	R_CR1             (0x00 / 4)
#define	R_CR1_DFF      (1 << 11)
#define	R_CR1_LSBFIRST (1 <<  7)
#define	R_CR1_SPE      (1 <<  6)
#define	R_CR2             (0x04 / 4)

#define	R_SR       (0x08 / 4)
#define	R_SR_RESET    0x0002
#define	R_SR_MASK     0x01FF
#define R_SR_OVR     (1 << 6)
#define R_SR_TXE     (1 << 1)
#define R_SR_RXNE    (1 << 0)

#define	R_DR       (0x0C / 4)
#define	R_CRCPR    (0x10 / 4)
#define	R_CRCPR_RESET 0x0007
#define	R_RXCRCR   (0x14 / 4)
#define	R_TXCRCR   (0x18 / 4)
#define	R_I2SCFGR  (0x1C / 4)
#define	R_I2SPR    (0x20 / 4)
#define	R_I2SPR_RESET 0x0002
#define R_MAX      (0x24 / 4)

typedef struct stm32f2xx_spi_s {
    SysBusDevice busdev;
    MemoryRegion iomem;
    qemu_irq irq;

    SSIBus *spi;

    stm32_periph_t periph;

    int32_t rx;
    int rx_full; 
    uint16_t regs[R_MAX];
} Stm32Spi;

static uint64_t
stm32f2xx_spi_read(void *arg, hwaddr offset, unsigned size)
{
    Stm32Spi *s = arg;
    uint16_t r = UINT16_MAX;

    if (!(size == 1 || size == 2 || size == 4 || (offset & 0x3) != 0)) {
        STM32_BAD_REG(offset, size);
    }
    offset >>= 2;
    if (offset < R_MAX) {
        r = s->regs[offset];
    } else {
        stm32_hw_warn("Out of range SPI write, offset 0x%x", (unsigned)offset<<2);
    }
    switch (offset) {
    case R_DR:
        s->regs[R_SR] &= ~R_SR_RXNE;
    }
    return r;
}

static uint8_t
bitswap(uint8_t val)
{
    return ((val * 0x0802LU & 0x22110LU) | (val * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static void
stm32f2xx_spi_write(void *arg, hwaddr addr, uint64_t data, unsigned size)
{
    struct stm32f2xx_spi_s *s = (struct stm32f2xx_spi_s *)arg;
    int offset = addr & 0x3;

    /* SPI registers are all at most 16 bits wide */
    data &= 0xFFFFF;
    addr >>= 2;

    switch (size) {
        case 1:
            data = (s->regs[addr] & ~(0xff << (offset * 8))) | data << (offset * 8);
            break;
        case 2:
            data = (s->regs[addr] & ~(0xffff << (offset * 8))) | data << (offset * 8);
            break;
        case 4:
            break;
        default:
            abort();
    }

    switch (addr) {
    case R_CR1:
        if ((data & R_CR1_DFF) != s->regs[R_CR1] && (s->regs[R_CR1] & R_CR1_SPE) != 0)
            qemu_log_mask(LOG_GUEST_ERROR, "cannot change DFF with SPE set\n");
        if (data & R_CR1_DFF)
            qemu_log_mask(LOG_UNIMP, "f2xx DFF 16-bit mode not implemented\n");
        s->regs[R_CR1] = data;
        break;
    case R_DR:
        s->regs[R_SR] &= ~R_SR_TXE;
        if (s->regs[R_SR] & R_SR_RXNE) {
            s->regs[R_SR] |= R_SR_OVR;
        }
        if (s->regs[R_CR1] & R_CR1_LSBFIRST) {
            s->regs[R_DR] = bitswap(ssi_transfer(s->spi, bitswap(data)));
        } else {
            s->regs[R_DR] = ssi_transfer(s->spi, data);
        }
        
        s->regs[R_SR] |= R_SR_RXNE;
        s->regs[R_SR] |= R_SR_TXE;
        break;
    default:
        if (addr < ARRAY_SIZE(s->regs)) {
            s->regs[addr] = data;
        } else {
            STM32_BAD_REG(addr, size);
        }
>>>>>>> 919b29ba7d... Pebble Qemu
    }
}

static const MemoryRegionOps stm32f2xx_spi_ops = {
    .read = stm32f2xx_spi_read,
    .write = stm32f2xx_spi_write,
<<<<<<< HEAD
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_stm32f2xx_spi = {
    .name = TYPE_STM32F2XX_SPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(spi_cr1, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_cr2, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_sr, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_dr, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_crcpr, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_rxcrcr, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_txcrcr, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_i2scfgr, STM32F2XXSPIState),
        VMSTATE_UINT32(spi_i2spr, STM32F2XXSPIState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f2xx_spi_init(Object *obj)
{
    STM32F2XXSPIState *s = STM32F2XX_SPI(obj);
    DeviceState *dev = DEVICE(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f2xx_spi_ops, s,
                          TYPE_STM32F2XX_SPI, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    s->ssi = ssi_create_bus(dev, "ssi");
}

static void stm32f2xx_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f2xx_spi_reset;
    dc->vmsd = &vmstate_stm32f2xx_spi;
}

static const TypeInfo stm32f2xx_spi_info = {
    .name          = TYPE_STM32F2XX_SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F2XXSPIState),
    .instance_init = stm32f2xx_spi_init,
    .class_init    = stm32f2xx_spi_class_init,
};

static void stm32f2xx_spi_register_types(void)
=======
    .endianness = DEVICE_NATIVE_ENDIAN
};

static void
stm32f2xx_spi_reset(DeviceState *dev)
{
    struct stm32f2xx_spi_s *s = FROM_SYSBUS(struct stm32f2xx_spi_s,
      SYS_BUS_DEVICE(dev));

    s->regs[R_SR] = R_SR_RESET;
    switch (s->periph) {
    case 0:
        break;
    case 1:
        break;
    default:
        break;
    }
}

static int
stm32f2xx_spi_init(SysBusDevice *dev)
{
    struct stm32f2xx_spi_s *s = FROM_SYSBUS(struct stm32f2xx_spi_s, dev);

    memory_region_init_io(&s->iomem, NULL, &stm32f2xx_spi_ops, s, "spi", 0x3ff);
    sysbus_init_mmio(dev, &s->iomem);
    sysbus_init_irq(dev, &s->irq);
    s->spi = ssi_create_bus(DEVICE(dev), "ssi");

    return 0;
}


static Property stm32f2xx_spi_properties[] = {
    DEFINE_PROP_INT32("periph", struct stm32f2xx_spi_s, periph, -1),
    DEFINE_PROP_END_OF_LIST()
};

static void
stm32f2xx_spi_class_init(ObjectClass *c, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(c);
    SysBusDeviceClass *sc = SYS_BUS_DEVICE_CLASS(c);

    sc->init = stm32f2xx_spi_init;
    dc->reset = stm32f2xx_spi_reset;
    dc->props = stm32f2xx_spi_properties;
}

static const TypeInfo stm32f2xx_spi_info = {
    .name = "stm32f2xx_spi",
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct stm32f2xx_spi_s),
    .class_init = stm32f2xx_spi_class_init
};

static void
stm32f2xx_spi_register_types(void)
>>>>>>> 919b29ba7d... Pebble Qemu
{
    type_register_static(&stm32f2xx_spi_info);
}

type_init(stm32f2xx_spi_register_types)
<<<<<<< HEAD
=======

/*
 *
 */
/*
 * Serial peripheral interface (SPI) RM0033 section 25
 */

>>>>>>> 919b29ba7d... Pebble Qemu
