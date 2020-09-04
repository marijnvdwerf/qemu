/*
 * ARMV7M System emulation.
 *
 * Copyright (c) 2006-2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "qemu/osdep.h"
#include "hw/arm/armv7m.h"
#include "qapi/error.h"
#include "cpu.h"
#include "hw/sysbus.h"
<<<<<<< HEAD
#include "hw/arm/boot.h"
=======
#include "hw/arm/arm.h"
#include "hw/arm/stm32.h"
>>>>>>> 919b29ba7d... Pebble Qemu
#include "hw/loader.h"
#include "hw/qdev-properties.h"
#include "elf.h"
#include "sysemu/qtest.h"
#include "sysemu/reset.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "exec/address-spaces.h"
#include "target/arm/idau.h"

/* Bitbanded IO.  Each word corresponds to a single bit.  */

/* Get the byte address of the real memory for a bitband access.  */
static inline hwaddr bitband_addr(BitBandState *s, hwaddr offset)
{
    return s->base | (offset & 0x1ffffff) >> 5;
}

static MemTxResult bitband_read(void *opaque, hwaddr offset,
                                uint64_t *data, unsigned size, MemTxAttrs attrs)
{
    BitBandState *s = opaque;
    uint8_t buf[4];
    MemTxResult res;
    int bitpos, bit;
    hwaddr addr;

    assert(size <= 4);

    /* Find address in underlying memory and round down to multiple of size */
    addr = bitband_addr(s, offset) & (-size);
    res = address_space_read(&s->source_as, addr, attrs, buf, size);
    if (res) {
        return res;
    }
    /* Bit position in the N bytes read... */
    bitpos = (offset >> 2) & ((size * 8) - 1);
    /* ...converted to byte in buffer and bit in byte */
    bit = (buf[bitpos >> 3] >> (bitpos & 7)) & 1;
    *data = bit;
    return MEMTX_OK;
}

static MemTxResult bitband_write(void *opaque, hwaddr offset, uint64_t value,
                                 unsigned size, MemTxAttrs attrs)
{
    BitBandState *s = opaque;
    uint8_t buf[4];
    MemTxResult res;
    int bitpos, bit;
    hwaddr addr;

    assert(size <= 4);

    /* Find address in underlying memory and round down to multiple of size */
    addr = bitband_addr(s, offset) & (-size);
    res = address_space_read(&s->source_as, addr, attrs, buf, size);
    if (res) {
        return res;
    }
    /* Bit position in the N bytes read... */
    bitpos = (offset >> 2) & ((size * 8) - 1);
    /* ...converted to byte in buffer and bit in byte */
    bit = 1 << (bitpos & 7);
    if (value & 1) {
        buf[bitpos >> 3] |= bit;
    } else {
        buf[bitpos >> 3] &= ~bit;
    }
    return address_space_write(&s->source_as, addr, attrs, buf, size);
}

static const MemoryRegionOps bitband_ops = {
    .read_with_attrs = bitband_read,
    .write_with_attrs = bitband_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
};

static void bitband_init(Object *obj)
{
    BitBandState *s = BITBAND(obj);
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &bitband_ops, s,
                          "bitband", 0x02000000);
    sysbus_init_mmio(dev, &s->iomem);
}

static void bitband_realize(DeviceState *dev, Error **errp)
{
    BitBandState *s = BITBAND(dev);

    if (!s->source_memory) {
        error_setg(errp, "source-memory property not set");
        return;
    }

    address_space_init(&s->source_as, s->source_memory, "bitband-source");
}

/* Board init.  */

static const hwaddr bitband_input_addr[ARMV7M_NUM_BITBANDS] = {
    0x20000000, 0x40000000
};

static const hwaddr bitband_output_addr[ARMV7M_NUM_BITBANDS] = {
    0x22000000, 0x42000000
};

static void armv7m_instance_init(Object *obj)
{
    ARMv7MState *s = ARMV7M(obj);
    int i;

    /* Can't init the cpu here, we don't yet know which model to use */

    memory_region_init(&s->container, obj, "armv7m-container", UINT64_MAX);

    sysbus_init_child_obj(obj, "nvnic", &s->nvic, sizeof(s->nvic), TYPE_NVIC);
    object_property_add_alias(obj, "num-irq",
                              OBJECT(&s->nvic), "num-irq", &error_abort);

    for (i = 0; i < ARRAY_SIZE(s->bitband); i++) {
        sysbus_init_child_obj(obj, "bitband[*]", &s->bitband[i],
                              sizeof(s->bitband[i]), TYPE_BITBAND);
    }
}

