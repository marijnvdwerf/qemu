
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
 * Machine
 */

typedef struct {
  MachineState parent;

  ARMv7MState armv7m;
  MemoryRegion flash;
  MemoryRegion sysram;

  MemoryRegion crg_aon;
} BipSMachineState;

#define TYPE_BIP_S_MACHINE MACHINE_TYPE_NAME("bip-s")

#define BIPS_MACHINE(obj)                                       \
    OBJECT_CHECK(BipSMachineState, obj, TYPE_BIP_S_MACHINE)

static inline void create_unimplemented_layer(const char *name, hwaddr base, hwaddr size) {
    DeviceState *dev = qdev_create(NULL, TYPE_UNIMPLEMENTED_DEVICE);

    qdev_prop_set_string(dev, "name", name);
    qdev_prop_set_uint64(dev, "size", size);
    qdev_init_nofail(dev);

    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(dev), 0, base, -900);
}

static void bip_s_init(MachineState *machine) {
    Error * err = NULL;
    BipSMachineState *bip = BIPS_MACHINE(machine);

    MemoryRegion *system_memory = get_system_memory();

    sysbus_init_child_obj(OBJECT(bip), "armv7m", &bip->armv7m, sizeof(bip->armv7m), TYPE_ARMV7M);
    DeviceState *armv7m = DEVICE(&bip->armv7m);
    qdev_prop_set_string(armv7m, "cpu-type", machine->cpu_type);
    object_property_set_link(OBJECT(armv7m), OBJECT(system_memory),
                             "memory", &error_abort);
    object_property_set_bool(OBJECT(armv7m), true, "realized", &err);
    if (err != NULL) {
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

    load_image_targphys("/Users/Marijn/Downloads/tonlesap_202006191826_2.1.1.16_tonlesap.img", 0x0, 0x8192 * 1024);

    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename,
                       0x8192 * 1024);
}

static void bip_s_machine_class_init(ObjectClass *oc, void *data) {
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "Amazfit Bip S";
    mc->init = bip_s_init;
    mc->max_cpus = 1;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-m33");
}

static const TypeInfo bip_s_info = {
    .name = TYPE_BIP_S_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(BipSMachineState),
    .class_init = bip_s_machine_class_init,
};

static void bip_s_machine_init(void) {
    type_register_static(&bip_s_info);
}

type_init(bip_s_machine_init);
