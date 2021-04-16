
#include "qemu/osdep.h"
#include "hw/misc/unimp.h"
#include "hw/sysbus.h"
#include "hw/arm/armv7m.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/loader.h"
#include "qemu/log.h"

#include "hw/arm/stm32l467_soc.h"
#include "hw/arm/stm32l476_gpio.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define PERIPH_BASE 0x40000000UL
#define APB1PERIPH_BASE        PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x08000000UL)

/*!< APB1 peripherals */
#define TIM2_BASE             (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE             (APB1PERIPH_BASE + 0x0400UL)
#define TIM4_BASE             (APB1PERIPH_BASE + 0x0800UL)
#define TIM5_BASE             (APB1PERIPH_BASE + 0x0C00UL)
#define TIM6_BASE             (APB1PERIPH_BASE + 0x1000UL)
#define TIM7_BASE             (APB1PERIPH_BASE + 0x1400UL)
#define LCD_BASE              (APB1PERIPH_BASE + 0x2400UL)
#define RTC_BASE              (APB1PERIPH_BASE + 0x2800UL)
#define WWDG_BASE             (APB1PERIPH_BASE + 0x2C00UL)
#define IWDG_BASE             (APB1PERIPH_BASE + 0x3000UL)
#define SPI2_BASE             (APB1PERIPH_BASE + 0x3800UL)
#define SPI3_BASE             (APB1PERIPH_BASE + 0x3C00UL)
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400UL)
#define USART3_BASE           (APB1PERIPH_BASE + 0x4800UL)
#define UART4_BASE            (APB1PERIPH_BASE + 0x4C00UL)
#define UART5_BASE            (APB1PERIPH_BASE + 0x5000UL)
#define I2C1_BASE             (APB1PERIPH_BASE + 0x5400UL)
#define I2C2_BASE             (APB1PERIPH_BASE + 0x5800UL)
#define I2C3_BASE             (APB1PERIPH_BASE + 0x5C00UL)
#define CAN1_BASE             (APB1PERIPH_BASE + 0x6400UL)
#define PWR_BASE              (APB1PERIPH_BASE + 0x7000UL)
#define DAC_BASE              (APB1PERIPH_BASE + 0x7400UL)
#define DAC1_BASE             (APB1PERIPH_BASE + 0x7400UL)
#define OPAMP_BASE            (APB1PERIPH_BASE + 0x7800UL)
#define OPAMP1_BASE           (APB1PERIPH_BASE + 0x7800UL)
#define OPAMP2_BASE           (APB1PERIPH_BASE + 0x7810UL)
#define LPTIM1_BASE           (APB1PERIPH_BASE + 0x7C00UL)
#define LPUART1_BASE          (APB1PERIPH_BASE + 0x8000UL)
#define SWPMI1_BASE           (APB1PERIPH_BASE + 0x8800UL)
#define LPTIM2_BASE           (APB1PERIPH_BASE + 0x9400UL)


