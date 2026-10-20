/* Timer resolution probe for Linux boards: clock_getres, smallest non-zero
 * step between consecutive clock_gettime(CLOCK_MONOTONIC_RAW) reads, and
 * whether perf_event_open(PERF_COUNT_HW_CPU_CYCLES) works from user space. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
static uint64_t ns(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return (uint64_t)t.tv_sec*1000000000ull + t.tv_nsec; }
int main(void){
    struct timespec r; clock_getres(CLOCK_MONOTONIC_RAW, &r);
    printf("clock_getres(MONOTONIC_RAW) = %ld ns\n", r.tv_nsec);
    uint64_t minstep = ~0ull, a = ns(); long same = 0, n = 0;
    for (int i = 0; i < 1000000; i++) { uint64_t b = ns(); if (b == a) same++; else { if (b - a < minstep) minstep = b - a; n++; } a = b; }
    printf("min nonzero step = %llu ns, %ld equal pairs, %ld steps in 1e6 reads\n", (unsigned long long)minstep, same, n);
    struct perf_event_attr pe; memset(&pe, 0, sizeof pe);
    pe.type = PERF_TYPE_HARDWARE; pe.size = sizeof pe; pe.config = PERF_COUNT_HW_CPU_CYCLES; pe.exclude_kernel = 1; pe.exclude_hv = 1;
    int fd = (int)syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    printf("perf_event_open(CPU_CYCLES): %s\n", fd >= 0 ? "OK" : strerror(errno));
    return 0;
}
