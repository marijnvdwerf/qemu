
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
#include "soc.h"

bool POWER_IS_UP = 1;
bool DBG_IS_ACTIVE = 0;
bool COM_IS_UP = 0;
bool COM_IS_DOWN = 1;
bool TIM_IS_UP = 0;
bool TIM_IS_DOWN = 1;
bool MEM_IS_UP = 1;
bool MEM_IS_DOWN = 0;
bool SYS_IS_UP = 1;
bool SYS_IS_DOWN = 0;
bool PER_IS_UP = 1;
bool PER_IS_DOWN = 0;
bool RAD_IS_UP = 1;
bool RAD_IS_DOWN = 0;

#define SYS_STAT_OFFSET 0x28

#define SYS_STAT_POWER_IS_UP_BIT    13
#define SYS_STAT_DBG_IS_ACTIVE_BIT  12
#define SYS_STAT_COM_IS_UP_BIT      11
#define SYS_STAT_COM_IS_DOWN_BIT    10
#define SYS_STAT_TIM_IS_UP_BIT      9
#define SYS_STAT_TIM_IS_DOWN_BIT    8
#define SYS_STAT_MEM_IS_UP_BIT      7
#define SYS_STAT_MEM_IS_DOWN_BIT    6
#define SYS_STAT_SYS_IS_UP_BIT      5
#define SYS_STAT_SYS_IS_DOWN_BIT    4
#define SYS_STAT_PER_IS_UP_BIT      3
#define SYS_STAT_PER_IS_DOWN_BIT    2
#define SYS_STAT_RAD_IS_UP_BIT      1
#define SYS_STAT_RAD_IS_DOWN_BIT    0

#define SYS_STAT_POWER_IS_UP        (1U << SYS_STAT_POWER_IS_UP_BIT)
#define SYS_STAT_DBG_IS_ACTIVE      (1U << SYS_STAT_DBG_IS_ACTIVE_BIT)
#define SYS_STAT_COM_IS_UP          (1U << SYS_STAT_COM_IS_UP_BIT)
#define SYS_STAT_COM_IS_DOWN        (1U << SYS_STAT_COM_IS_DOWN_BIT)
#define SYS_STAT_TIM_IS_UP          (1U << SYS_STAT_TIM_IS_UP_BIT)
#define SYS_STAT_TIM_IS_DOWN        (1U << SYS_STAT_TIM_IS_DOWN_BIT)
#define SYS_STAT_MEM_IS_UP          (1U << SYS_STAT_MEM_IS_UP_BIT)
#define SYS_STAT_MEM_IS_DOWN        (1U << SYS_STAT_MEM_IS_DOWN_BIT)
#define SYS_STAT_SYS_IS_UP          (1U << SYS_STAT_SYS_IS_UP_BIT)
#define SYS_STAT_SYS_IS_DOWN        (1U << SYS_STAT_SYS_IS_DOWN_BIT)
#define SYS_STAT_PER_IS_UP          (1U << SYS_STAT_PER_IS_UP_BIT)
#define SYS_STAT_PER_IS_DOWN        (1U << SYS_STAT_PER_IS_DOWN_BIT)
#define SYS_STAT_RAD_IS_UP          (1U << SYS_STAT_RAD_IS_UP_BIT)
#define SYS_STAT_RAD_IS_DOWN        (1U << SYS_STAT_RAD_IS_DOWN_BIT)

#define PMU_CTRL_OFFSET  0x20

