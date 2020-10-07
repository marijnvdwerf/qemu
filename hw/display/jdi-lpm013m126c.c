#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "qemu/log.h"
#include "hw/ssi/ssi.h"
#include "ui/console.h"
#include "ui/pixel_ops.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb;

typedef struct {
    SSISlave ssidev;
    QemuConsole *con;

    int state;
    uint16_t header;

    rgb buffer[176][176];
    uint32_t fifo;
    uint32_t fifo_len;
    uint32_t bpp;
    int cur_x;
    int line;
} lcd_state;

#define TYPE_LCD "jdi-lpm013m126c"
#define LCD(obj) \
    OBJECT_CHECK(lcd_state, (obj), TYPE_LCD)

#define M0_HI(x)  ((((x) >> 0) & 1) != 0)
#define M1_HI(x)  ((((x) >> 1) & 1) != 0)
#define M2_HI(x)  ((((x) >> 2) & 1) != 0)
#define M3_HI(x)  ((((x) >> 3) & 1) != 0)
#define M4_HI(x)  ((((x) >> 4) & 1) != 0)
#define M5_HI(x)  ((((x) >> 5) & 1) != 0)

#define M0_LOW(x)  ((((x) >> 0) & 1) == 0)
#define M1_LOW(x)  ((((x) >> 1) & 1) == 0)
#define M2_LOW(x)  ((((x) >> 2) & 1) == 0)
#define M3_LOW(x)  ((((x) >> 3) & 1) == 0)
#define M4_LOW(x)  ((((x) >> 4) & 1) == 0)
#define M5_LOW(x)  ((((x) >> 5) & 1) == 0)

static uint8_t bitswap8(uint8_t val) {
    return ((val * 0x0802LU & 0x22110LU) | (val * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

uint32_t parse_header(lcd_state *s, uint16_t header) {
    if (header == 0x0000 || header == 0xFFFF)
    {
        // Dummy
        s->state = 0;
        return 0;
    }

    if (M0_LOW(header) && M2_HI(header))
    {
        printf("all clear\n");
        for (int x = 0; x < 176; x++)
        {
            for (int y = 0; y < 176; y++)
            {
                s->buffer[y][x].r = x % 2;
                s->buffer[y][x].g = y % 2;
                s->buffer[y][x].b = 0;
            }
        }
        s->state = 0;
        return 0;
    }

    if (M0_HI(header) && M2_LOW(header))
    {
        // Update

        s->state = 2;
        s->line = bitswap8(header >> 8) - 1;
        s->cur_x = 0;
        if (M3_HI(header))
        {
            s->bpp = 4;
        }
        else if (M4_HI(header))
        {
            s->bpp = 1;
        }
        else
        {
            s->bpp = 3;
        }

        return 0;
    }
    else
    {
        assert(false);
    }

    s->state = 0;
    return 0;
}

uint32_t jdi_lpm013m126c_transfer(SSISlave *bus, uint32_t val32)
{
    lcd_state *s = LCD(bus);
    uint8_t val = val32 & 0xFF;

    if (s->state == -1)
    {
        return 0;
    }

    if (s->state == 0)
    {
        s->header = val;
        s->state = 1;
        return 0;
    }

    if (s->state == 1) {
        s->header |= val << 8;

        s->state = -1;
        parse_header(s, s->header);
        assert(s->state != -1);
        return 0;
    }

    if (s->state == 2) {
        s->fifo |= val << s->fifo_len;
        s->fifo_len += 8;

        while (s->fifo_len >= s->bpp) {

            int r, g, b;
            if (s->bpp == 1) {
                r = g = b = s->fifo & 1;
            } else {
                r = (s->fifo >> 0) & 1;
                g = (s->fifo >> 1) & 1;
                b = (s->fifo >> 2) & 1;
            }

            if(s->cur_x < 176)
            {
                s->buffer[s->line][s->cur_x].r = r;
                s->buffer[s->line][s->cur_x].g = g;
                s->buffer[s->line][s->cur_x].b = b;
            }
            s->fifo = s->fifo >> s->bpp;
            s->fifo_len -= s->bpp;

            s->cur_x++;
            if (s->cur_x == 176) {
                s->state = 0;
                return 0;
            }
        }
    }
}

static void sm_lcd_update_display(void *arg) {
    lcd_state *s = arg;

    DisplaySurface *surface = qemu_console_surface(s->con);
    int bpp = surface_bits_per_pixel(surface);
    void *d = surface_data(surface);

    assert(bpp == 32);

    for (int y = 0; y < 176; y++) {
        for (int x = 0; x < 176; x++) {

            *((uint32_t *) d) = rgb_to_pixel32(
                s->buffer[y][x].r * 255,
                s->buffer[y][x].g * 255,
                s->buffer[y][x].b * 255);
            d += 4;
        }
    }
    dpy_gfx_update(s->con, 0, 0, 176, 176);
}

static void sm_lcd_invalidate_display(void *arg) {
    lcd_state *s = arg;
}

static const GraphicHwOps sm_lcd_ops = {
    .gfx_update = sm_lcd_update_display,
    .invalidate = sm_lcd_invalidate_display,
};

static void jdi_lpm013m126c_realize(SSISlave *d, Error **errp) {

    DeviceState *dev = DEVICE(d);
    lcd_state *s = LCD(dev);

    s->con = graphic_console_init(dev, 0, &sm_lcd_ops, s);
    qemu_console_resize(s->con, 176, 176);
}

static void jdi_lpm013m126c_class_init(ObjectClass *klass, void *data) {
    SSISlaveClass *k = SSI_SLAVE_CLASS(klass);
    k->transfer = jdi_lpm013m126c_transfer;
    k->realize = jdi_lpm013m126c_realize;
    k->cs_polarity = SSI_CS_LOW;
}

static const TypeInfo jdi_lpm013m126c_info = {
    .name          = TYPE_LCD,
    .parent        = TYPE_SSI_SLAVE,
    .instance_size = sizeof(lcd_state),
    .class_init    = jdi_lpm013m126c_class_init,
};

static void jdi_lpm013m126c_register_types(void) {
    type_register_static(&jdi_lpm013m126c_info);
}

type_init(jdi_lpm013m126c_register_types)
