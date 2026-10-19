/* Dipper timing-leakage test, RIOT application (Cortex-M boards). */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "periph_conf.h"
#include "ztimer.h"
#include "dudect.h"
#include "platform.h"

#ifndef CFG_NCAL
#define CFG_NCAL 10000
#endif

unsigned plat_irq_state;
static char name_buf[128];
const char *plat_name_str = name_buf;

void plat_print(const char *line){
    fputs(line, stdout);
    fputs("\r\n", stdout);
    fflush(stdout);
}

void plat_setup(void){
#if defined(DWT_CTRL_CYCCNTENA_Msk) && !defined(PLAT_FORCE_SYSTICK)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#else
    SysTick->CTRL = 0;
    SysTick->LOAD = 0x00FFFFFFu;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;   /* core clock, no IRQ */
#endif
    snprintf(name_buf, sizeof name_buf, "riot-%s-%s-cpuid%08lx-%luHz", RIOT_BOARD, RIOT_CPU,
             (unsigned long)SCB->CPUID, (unsigned long)CLOCK_CORECLOCK);
}

int main(void){
    plat_setup();
    ztimer_sleep(ZTIMER_MSEC, 2000);          /* let the serial reader attach */
    dudect_cfg_t cfg = { .seed = CFG_SEED, .n = CFG_N, .ncal = CFG_NCAL, .nraw = CFG_NRAW,
                         .variants = CFG_VARIANTS, .tests = CFG_TESTS };
    for (;;) {
        dudect_run_all(&cfg);
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