#define PMU_CTRL_ENABLE_CLKLESS_BIT     8
#define PMU_CTRL_RETAIN_CACHE_BIT       7
#define PMU_CTRL_SYS_SLEEP_BIT          6
#define PMU_CTRL_RESET_ON_WAKEUP_BIT    5
#define PMU_CTRL_MAP_BANDGAP_EN_BIT     4
#define PMU_CTRL_COM_SLEEP_BIT          3
#define PMU_CTRL_TIM_SLEEP_BIT          2
#define PMU_CTRL_RADIO_SLEEP_BIT        1
#define PMU_CTRL_PERIPH_SLEEP_BIT       0
#define PMU_CTRL_ENABLE_CLKLESS         (1U << PMU_CTRL_ENABLE_CLKLESS_BIT)
#define PMU_CTRL_RETAIN_CACHE           (1U << PMU_CTRL_RETAIN_CACHE_BIT)
#define PMU_CTRL_SYS_SLEEP              (1U << PMU_CTRL_SYS_SLEEP_BIT)
#define PMU_CTRL_RESET_ON_WAKEUP        (1U << PMU_CTRL_RESET_ON_WAKEUP_BIT)
#define PMU_CTRL_MAP_BANDGAP_EN         (1U << PMU_CTRL_MAP_BANDGAP_EN_BIT)
#define PMU_CTRL_COM_SLEEP              (1U << PMU_CTRL_COM_SLEEP_BIT)
#define PMU_CTRL_TIM_SLEEP              (1U << PMU_CTRL_TIM_SLEEP_BIT)
#define PMU_CTRL_RADIO_SLEEP            (1U << PMU_CTRL_RADIO_SLEEP_BIT)
#define PMU_CTRL_PERIPH_SLEEP           (1U << PMU_CTRL_PERIPH_SLEEP_BIT)

#define CLK_CTRL_OFFSET 0x14
#define CLK_CTRL_SYS_RUNNING_AT_PLL96M_BIT      15
#define CLK_CTRL_SYS_RUNNING_AT_RC32M_BIT       14
#define CLK_CTRL_SYS_RUNNING_AT_XTAL32M_BIT     13
#define CLK_CTRL_SYS_RUNNING_AT_LP_CLK_BIT      12
#define CLK_CTRL_SYS_CLK_SEL_BIT                0

#define CLK_SWITCH2XTAL_OFFSET 0x1C
#define CLK_SWITCH2XTAL_SWITCH2XTAL_BIT         0
#define CLK_SWITCH2XTAL_SWITCH2XTAL             (1U << CLK_SWITCH2XTAL_SWITCH2XTAL_BIT)

bool pd_power = true;
bool pd_com = false;
bool pd_tim = false;
bool pd_mem = true;
bool pd_sys = true;
bool pd_per = false;
bool pd_rad = false;

typedef enum sys_clk_is_type {
  XTAL32M = 0,
  RC32M = 1,
  LowPower = 2,
  PLL96Mhz = 3,
} sys_clk_is_t;

sys_clk_is_t syc_clk = RC32M;

static uint32_t getValue(hwaddr offset) {
    switch (offset) {

        case CLK_CTRL_OFFSET: {
            uint32_t out = 0;
            out |= syc_clk << CLK_CTRL_SYS_CLK_SEL_BIT;
            out |= 1 << 6;

            if (syc_clk == PLL96Mhz)
                out |= 1U << CLK_CTRL_SYS_RUNNING_AT_PLL96M_BIT;
            if (syc_clk == XTAL32M)
                out |= 1U << CLK_CTRL_SYS_RUNNING_AT_XTAL32M_BIT;
            if (syc_clk == RC32M)
                out |= 1U << CLK_CTRL_SYS_RUNNING_AT_RC32M_BIT;
            if (syc_clk == LowPower)
                out |= 1U << CLK_CTRL_SYS_RUNNING_AT_LP_CLK_BIT;

            return out;
        }

        case SYS_STAT_OFFSET: {
            uint32_t out = 0;

            out |= pd_power ? SYS_STAT_POWER_IS_UP : 0;
            out |= pd_com ? SYS_STAT_COM_IS_UP : SYS_STAT_COM_IS_DOWN;
            out |= pd_tim ? SYS_STAT_TIM_IS_UP : SYS_STAT_TIM_IS_DOWN;
            out |= pd_mem ? SYS_STAT_MEM_IS_UP : SYS_STAT_MEM_IS_DOWN;
            out |= pd_sys ? SYS_STAT_SYS_IS_UP : SYS_STAT_SYS_IS_DOWN;
            out |= pd_per ? SYS_STAT_PER_IS_UP : SYS_STAT_PER_IS_DOWN;
            out |= pd_rad ? SYS_STAT_RAD_IS_UP : SYS_STAT_RAD_IS_DOWN;
            return out;
        }

        case PMU_CTRL_OFFSET: {
            uint32_t out = 0;

            out |= pd_com == false ? PMU_CTRL_COM_SLEEP : 0;
            out |= pd_tim == false ? PMU_CTRL_TIM_SLEEP : 0;
            out |= pd_rad == false ? PMU_CTRL_RADIO_SLEEP : 0;
            out |= pd_per == false ? PMU_CTRL_PERIPH_SLEEP : 0;
            return out;
        }
    }

    return 0;
}