/*!< APB2 peripherals */
#define SYSCFG_BASE           (APB2PERIPH_BASE + 0x0000UL)
#define VREFBUF_BASE          (APB2PERIPH_BASE + 0x0030UL)
#define COMP1_BASE            (APB2PERIPH_BASE + 0x0200UL)
#define COMP2_BASE            (APB2PERIPH_BASE + 0x0204UL)
#define EXTI_BASE             (APB2PERIPH_BASE + 0x0400UL)
#define FIREWALL_BASE         (APB2PERIPH_BASE + 0x1C00UL)
#define SDMMC1_BASE           (APB2PERIPH_BASE + 0x2800UL)
#define TIM1_BASE             (APB2PERIPH_BASE + 0x2C00UL)
#define SPI1_BASE             (APB2PERIPH_BASE + 0x3000UL)
#define TIM8_BASE             (APB2PERIPH_BASE + 0x3400UL)
#define USART1_BASE           (APB2PERIPH_BASE + 0x3800UL)
#define TIM15_BASE            (APB2PERIPH_BASE + 0x4000UL)
#define TIM16_BASE            (APB2PERIPH_BASE + 0x4400UL)
#define TIM17_BASE            (APB2PERIPH_BASE + 0x4800UL)
#define SAI1_BASE             (APB2PERIPH_BASE + 0x5400UL)
#define SAI1_Block_A_BASE     (SAI1_BASE + 0x0004UL)
#define SAI1_Block_B_BASE     (SAI1_BASE + 0x0024UL)
#define SAI2_BASE             (APB2PERIPH_BASE + 0x5800UL)
#define SAI2_Block_A_BASE     (SAI2_BASE + 0x0004UL)
#define SAI2_Block_B_BASE     (SAI2_BASE + 0x0024UL)
#define DFSDM1_BASE           (APB2PERIPH_BASE + 0x6000UL)
#define DFSDM1_Channel0_BASE  (DFSDM1_BASE + 0x0000UL)
#define DFSDM1_Channel1_BASE  (DFSDM1_BASE + 0x0020UL)
#define DFSDM1_Channel2_BASE  (DFSDM1_BASE + 0x0040UL)
#define DFSDM1_Channel3_BASE  (DFSDM1_BASE + 0x0060UL)
#define DFSDM1_Channel4_BASE  (DFSDM1_BASE + 0x0080UL)
#define DFSDM1_Channel5_BASE  (DFSDM1_BASE + 0x00A0UL)
#define DFSDM1_Channel6_BASE  (DFSDM1_BASE + 0x00C0UL)
#define DFSDM1_Channel7_BASE  (DFSDM1_BASE + 0x00E0UL)
#define DFSDM1_Filter0_BASE   (DFSDM1_BASE + 0x0100UL)
#define DFSDM1_Filter1_BASE   (DFSDM1_BASE + 0x0180UL)
#define DFSDM1_Filter2_BASE   (DFSDM1_BASE + 0x0200UL)
#define DFSDM1_Filter3_BASE   (DFSDM1_BASE + 0x0280UL)

/*!< AHB1 peripherals */
#define DMA1_BASE             (AHB1PERIPH_BASE)
#define DMA2_BASE             (AHB1PERIPH_BASE + 0x0400UL)
#define RCC_BASE              (AHB1PERIPH_BASE + 0x1000UL)
#define FLASH_R_BASE          (AHB1PERIPH_BASE + 0x2000UL)
#define CRC_BASE              (AHB1PERIPH_BASE + 0x3000UL)
#define TSC_BASE              (AHB1PERIPH_BASE + 0x4000UL)


#define DMA1_Channel1_BASE    (DMA1_BASE + 0x0008UL)
#define DMA1_Channel2_BASE    (DMA1_BASE + 0x001CUL)
#define DMA1_Channel3_BASE    (DMA1_BASE + 0x0030UL)
#define DMA1_Channel4_BASE    (DMA1_BASE + 0x0044UL)
#define DMA1_Channel5_BASE    (DMA1_BASE + 0x0058UL)
#define DMA1_Channel6_BASE    (DMA1_BASE + 0x006CUL)
#define DMA1_Channel7_BASE    (DMA1_BASE + 0x0080UL)
#define DMA1_CSELR_BASE       (DMA1_BASE + 0x00A8UL)


#define DMA2_Channel1_BASE    (DMA2_BASE + 0x0008UL)
#define DMA2_Channel2_BASE    (DMA2_BASE + 0x001CUL)
#define DMA2_Channel3_BASE    (DMA2_BASE + 0x0030UL)
#define DMA2_Channel4_BASE    (DMA2_BASE + 0x0044UL)
#define DMA2_Channel5_BASE    (DMA2_BASE + 0x0058UL)
#define DMA2_Channel6_BASE    (DMA2_BASE + 0x006CUL)
#define DMA2_Channel7_BASE    (DMA2_BASE + 0x0080UL)
#define DMA2_CSELR_BASE       (DMA2_BASE + 0x00A8UL)


/*!< AHB2 peripherals */
#define GPIOA_BASE            (AHB2PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE            (AHB2PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE            (AHB2PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE            (AHB2PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE            (AHB2PERIPH_BASE + 0x1000UL)
#define GPIOF_BASE            (AHB2PERIPH_BASE + 0x1400UL)
#define GPIOG_BASE            (AHB2PERIPH_BASE + 0x1800UL)
#define GPIOH_BASE            (AHB2PERIPH_BASE + 0x1C00UL)

#define USBOTG_BASE           (AHB2PERIPH_BASE + 0x08000000UL)

#define ADC1_BASE             (AHB2PERIPH_BASE + 0x08040000UL)
#define ADC2_BASE             (AHB2PERIPH_BASE + 0x08040100UL)
#define ADC3_BASE             (AHB2PERIPH_BASE + 0x08040200UL)
#define ADC123_COMMON_BASE    (AHB2PERIPH_BASE + 0x08040300UL)


