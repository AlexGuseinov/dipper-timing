/* Host test program (x86-64 Linux, Linux/ARM boards, macOS for development).
 *   ./dudect_host [-s seed] [-n N] [-c ncal] [-r nraw] [-v mask] [-t mask] [-p cpu]
 * Prints the line protocol of docs/PROTOCOL.md to stdout. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dudect.h"
#include "platform.h"
#ifdef __linux__
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <sys/utsname.h>
#endif

const char *plat_timer_name = "?";
static char name_buf[128] = "host";
const char *plat_name(void){ return name_buf; }

void plat_print(const char *line){ fputs(line, stdout); fputs("\r\n", stdout); fflush(stdout); }

#if defined(__linux__) && !defined(__x86_64__)
int plat_perf_fd = -1;
plat_t plat_now_slow(void){
    if (plat_perf_fd >= 0) {
        uint64_t v;
        if (read(plat_perf_fd, &v, sizeof v) == sizeof v) return v;
    }
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
#endif

int plat_setup(int cpu){
#ifdef __linux__
    struct utsname u; uname(&u);
    snprintf(name_buf, sizeof name_buf, "linux-%s-%s", u.machine, u.release);
    if (cpu >= 0) {
        cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu, &set);
        if (sched_setaffinity(0, sizeof set, &set)) { perror("sched_setaffinity"); return 1; }
    }
#if defined(__x86_64__)
    plat_timer_name = "rdtsc(lfence)";
#else
    struct perf_event_attr pe; memset(&pe, 0, sizeof pe);
    pe.type = PERF_TYPE_HARDWARE; pe.size = sizeof pe;
    pe.config = PERF_COUNT_HW_CPU_CYCLES; pe.exclude_kernel = 1; pe.exclude_hv = 1;
    plat_perf_fd = (int)syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (plat_perf_fd >= 0) {
        ioctl(plat_perf_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(plat_perf_fd, PERF_EVENT_IOC_ENABLE, 0);
        plat_timer_name = "perf_event_open(CPU_CYCLES,read)";
    } else {
        perror("perf_event_open");
        plat_timer_name = "clock_gettime(MONOTONIC_RAW,ns)";
    }
#endif
#else
    (void)cpu;
    static char tn[64];
    uint64_t f; __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
    snprintf(name_buf, sizeof name_buf, "macos-arm64-devonly");
    snprintf(tn, sizeof tn, "cntvct_el0(%lluHz)", (unsigned long long)f);
    plat_timer_name = tn;
#endif
    return 0;
}

int main(int argc, char **argv){
    dudect_cfg_t cfg = { .seed = 1, .n = 100000, .ncal = 10000, .nraw = 0,
                         .variants = 0x1F, .tests = 3 };
    int cpu = -1, opt;
    while ((opt = getopt(argc, argv, "s:n:c:r:v:t:p:")) != -1) {
        unsigned long v = strtoul(optarg, NULL, 0);
        switch (opt) {
        case 's': cfg.seed = (uint32_t)v; break;
        case 'n': cfg.n = (uint32_t)v; break;
        case 'c': cfg.ncal = (uint32_t)v; break;
        case 'r': cfg.nraw = (uint32_t)v; break;
        case 'v': cfg.variants = (uint32_t)v; break;
        case 't': cfg.tests = (uint32_t)v; break;
        case 'p': cpu = (int)v; break;
        default: fprintf(stderr, "bad option\n"); return 2;
        }
    }
    if (plat_setup(cpu)) return 1;
    return dudect_run_all(&cfg);
}