static uint64_t pxa2xx_pic_mem_read(void *opaque, hwaddr offset,
                                    unsigned size) {

    switch (offset) {
        case SYS_STAT_OFFSET:
        case PMU_CTRL_OFFSET:
        case CLK_CTRL_OFFSET:
            return getValue(offset);

        default:
            qemu_log_mask(LOG_UNIMP, "Read from 0x%llX\n", offset + 0x50000000);
    }
    return 0;
}

static void pxa2xx_pic_mem_write(void *opaque, hwaddr offset,
                                 uint64_t value, unsigned size) {

    switch (offset) {
        case SYS_STAT_OFFSET:
        case PMU_CTRL_OFFSET:
        case CLK_CTRL_OFFSET: {
            uint32_t current = getValue(offset);;
            printf("Write 0x%08llX to  0x%llX (was 0x%08X)\n", value, offset + 0x50000000, current);
            break;
        }

        default:
            printf("Write 0x%llX to  0x%llX\n", value, offset + 0x50000000);
            break;
    }

    switch (offset) {
        case PMU_CTRL_OFFSET:
            pd_com = (value & PMU_CTRL_COM_SLEEP) ? false : true;
            pd_rad = (value & PMU_CTRL_RADIO_SLEEP) ? false : true;
            pd_per = (value & PMU_CTRL_PERIPH_SLEEP) ? false : true;
            pd_tim = (value & PMU_CTRL_TIM_SLEEP) ? false : true;
            break;

        case CLK_CTRL_OFFSET:
//            syc_clk = (value >> CLK_CTRL_SYS_CLK_SEL_BIT) & 0b11;
            break;

        case CLK_SWITCH2XTAL_OFFSET:
            if (value & CLK_SWITCH2XTAL_SWITCH2XTAL)
                syc_clk = XTAL32M;
            break;

    }
}