#define RNG_BASE              (AHB2PERIPH_BASE + 0x08060800UL)


/*!< FMC Banks registers base  address */
#define FMC_Bank1_R_BASE      (FMC_R_BASE + 0x0000UL)
#define FMC_Bank1E_R_BASE     (FMC_R_BASE + 0x0104UL)
#define FMC_Bank3_R_BASE      (FMC_R_BASE + 0x0080UL)

/* Debug MCU registers base address */
#define DBGMCU_BASE           (0xE0042000UL)

/*!< USB registers base address */
#define USB_OTG_FS_PERIPH_BASE               (0x50000000UL)

#define USB_OTG_GLOBAL_BASE                  (0x00000000UL)
#define USB_OTG_DEVICE_BASE                  (0x00000800UL)
#define USB_OTG_IN_ENDPOINT_BASE             (0x00000900UL)
#define USB_OTG_OUT_ENDPOINT_BASE            (0x00000B00UL)
#define USB_OTG_EP_REG_SIZE                  (0x00000020UL)
#define USB_OTG_HOST_BASE                    (0x00000400UL)
#define USB_OTG_HOST_PORT_BASE               (0x00000440UL)
#define USB_OTG_HOST_CHANNEL_BASE            (0x00000500UL)
#define USB_OTG_HOST_CHANNEL_SIZE            (0x00000020UL)
#define USB_OTG_PCGCCTL_BASE                 (0x00000E00UL)
#define USB_OTG_FIFO_BASE                    (0x00001000UL)
#define USB_OTG_FIFO_SIZE                    (0x00001000UL)

#define QSPI_BASE             (0x90000000UL) /*!< QUADSPI memories accessible over AHB base address */
#define QSPI_R_BASE           (0xA0001000UL) /*!< QUADSPI control registers base address */

enum {
    DMA2_Channel1_IRQn = 56,     /*!< DMA2 Channel 1 global Interrupt                                   */
    DMA2_Channel2_IRQn = 57,     /*!< DMA2 Channel 2 global Interrupt                                   */
    DMA2_Channel3_IRQn = 58,     /*!< DMA2 Channel 3 global Interrupt                                   */
    DMA2_Channel4_IRQn = 59,     /*!< DMA2 Channel 4 global Interrupt                                   */
    DMA2_Channel5_IRQn = 60,     /*!< DMA2 Channel 5 global Interrupt                                   */

    LPTIM1_IRQn = 65,     /*!< LP TIM1 interrupt                                                 */

    DMA2_Channel6_IRQn = 68,     /*!< DMA2 Channel 6 global interrupt                                   */
    DMA2_Channel7_IRQn = 69,     /*!< DMA2 Channel 7 global interrupt                                   */
};


static inline void create_unimplemented_layer(const char *name, hwaddr base, hwaddr size) {
    DeviceState *dev = qdev_create(NULL, TYPE_UNIMPLEMENTED_DEVICE);

    qdev_prop_set_string(dev, "name", name);
    qdev_prop_set_uint64(dev, "size", size);
    qdev_init_nofail(dev);

    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(dev), 0, base, -900);
}

static void stm32l467_soc_initfn(Object *obj) {
    STM32L467State *s = STM32L467_SOC(obj);

    sysbus_init_child_obj(obj, "armv7m", &s->armv7m, sizeof(s->armv7m),
                          TYPE_ARMV7M);

    sysbus_init_child_obj(obj, "pwr", &s->pwr, sizeof(s->pwr),
                          TYPE_STM32L476_PWR);

    sysbus_init_child_obj(obj, "lptim1", &s->lptim1, sizeof(s->lptim1),
                          TYPE_STM32L476_LPTIM);

    sysbus_init_child_obj(obj, "rcc", &s->rcc, sizeof(s->rcc),
                          TYPE_STM32L476_RCC);

    sysbus_init_child_obj(obj, "flash", &s->flash_r, sizeof(s->flash_r),
                          TYPE_STM32L476_FLASH);

    sysbus_init_child_obj(obj, "DMA2", &s->dma, sizeof(s->dma),
                          TYPE_STM32L476_DMA);

    sysbus_init_child_obj(obj, "QSPI", &s->qspi, sizeof(s->qspi),
                          TYPE_STM32L476_QSPI);

    for (int i = 0; i < 3; i++) {
        sysbus_init_child_obj(obj, "I2C[*]", &s->i2c[i], sizeof(s->i2c[i]), TYPE_STM32L476_I2C);
        char *name = g_strdup_printf("I2C%d", i+1);
        qdev_prop_set_string(DEVICE(&s->i2c[i]), "name", name);
    }

    sysbus_init_child_obj(obj, "SPI3", &s->spi[2], sizeof(s->spi[2]),
                          TYPE_STM32L476_SPI);
    s->spi[2].rxdrq = s->dma.channels[0].drq[0b0011]; // RX
    s->spi[2].txdrq = s->dma.channels[1].drq[0b0011]; // TX
}

