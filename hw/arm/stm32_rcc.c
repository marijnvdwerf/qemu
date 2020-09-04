#include "qemu/osdep.h"
#include "hw/arm/stm32.h"
#include "hw/arm/stm32_clktree.h"
#include "stm32f2xx_rcc.h"

/* PUBLIC FUNCTIONS */

void stm32_rcc_check_periph_clk(Stm32Rcc *s, stm32_periph_t periph)
{
    Clk clk = s->PERIPHCLK[periph];

    assert(clk != NULL);

    if(!clktree_is_enabled(clk)) {
        /* I assume writing to a peripheral register while the peripheral clock
         * is disabled is a bug and give a warning to unsuspecting programmers.
         * When I made this mistake on real hardware the write had no effect.
         */
        stm32_hw_warn("Warning: You are attempting to use the stm32_rcc peripheral %d while "
                 "its clock is disabled.\n", periph);
    }
}

void stm32_rcc_set_periph_clk_irq(
        Stm32Rcc *s,
        stm32_periph_t periph,
        qemu_irq periph_irq)
{
    Stm32f2xxRcc*v = STM32F2XX_RCC(s);
    Clk clk2 = v->PERIPHCLK[periph];

    assert(clk2 != NULL);

    clktree_adduser(clk2, periph_irq);
}

uint32_t stm32_rcc_get_periph_freq(
        Stm32Rcc *s,
        stm32_periph_t periph)
{
    Clk clk;

    Stm32f2xxRcc*v = STM32F2XX_RCC(s);
    clk = v->PERIPHCLK[periph];

    assert(clk != NULL);

    return clktree_get_output_freq(clk);
}