static const MemoryRegionOps crg_aon_ops = {
    .read = pxa2xx_pic_mem_read,
    .write = pxa2xx_pic_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

/**
 * OTPC
 */

#define OTPC 0x30070000

typedef enum {
  HW_OTPC_MODE_PDOWN = 0,    /**< OTP cell and LDO are inactive*/
  HW_OTPC_MODE_DSTBY = 1,    /**< OTP cell is powered on LDO is inactive*/
  HW_OTPC_MODE_STBY = 2,    /**< OTP cell and LDO are powered on, chip select is deactivated*/
  HW_OTPC_MODE_READ = 3,    /**< OTP cell can be read*/
  HW_OTPC_MODE_PROG = 4,    /**< OTP cell can be programmed*/
  HW_OTPC_MODE_PVFY = 5,    /**< OTP cell can be read in PVFY margin read mode*/
  HW_OTPC_MODE_RINI = 6     /**< OTP cell can be read in RINI margin read mode*/
} HW_OTPC_MODE;

// Mode register
#define OTPC_MODE_REG           0x00
#define OTPC_MODE_MODE_BIT       0

// Status register
#define OTPC_STAT_REG           0x04

// The address of the word that will be programmed, when the PROG mode is used.
#define OTPC_PADDR_REG          0x08

// The 32-bit word that will be programmed, when the PROG mode is used.
#define OTPC_PWORD_REG          0x0C

// Various timing parameters of the OTP cell.
#define OTPC_TIM1_REG           0x10

// Various timing parameters of the OTP cell.
#define OTPC_TIM2_REG           0x14

HW_OTPC_MODE otpc_mode;

static uint64_t otpc_mem_read(void *opaque, hwaddr offset, unsigned size) {

    switch (offset) {
        case OTPC_MODE_REG:
            return otpc_mode;

        case OTPC_STAT_REG: {
            uint32_t out = 0;
            out |= 1 << 0;
            out |= 1 << 1;
            out |= 1 << 2;

            return out;
        }
    }

    return 0;
}

static void otpc_mem_write(void *opaque, hwaddr offset,
                           uint64_t value, unsigned size) {
    switch (offset) {
        case OTPC_MODE_REG:
            otpc_mode = (value >> OTPC_MODE_MODE_BIT) & 0b111;
            break;
    }
}

static const MemoryRegionOps otpc_ops = {
    .read = otpc_mem_read,
    .write = otpc_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


/**
 * CRG_XTAL
 */

#define PLL_SYS_CTRL1_REG                                       0x60
#define CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_SEL_MIN_CUR_INT_BIT      14
#define CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_PRE_DIV_BIT              11
#define CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_N_DIV_BIT                4
#define CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_VREF_HOLD_BIT        3
#define CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_ENABLE_BIT           2
#define CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_EN_BIT                   1
#define CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_ENABLE               (1U << CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_ENABLE_BIT)
#define CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_EN                       (1U << CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_EN_BIT)

#define PLL_SYS_STATUS_REG                                      0x70
#define CRG_XTAL_PLL_SYS_STATUS_REG_LDO_PLL_OK_BIT              15
#define CRG_XTAL_PLL_SYS_STATUS_REG_PLL_CALIBRATION_END_BIT     11
#define CRG_XTAL_PLL_SYS_STATUS_REG_PLL_BEST_MIN_CUR_BIT        5
#define CRG_XTAL_PLL_SYS_STATUS_REG_PLL_LOCK_FINE_BIT           0

bool ldoPllOn = false;
bool pllOn = false;

static uint64_t crg_xtal_mem_read(void *opaque, hwaddr offset, unsigned size) {

    switch (offset) {
        case PLL_SYS_CTRL1_REG: {
            uint32_t out = 0;
            out |= 1 << CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_SEL_MIN_CUR_INT_BIT;
            out |= 2 << 12;
            out |= 1 << CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_PRE_DIV_BIT;
            out |= 6 << CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_N_DIV_BIT;
            out |= 0 << CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_VREF_HOLD_BIT;
            if (ldoPllOn)
                out |= 1 << CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_ENABLE_BIT;
            if (pllOn)
                out |= 1 << CRG_XTAL_PLL_SYS_CTRL1_REG_PLL_EN_BIT;

            return out;
        }
        case PLL_SYS_STATUS_REG: {
            uint32_t out = 0;
            if (ldoPllOn)
                out |= 1 << CRG_XTAL_PLL_SYS_STATUS_REG_LDO_PLL_OK_BIT;
            if (false) {
                out |= 1 << CRG_XTAL_PLL_SYS_STATUS_REG_PLL_CALIBRATION_END_BIT;
                out |= 1 << CRG_XTAL_PLL_SYS_STATUS_REG_PLL_BEST_MIN_CUR_BIT;
                out |= 1 << CRG_XTAL_PLL_SYS_STATUS_REG_PLL_LOCK_FINE_BIT;
            }

            return out;
        }
    }

    return 0;
}

static void crg_xtal_mem_write(void *opaque, hwaddr offset, uint64_t value, unsigned size) {
    switch (offset) {
        case PLL_SYS_CTRL1_REG:
            if (value & CRG_XTAL_PLL_SYS_CTRL1_REG_LDO_PLL_ENABLE) {
                ldoPllOn = true;
            } else {
                ldoPllOn = false;
            }
            break;
    }
}
static const MemoryRegionOps crg_xtal_ops = {
    .read = crg_xtal_mem_read,
    .write = crg_xtal_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

/**
 * Machine
 */

typedef struct {
  SysBusDevice parent_obj;

  ARMv7MState armv7m;
  MemoryRegion flash;
  MemoryRegion sysram;

  MemoryRegion crg_aon;
  MemoryRegion crg_xtal;
  MemoryRegion otpc_c;
} DA2469xState;

static inline void create_unimplemented_layer(const char *name, hwaddr base, hwaddr size) {
    DeviceState *dev = qdev_create(NULL, TYPE_UNIMPLEMENTED_DEVICE);

    qdev_prop_set_string(dev, "name", name);
    qdev_prop_set_uint64(dev, "size", size);
    qdev_init_nofail(dev);

    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(dev), 0, base, -900);
}

static void da1469x_soc_initfn(Object *obj) {
    DA2469xState *s = DA1469X_SOC(obj);

    sysbus_init_child_obj(obj, "armv7m", &s->armv7m, sizeof(s->armv7m), TYPE_ARMV7M);
}

static void da1469x_soc_realize(DeviceState *dev_soc, Error **errp) {
    Error *err = NULL;
    DA2469xState *bip = DA1469X_SOC(dev_soc);

    MemoryRegion *system_memory = get_system_memory();

    DeviceState *armv7m = DEVICE(&bip->armv7m);
    qdev_prop_set_string(armv7m, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m33"));
    object_property_set_link(OBJECT(&bip->armv7m), OBJECT(get_system_memory()), "memory", &error_abort);
    object_property_set_bool(OBJECT(&bip->armv7m), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    memory_region_init_ram(&bip->flash, NULL, "flash", 0x20000000, &err);
    if (err != NULL) {
        return;
    }
    memory_region_set_readonly(&bip->flash, true);
    memory_region_add_subregion(system_memory, 0x00000000, &bip->flash);

    memory_region_init_ram(&bip->sysram, NULL, "system", 512 * 1024, &error_fatal);
    memory_region_add_subregion(system_memory, 0x20000000, &bip->sysram);

    create_unimplemented_device("PSRAM", 0x00000000, 0xFFFFFFFF);

    // Power Domains Controller
    create_unimplemented_layer("PDC", 0x50000200, 0x100);

    static uint32_t crg_aon_val = 0xffffffff;
    memory_region_init_io(&bip->crg_aon, NULL, &crg_aon_ops, &crg_aon_val, "crc", 0x100);
    memory_region_add_subregion(system_memory, 0x50000000, &bip->crg_aon);

    static uint32_t crg_xtal_val = 0xffffffff;
    memory_region_init_io(&bip->crg_xtal, NULL, &crg_xtal_ops, &crg_xtal_val, "crg_xtal", 0x100);
    memory_region_add_subregion(system_memory, 0x50010000U, &bip->crg_xtal);

    static uint32_t otpc_val = 0xffffffff;
    memory_region_init_io(&bip->otpc_c, NULL, &otpc_ops, &otpc_val, "otpc", 0x80000);
    memory_region_add_subregion(system_memory, 0x30070000, &bip->otpc_c);
}

static void da1469x_soc_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = da1469x_soc_realize;
}

static const TypeInfo da1469x_soc_info = {
    .name          = TYPE_DA1469X_SOC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(DA2469xState),
    .instance_init = da1469x_soc_initfn,
    .class_init    = da1469x_soc_class_init,
};

static void da1469x_soc_types(void) {
    type_register_static(&da1469x_soc_info);
}

type_init(da1469x_soc_types)
