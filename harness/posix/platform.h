/* Platform layer for hosted systems: x86-64 Linux (rdtsc), Linux on ARM
 * (perf_event_open cycle counter, fallback clock_gettime), macOS arm64
 * (CNTVCT_EL0, development only). Selected at compile time; see main.c. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include <time.h>

void plat_print(const char *line);
int  plat_setup(int cpu);        /* pin to cpu, open counters; 0 = ok */
extern const char *plat_timer_name;
#define PLAT_NAME  plat_name()
#define PLAT_TIMER plat_timer_name
const char *plat_name(void);

static inline void plat_irq_off(void) {}
static inline void plat_irq_on(void) {}

#if defined(__x86_64__)
typedef uint64_t plat_t;
static inline plat_t plat_now(void){
    uint32_t lo, hi;
    __asm__ volatile("lfence\n\trdtsc\n\tlfence" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__) && defined(__APPLE__)
typedef uint64_t plat_t;
static inline plat_t plat_now(void){
    uint64_t v;
    __asm__ volatile("isb\n\tmrs %0, cntvct_el0\n\tisb" : "=r"(v) :: "memory");
    return v;
}
#elif defined(__linux__)
typedef uint64_t plat_t;
extern int plat_perf_fd;
plat_t plat_now_slow(void);
static inline plat_t plat_now(void){ return plat_now_slow(); }
#else
#error "unsupported host"
#endif

static inline uint32_t plat_delta(plat_t t0, plat_t t1){
    uint64_t d = t1 - t0;
    return d > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)d;
}
#endif