static void stm32l467_soc_realize(DeviceState *dev_soc, Error **errp) {
    STM32L467State *s = STM32L467_SOC(dev_soc);
    Error *err = NULL;
    SysBusDevice *busdev;

    MemoryRegion *system_memory = get_system_memory();

    create_unimplemented_layer("IO", 0, 0xFFFFFFFF);

    create_unimplemented_layer("TIM2", TIM2_BASE, 0x400);
    create_unimplemented_layer("TIM3", TIM3_BASE, 0x400);
    create_unimplemented_layer("RTC", RTC_BASE, 0x400);
    create_unimplemented_layer("RNG", RNG_BASE, 0x400);
    create_unimplemented_layer("USART1", USART1_BASE, 0x400);
    create_unimplemented_layer("DMA1", DMA1_BASE, 0x400);
    create_unimplemented_layer("IWDG", IWDG_BASE, 0x400);
    create_unimplemented_layer("ADC1", ADC1_BASE, 0x100);
    create_unimplemented_layer("ADC2", ADC2_BASE, 0x100);
    create_unimplemented_layer("ADC3", ADC3_BASE, 0x100);
    create_unimplemented_layer("ADC123_COMMON", ADC123_COMMON_BASE, 0x100);
    create_unimplemented_layer("SPI1", SPI1_BASE, 0x400);

    create_unimplemented_layer("CRC", CRC_BASE, 0x400);
    create_unimplemented_layer("TIM1", TIM1_BASE, 0x400);

    s->syscfg = qdev_create(NULL, "stm32l476-syscfg");
    qdev_init_nofail(s->syscfg);
    sysbus_mmio_map(SYS_BUS_DEVICE(s->syscfg), 0, SYSCFG_BASE);

    /* GPIO */
    struct
    {
        hwaddr addr;
        char *name;
        uint32_t GPIOx_MODER;
        uint32_t GPIOx_OSPEEDR;
        uint32_t GPIOx_PUPDR;
    } const gpio_desc[] = {
        {GPIOA_BASE, "GPIOA", 0xABFFFFFF, 0x0C000000, 0x64000000},
        {GPIOB_BASE, "GPIOB", 0xFFFFFEBF, 0x00000000, 0x00000100},
        {GPIOC_BASE, "GPIOC", 0xFFFFFFFF, 0x00000000, 0x00000000},
        {GPIOD_BASE, "GPIOD", 0xFFFFFFFF, 0x00000000, 0x00000000},
        {GPIOE_BASE, "GPIOE", 0xFFFFFFFF, 0x00000000, 0x00000000},
        {GPIOF_BASE, "GPIOF", 0xFFFFFFFF, 0x00000000, 0x00000000},
        {GPIOG_BASE, "GPIOG", 0xFFFFFFFF, 0x00000000, 0x00000000},
        {GPIOH_BASE, "GPIOH", 0x0000000F, 0x00000000, 0x00000000},
    };
    for (int i = 0; i < ARRAY_SIZE(gpio_desc); ++i)
    {
        s->gpio[i] = qdev_create(NULL, TYPE_STM32L476_GPIO);
        s->gpio[i]->id = gpio_desc[i].name;
        qdev_init_nofail(s->gpio[i]);
        sysbus_mmio_map(SYS_BUS_DEVICE(s->gpio[i]), 0, gpio_desc[i].addr);
    }

    create_unimplemented_layer("EXTI", EXTI_BASE, 0x400);
    create_unimplemented_layer("QUADSPI", QSPI_R_BASE, 0x400);

    memory_region_init_rom(&s->flash, OBJECT(dev_soc), "STM32L467.flash",
                           FLASH_SIZE, &error_fatal);

    memory_region_init_alias(&s->flash_alias,
                             OBJECT(dev_soc),
                             "alias",
                             &s->flash,
                             0,
                             FLASH_SIZE);
    memory_region_add_subregion(system_memory, 0, &s->flash);
    memory_region_add_subregion(system_memory, FLASH_BASE_ADDRESS, &s->flash_alias);

    memory_region_init_ram(&s->sram1, OBJECT(dev_soc), "STM32F205.sram1", 96 * 1024, &error_fatal);
    memory_region_add_subregion(system_memory, 0x20000000, &s->sram1); // 0x18000
    //  memsave 0x20000000 0x18000 ../sram1.bin

    memory_region_init_ram(&s->sram2, OBJECT(dev_soc), "STM32F205.sram2", 32 * 1024, &error_fatal);
    memory_region_add_subregion(system_memory, 0x10000000, &s->sram2); // 0x8000
    //  memsave 0x10000000 0x8000 ../sram2.bin

    DeviceState *armv7m = DEVICE(&s->armv7m);
    qdev_prop_set_string(armv7m, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m4"));
    qdev_prop_set_uint32(armv7m, "num-irq", 82);
    object_property_set_link(OBJECT(&s->armv7m), OBJECT(system_memory),
                             "memory", &error_abort);
    object_property_set_bool(OBJECT(&s->armv7m), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* PWR registers */
    object_property_set_bool(OBJECT(&s->pwr), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->pwr);
    sysbus_mmio_map(busdev, 0, PWR_BASE);

    /* LPTIM1 registers */
    object_property_set_bool(OBJECT(&s->lptim1), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->lptim1);
    sysbus_mmio_map(busdev, 0, LPTIM1_BASE);
    sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, LPTIM1_IRQn));

    /* RCC registers */
    object_property_set_bool(OBJECT(&s->rcc), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->rcc);
    sysbus_mmio_map(busdev, 0, RCC_BASE);

    /* FLASH registers */
    object_property_set_bool(OBJECT(&s->flash_r), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->flash_r);
    sysbus_mmio_map(busdev, 0, FLASH_R_BASE);

    /* DMA registers */
    object_property_set_bool(OBJECT(&s->dma), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->dma);
    sysbus_mmio_map(busdev, 0, DMA2_BASE);
    sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, DMA2_Channel1_IRQn));
    sysbus_connect_irq(busdev, 1, qdev_get_gpio_in(armv7m, DMA2_Channel2_IRQn));
    sysbus_connect_irq(busdev, 2, qdev_get_gpio_in(armv7m, DMA2_Channel3_IRQn));
    sysbus_connect_irq(busdev, 3, qdev_get_gpio_in(armv7m, DMA2_Channel4_IRQn));
    sysbus_connect_irq(busdev, 4, qdev_get_gpio_in(armv7m, DMA2_Channel5_IRQn));
    sysbus_connect_irq(busdev, 5, qdev_get_gpio_in(armv7m, DMA2_Channel6_IRQn));
    sysbus_connect_irq(busdev, 6, qdev_get_gpio_in(armv7m, DMA2_Channel7_IRQn));

    /* SPI registers */
    object_property_set_bool(OBJECT(&s->spi[2]), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->spi[2]);
    sysbus_mmio_map(busdev, 0, SPI3_BASE);

    /* QSPI registers */
    object_property_set_bool(OBJECT(&s->qspi), true, "realized", &err);
    busdev = SYS_BUS_DEVICE(&s->qspi);
    sysbus_mmio_map(busdev, 0, QSPI_BASE);
    sysbus_mmio_map(busdev, 1, QSPI_R_BASE);

    /* I2C registers */
    for (int i = 0; i < 3; i++) {
        object_property_set_bool(OBJECT(&s->i2c[i]), true, "realized", &err);
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c[0]), 0, I2C1_BASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c[1]), 0, I2C2_BASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c[2]), 0, I2C3_BASE);

    system_clock_scale = 1000;
}

static Property stm32l467_soc_properties[] = {
    DEFINE_PROP_STRING("cpu-type", STM32L467State, cpu_type),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32l467_soc_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = stm32l467_soc_realize;
    device_class_set_props(dc, stm32l467_soc_properties);
}

static const TypeInfo stm32l467_soc_info = {
    .name          = TYPE_STM32L467_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32L467State),
    .instance_init = stm32l467_soc_initfn,
    .class_init    = stm32l467_soc_class_init,
};

static void stm32l467_soc_types(void) {
    type_register_static(&stm32l467_soc_info);
}

type_init(stm32l467_soc_types)