#include "qemu/osdep.h"
#include "hw/arm/stm32l476_rcc.h"
#include "exec/address-spaces.h"
#include "hw/arm/boot.h"
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "sysemu/sysemu.h"

#define RCC_CR 0x00
#define RCC_ICSCR 0x04
#define CFGR 0x08
#define RCC_PLLCFGR 0x0C
#define PLLSAI1CFGR 0x10
#define PLLSAI2CFGR 0x14
#define RCC_CIER 0x18
#define CIFR 0x1C
#define CICR 0x20
#define RESERVED0 0x24
#define AHB1RSTR 0x28
#define AHB2RSTR 0x2C
#define AHB3RSTR 0x30
#define RESERVED1 0x34
#define RCC_APB1RSTR1 0x38
#define APB1RSTR2 0x3C
#define APB2RSTR 0x40
#define RESERVED2 0x44
#define AHB1ENR 0x48
#define AHB2ENR 0x4C
#define AHB3ENR 0x50
#define RESERVED3 0x54
#define RCC_APB1ENR1 0x58
#define APB1ENR2 0x5C
#define RCC_APB2ENR 0x60
#define RESERVED4 0x64
#define AHB1SMENR 0x68
#define AHB2SMENR 0x6C
#define AHB3SMENR 0x70
#define RESERVED5 0x74
#define APB1SMENR1 0x78
#define APB1SMENR2 0x7C
#define APB2SMENR 0x80
#define RESERVED6 0x84
#define RCC_CCIPR 0x88
#define RESERVED7 0x8C
#define BDCR 0x90
#define CSR 0x94

static void stm32l476_rcc_reset(DeviceState *dev)
{
    STM32L476RccState *s = STM32L476_RCC(dev);
    s->acr_latency = 0;

    s->LSEON = false;
    s->LSEBYP = false;
}

static uint64_t stm32l476_rcc_read(void *opaque, hwaddr offset,
                                   unsigned int size)
{
    STM32L476RccState *s = opaque;

    switch (offset) {
    case RCC_CR: {
        uint32_t out = 0;
        out |= 1 << 17; /* HSERDY */
        out |= 1 << 16; /* HSEON */
        out |= 1 << 10; /* HSIRDY */
        out |= 1 << 8;  /* HSION */
        out |= 1 << 1;  /* MSIRDY */
        return out;
    }

    case CFGR: {
        uint32_t out = 0;
        out |= s->SW << 2;
        return out;
    }

    case BDCR: {
        uint32_t out = 0;
        out |= s->LSEON << 0;
        if (s->LSEON) {
            out |= 0b1 << 1;
        }
        out |= s->LSEBYP << 2;
        return out;
    }

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented device read "
                      "(size %d, offset 0x%" HWADDR_PRIx ")\n",
                      __func__, size, offset);
        break;
    }
    return 0;
}

static void stm32l476_rcc_write(void *opaque, hwaddr offset, uint64_t val64,
                                unsigned int size)
{
    STM32L476RccState *s = opaque;

    switch (offset) {
    case RCC_CR:
    case RCC_ICSCR:
    case RCC_PLLCFGR:
    case RCC_CIER:
    case RCC_APB1RSTR1:
    case RCC_APB1ENR1:
    case RCC_APB2ENR:
        break;
    case RCC_CCIPR: {
        printf("CCIPR: 0x%08X\n", val64);
        break;
    }

    case CFGR: {
        s->SW = (val64 >> 0) & 0b11;
        break;
    }

    case BDCR: {
        s->LSEON = (val64 >> 0) & 0b1;
        s->LSEBYP = (val64 >> 2) & 0b1;
        break;
    }

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented device write "
                      "(size %d, value 0x%" PRIx64 ", offset 0x%" HWADDR_PRIx
                      ")\n",
                      __func__, size, val64, offset);
        break;
    }
}

static const MemoryRegionOps stm32l476_rcc_ops = {
    .read = stm32l476_rcc_read,
    .write = stm32l476_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32l476_rcc_init(Object *obj)
{
    STM32L476RccState *s = STM32L476_RCC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32l476_rcc_ops, s,
                          TYPE_STM32L476_RCC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void stm32l476_rcc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32l476_rcc_reset;
}

static const TypeInfo stm32l476_rcc_info = {
    .name = TYPE_STM32L476_RCC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L476RccState),
    .instance_init = stm32l476_rcc_init,
    .class_init = stm32l476_rcc_class_init,
};

static void stm32l476_rcc_register_types(void)
{
    type_register_static(&stm32l476_rcc_info);
}

type_init(stm32l476_rcc_register_types)