static void armv7m_realize(DeviceState *dev, Error **errp)
{
    ARMv7MState *s = ARMV7M(dev);
    SysBusDevice *sbd;
    Error *err = NULL;
    int i;

    if (!s->board_memory) {
        error_setg(errp, "memory property was not set");
        return;
    }

    memory_region_add_subregion_overlap(&s->container, 0, s->board_memory, -1);

    s->cpu = ARM_CPU(object_new_with_props(s->cpu_type, OBJECT(s), "cpu",
                                           &err, NULL));
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    object_property_set_link(OBJECT(s->cpu), OBJECT(&s->container), "memory",
                             &error_abort);
    if (object_property_find(OBJECT(s->cpu), "idau", NULL)) {
        object_property_set_link(OBJECT(s->cpu), s->idau, "idau", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
    }
    if (object_property_find(OBJECT(s->cpu), "init-svtor", NULL)) {
        object_property_set_uint(OBJECT(s->cpu), s->init_svtor,
                                 "init-svtor", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
    }
    if (object_property_find(OBJECT(s->cpu), "start-powered-off", NULL)) {
        object_property_set_bool(OBJECT(s->cpu), s->start_powered_off,
                                 "start-powered-off", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
    }
    if (object_property_find(OBJECT(s->cpu), "vfp", NULL)) {
        object_property_set_bool(OBJECT(s->cpu), s->vfp,
                                 "vfp", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
    }
    if (object_property_find(OBJECT(s->cpu), "dsp", NULL)) {
        object_property_set_bool(OBJECT(s->cpu), s->dsp,
                                 "dsp", &err);
        if (err != NULL) {
            error_propagate(errp, err);
            return;
        }
    }

    /*
     * Tell the CPU where the NVIC is; it will fail realize if it doesn't
     * have one. Similarly, tell the NVIC where its CPU is.
     */
    s->cpu->env.nvic = &s->nvic;
    s->nvic.cpu = s->cpu;

    object_property_set_bool(OBJECT(s->cpu), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* Note that we must realize the NVIC after the CPU */
    object_property_set_bool(OBJECT(&s->nvic), true, "realized", &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }

    /* Alias the NVIC's input and output GPIOs as our own so the board
     * code can wire them up. (We do this in realize because the
     * NVIC doesn't create the input GPIO array until realize.)
     */
    qdev_pass_gpios(DEVICE(&s->nvic), dev, NULL);
    qdev_pass_gpios(DEVICE(&s->nvic), dev, "SYSRESETREQ");
    qdev_pass_gpios(DEVICE(&s->nvic), dev, "NMI");

    /* Wire the NVIC up to the CPU */
    sbd = SYS_BUS_DEVICE(&s->nvic);
    sysbus_connect_irq(sbd, 0,
                       qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_IRQ));

    memory_region_add_subregion(&s->container, 0xe000e000,
                                sysbus_mmio_get_region(sbd, 0));

    if (s->enable_bitband) {
        for (i = 0; i < ARRAY_SIZE(s->bitband); i++) {
            Object *obj = OBJECT(&s->bitband[i]);
            SysBusDevice *sbd = SYS_BUS_DEVICE(&s->bitband[i]);

            object_property_set_int(obj, bitband_input_addr[i], "base", &err);
            if (err != NULL) {
                error_propagate(errp, err);
                return;
            }
            object_property_set_link(obj, OBJECT(s->board_memory),
                                     "source-memory", &error_abort);
            object_property_set_bool(obj, true, "realized", &err);
            if (err != NULL) {
                error_propagate(errp, err);
                return;
            }

            memory_region_add_subregion(&s->container, bitband_output_addr[i],
                                        sysbus_mmio_get_region(sbd, 0));
        }
    }
}

<<<<<<< HEAD
static Property armv7m_properties[] = {
    DEFINE_PROP_STRING("cpu-type", ARMv7MState, cpu_type),
    DEFINE_PROP_LINK("memory", ARMv7MState, board_memory, TYPE_MEMORY_REGION,
                     MemoryRegion *),
    DEFINE_PROP_LINK("idau", ARMv7MState, idau, TYPE_IDAU_INTERFACE, Object *),
    DEFINE_PROP_UINT32("init-svtor", ARMv7MState, init_svtor, 0),
    DEFINE_PROP_BOOL("enable-bitband", ARMv7MState, enable_bitband, false),
    DEFINE_PROP_BOOL("start-powered-off", ARMv7MState, start_powered_off,
                     false),
    DEFINE_PROP_BOOL("vfp", ARMv7MState, vfp, true),
    DEFINE_PROP_BOOL("dsp", ARMv7MState, dsp, true),
    DEFINE_PROP_END_OF_LIST(),
};

static void armv7m_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = armv7m_realize;
    device_class_set_props(dc, armv7m_properties);
=======
static void armv7m_bitband_init(Object *parent)
{
    DeviceState *dev;

    dev = qdev_create(NULL, TYPE_BITBAND);
    qdev_prop_set_uint32(dev, "base", 0x20000000);
    if(parent) {
        object_property_add_child(parent, "bitband-sram", OBJECT(dev), NULL);
    }
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x22000000);

    dev = qdev_create(NULL, TYPE_BITBAND);
    qdev_prop_set_uint32(dev, "base", 0x40000000);
    if(parent) {
        object_property_add_child(parent, "bitband-periph", OBJECT(dev), NULL);
    }
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x42000000);
>>>>>>> 919b29ba7d... Pebble Qemu
}

static const TypeInfo armv7m_info = {
    .name = TYPE_ARMV7M,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(ARMv7MState),
    .instance_init = armv7m_instance_init,
    .class_init = armv7m_class_init,
};

static void armv7m_reset(void *opaque)
{
    ARMCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}

<<<<<<< HEAD
void armv7m_load_kernel(ARMCPU *cpu, const char *kernel_filename, int mem_size)
{
=======
/* Init CPU and memory for a v7-M based board.
   flash_size and sram_size are in bytes.
   Returns the NVIC array.  */


DeviceState *armv7m_init(Object *parent, MemoryRegion *system_memory,
                      int flash_size, int sram_size, int num_irq,
                      const char *kernel_filename, const char *cpu_model)
{
    ARMCPU *cpu;
    return armv7m_translated_init(parent, system_memory, flash_size, sram_size, num_irq,
            kernel_filename, NULL, NULL, cpu_model, &cpu);
}

DeviceState *armv7m_translated_init(Object *parent, MemoryRegion *system_memory,
                                 int flash_size, int sram_size, int num_irq,
                                 const char *kernel_filename,
                                 uint64_t (*translate_fn)(void *, uint64_t),
                                 void *translate_opaque,
                                 const char *cpu_model,
                                 ARMCPU **cpu_device)
{
    ARMCPU *cpu;
    CPUARMState *env;
    DeviceState *nvic;
    if (num_irq == 0) {
        num_irq = STM32_MAX_IRQ + 1;
    }
>>>>>>> 919b29ba7d... Pebble Qemu
    int image_size;
    uint64_t entry;
    uint64_t lowaddr;
    int big_endian;
<<<<<<< HEAD
    AddressSpace *as;
    int asidx;
    CPUState *cs = CPU(cpu);
=======
    MemoryRegion *hack = g_new(MemoryRegion, 1);
    MemoryRegion *flash = NULL;
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    ObjectClass *cpu_oc;
    Error *err = NULL;

    if (kernel_filename) {
        flash = g_new(MemoryRegion, 1);
    }

    if (cpu_model == NULL) {
        cpu_model = "cortex-m3";
    }
    cpu_oc = cpu_class_by_name(TYPE_ARM_CPU, cpu_model);
    cpu = ARM_CPU(object_new(object_class_get_name(cpu_oc)));
    if (cpu == NULL) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }
    /* On Cortex-M3/M4, the MPU has 8 windows */
    object_property_set_int(OBJECT(cpu), 8, "pmsav7-dregion", &err);
    if (err) {
        error_report_err(err);
        exit(1);
    }
    object_property_set_bool(OBJECT(cpu), true, "realized", &err);
    if (err) {
        error_report_err(err);
        exit(1);
    }
    *cpu_device = cpu;
    env = &cpu->env;

    if (kernel_filename) {
        memory_region_init_ram(flash, NULL, "armv7m.flash", flash_size, &err);
        vmstate_register_ram_global(flash);
        memory_region_set_readonly(flash, true);
        memory_region_add_subregion(system_memory, 0, flash);
    }

    if (sram_size) {
        memory_region_init_ram(sram, NULL, "armv7m.sram", sram_size, &err);
        vmstate_register_ram_global(sram);
        memory_region_add_subregion(system_memory, 0x20000000, sram);
    }
    armv7m_bitband_init(parent);

    /* If this is an M4, create the core-coupled memory region */
    if (!strcmp(cpu_model, "cortex-m4")) {
        MemoryRegion *ccm = g_new(MemoryRegion, 1);
        memory_region_init_ram(ccm, NULL, "armv7m.ccm", 64 * 1024 /* 64K */, &err);
        vmstate_register_ram_global(ccm);
        memory_region_add_subregion(system_memory, 0x10000000, ccm);
    }

    nvic = qdev_create(NULL, "armv7m_nvic");
    qdev_prop_set_uint32(nvic, "num-irq", num_irq);
    env->nvic = nvic;
    if(parent) {
        object_property_add_child(parent, "nvic", OBJECT(nvic), NULL);
    }
    qdev_init_nofail(nvic);

    // Connect the nvic's CPU #0 "parent_irq" output to the CPU's IRQ input handler
    sysbus_connect_irq(SYS_BUS_DEVICE(nvic), 0,
                       qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_IRQ));
>>>>>>> 919b29ba7d... Pebble Qemu

    // Connect the nvic's "wakeup_out" output to the CPU's WKUP input handler
    qemu_irq cpu_wakeup_in = qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_WKUP);
    qdev_connect_gpio_out_named(DEVICE(nvic), "wakeup_out", 0, cpu_wakeup_in);



#ifdef TARGET_WORDS_BIGENDIAN
    big_endian = 1;
#else
    big_endian = 0;
#endif

<<<<<<< HEAD
    if (arm_feature(&cpu->env, ARM_FEATURE_EL3)) {
        asidx = ARMASIdx_S;
    } else {
        asidx = ARMASIdx_NS;
    }
    as = cpu_get_address_space(cs, asidx);

    if (kernel_filename) {
        image_size = load_elf_as(kernel_filename, NULL, NULL, NULL,
                                 &entry, &lowaddr, NULL,
                                 NULL, big_endian, EM_ARM, 1, 0, as);
        if (image_size < 0) {
            image_size = load_image_targphys_as(kernel_filename, 0,
                                                mem_size, as);
=======
    if (kernel_filename) {
        image_size = load_elf(kernel_filename, translate_fn, translate_opaque, &entry, &lowaddr,
                              NULL, big_endian, EM_ARM, 1);
        if (image_size < 0) {
            image_size = load_image_targphys(kernel_filename, 0, flash_size);
>>>>>>> 919b29ba7d... Pebble Qemu
            lowaddr = 0;
        }
        if (image_size < 0) {
            error_report("Could not load kernel '%s'", kernel_filename);
            exit(1);
        }
    }

    /* CPU objects (unlike devices) are not automatically reset on system
     * reset, so we must always register a handler to do so. Unlike
     * A-profile CPUs, we don't need to do anything special in the
     * handler to arrange that it starts correctly.
     * This is arguably the wrong place to do this, but it matches the
     * way A-profile does it. Note that this means that every M profile
     * board must call this function!
     */
    qemu_register_reset(armv7m_reset, cpu);
}

static Property bitband_properties[] = {
    DEFINE_PROP_UINT32("base", BitBandState, base, 0),
    DEFINE_PROP_LINK("source-memory", BitBandState, source_memory,
                     TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void bitband_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bitband_realize;
    device_class_set_props(dc, bitband_properties);
}

static const TypeInfo bitband_info = {
    .name          = TYPE_BITBAND,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BitBandState),
    .instance_init = bitband_init,
    .class_init    = bitband_class_init,
};

static void armv7m_register_types(void)
{
    type_register_static(&bitband_info);
    type_register_static(&armv7m_info);
}

type_init(armv7m_register_types)
