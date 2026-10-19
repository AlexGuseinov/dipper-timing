/* Platform layer for RIOT on Cortex-M.
 * Cortex-M3/M4: DWT CYCCNT (32-bit up-counter at core clock).
 * Cortex-M0+ (no CYCCNT): SysTick, 24-bit down-counter at core clock,
 * free-running with LOAD = 0xFFFFFF (RIOT does not use SysTick); one timed
 * operation must stay below 2^24 cycles, checked by plat_setup's caller.
 * Interrupts are disabled around every timed region. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include "cpu.h"
#include "irq.h"

extern unsigned plat_irq_state;
extern const char *plat_name_str;
#define PLAT_NAME plat_name_str
void plat_setup(void);
void plat_print(const char *line);

static inline void plat_irq_off(void){ plat_irq_state = irq_disable(); }
static inline void plat_irq_on(void){ irq_restore(plat_irq_state); }

typedef uint32_t plat_t;
#if defined(DWT_CTRL_CYCCNTENA_Msk) && !defined(PLAT_FORCE_SYSTICK)
#define PLAT_TIMER "DWT_CYCCNT"
static inline plat_t plat_now(void){ return DWT->CYCCNT; }
static inline uint32_t plat_delta(plat_t t0, plat_t t1){ return t1 - t0; }
#else
#define PLAT_TIMER "SysTick(24bit,down,coreclk)"
static inline plat_t plat_now(void){ return SysTick->VAL; }
static inline uint32_t plat_delta(plat_t t0, plat_t t1){ return (t0 - t1) & 0x00FFFFFFu; }
#endif
#endif
