# Environment

Every value comes from the run logs (`#meta` lines), the build tools, or the cited datasheet. Datasheet facts that are an inference rather than a direct statement are marked *(inference)*.
CPUID values are read from SCB->CPUID on the device:

| CPUID | core |
|---|---|
| 0x411FC231 | Cortex-M3 r1p1 |
| 0x410FC241 | Cortex-M4 r0p1 |
| 0x410CC601 | Cortex-M0+ r0p1 |

## Build flags (same for all five variants on a platform)

| platform | compiler | flags |
|---|---|---|
| x86-64 | gcc 13.3.0 (Ubuntu 24.04) | `-O2 -Wall -Wextra -ffp-contract=off` |
| Cortex-A8 (a8) | gcc 9.3.0 (on the node, Yocto image) | same as x86 |
| Cortex-M (RIOT) | arm-none-eabi-gcc 9.3.1 (GNU Arm Embedded 9-2020-q2) from `/opt/riot.source` on the IoT-LAB frontend | RIOT 2026.07-branch (`50ae2eba`) defaults with `CFLAGS_OPT=-O2`, plus `-ffp-contract=off` |
| ESP32 / ESP32-C6 | xtensa-esp-elf-gcc and riscv32-esp-elf-gcc 15.2.0 (crosstool-NG esp-15.2.0_20251204) | ESP-IDF 6.1.0 via PlatformIO 6.1.18, platform espressif32@7.1.3 (framework-espidf 4.60100.0); `CONFIG_COMPILER_OPTIMIZATION_PERF` (-O2), `-ffp-contract=off` |

`-ffp-contract=off` affects only the statistics code. It makes the device t-values bit-identical to the host recompute.

## Platforms

| platform | chip / core (from device) | clock | memory feature (source) | timer | runs (node, seed, date UTC) |
|---|---|---|---|---|---|
| x86-64 KVM guest (Contabo VPS) | "AMD EPYC Processor (with IBPB)", family 23 model 1 (Zen 1), 8 vCPU, KVM | TSC (BIOS string 2.0 GHz) | L1d 32 KB per vCPU, L2 512 KB per vCPU, L3 8 MB (lscpu) | `lfence; rdtsc; lfence` (clocksource kvm-clock; no cpufreq in the guest) | CPU 3 s1, CPU 6 s2 (N=10^6); CPU 3 s3, CPU 6 s4 (N=10^7), 19 Oct |
| TI AM3505 (IoT-LAB a8) | Cortex-A8 (`CPU part 0xc08`, rev 7), Linux 5.4.114 Yocto | 600 MHz (IoT-LAB page) | L1 16 KB I + 16 KB D, L2 256 KB, 64 B lines (TI SPRS550F; ARM DDI0344K) | `clock_gettime(CLOCK_MONOTONIC_RAW)`. `perf_event_open` gives ENOSYS. Clocksource `timer1`; measured step 76.9 ns (13 MHz); one call costs about 2.77 µs (syscall). No cpufreq governor in sysfs. | a8-10 s1, a8-40 s2 (N=10^6); a8-10 s3, a8-40 s4 (N=10^7), 20 Oct |
| STM32F103RE (IoT-LAB m3) | Cortex-M3 r1p1 | 72 MHz | no cache; 2×64-bit prefetch buffer on ICode only; 2 wait states (RM0008 rev 6 §2.3) | DWT CYCCNT | m3-4 s1, m3-11 s2 (Grenoble), 19 Oct |
| nRF52840 (IoT-LAB nrf52840dk) | Cortex-M4F r0p1 | 64 MHz | NVMC I-cache on ICODE only; data reads not cached *(inference from the bus split)*; WFLASH 2 WS (nRF52840 PS, NVMC) | DWT CYCCNT | nrf52840dk-4 s1, nrf52840dk-8 s2 (Saclay), 19 Oct |
| SAMR21G18A (IoT-LAB samr21) | Cortex-M0+ r0p1. The IoT-LAB page says M0; the datasheet and CPUID say M0+. | 48 MHz | NVM cache 8 lines × 64 bit, on at reset; serves data reads too *(inference: single AHB bus)*; 1 WS (Atmel-42223G §20.6.8, Tab. 42-30) | SysTick, 24-bit, core clock | samr21-3 s1, samr21-12 s2 (Saclay), 19 Oct |
| STM32L475VG (IoT-LAB st-iotnode) | Cortex-M4F r0p1 | 80 MHz | ART: I-cache 32×256 bit, D-cache 8×256 bit (256 B) for DCode literal/const reads, both on at reset; 4 WS (RM0351 §3.3.4, §3.7.1) | DWT CYCCNT | st-iotnode-2 s1, st-iotnode-7 s2 (Saclay), 19 Oct |
| RP2040 (IoT-LAB rpi-pico) | Cortex-M0+ r0p1 | 125 MHz | XIP cache 16 KB, 2-way, 8 B lines, for instruction and data reads (RP2040 DS §2.6.3) | SysTick, 24-bit, core clock | rpi-pico-2 s1, rpi-pico-7 s2 (Saclay), 19 Oct |
| ESP32-D0WD-V3 rev 3.0 (Ali's NodeMCU-32S) | Xtensa LX6, 2 cores; test on core 1 | 240 MHz | 32 KB 2-way cache per core, 32 B lines; `.rodata` in DROM via cache (ESP32 TRM v5.8 §3.3) | CCOUNT | own board (MAC c0:49:ef:6b:e6:74): Wi-Fi off s1, s2 (19 Oct); Wi-Fi scan load s3, s4 (20 Oct) |
| ESP32 rev 1.0 (IoT-LAB lopy4) | Xtensa LX6, 2 cores; test on core 1 | 240 MHz | as above | CCOUNT | lopy4-17 s1, lopy4-5 s2 (Strasbourg), 20 Oct |
| ESP32-C6 (IoT-LAB esp32-c6c1) | RISC-V HP core, 1 core | 160 MHz | 32 KB unified I/D cache, 4-way, 32 B lines (ESP32-C6 TRM v1.2 §5.3.3.2) | `esp_cpu_get_cycle_count()` = custom `mpccr`, which saturates; reset to 0 before each timed region | esp32-c6c1-1 s1, esp32-c6c1-3 s2 (Strasbourg), 20 Oct |

Not run:
- **rpi3**. All four nodes listed as Alive (rpi3-1..4, Grenoble) failed deployment in two experiments on 20 Oct (455733, 455734). They were not reachable by SSH ("No route to host"). The board was optional in the brief.
- **nrf52dk, st-lrwan1, arduino-zero, firefly**. These optional extra boards were not attempted.

## Measurement settings (all platforms)

- N = 10^6 measurements per (variant, test) after calibration, unless stated otherwise.
- Calibration: 10^4 samples. The exceptions are iotlab-m3 with 8000 and samr21 with 2500, because of RAM. The calibration samples are discarded.
- Batch: 256 samples (128 on m3 and samr21). Inputs and class labels are prepared before each batch with xoshiro128++. The seed is in every `#run` line.
- Fixed inputs: TV3 key `000102…0E0F` and TV3 plaintext `0123456789ABCDEF`.
- Timed region: one encryption (Test A), or one 128-bit key schedule plus one encryption (Test B), called through a function pointer.
  - Bare metal (RIOT): interrupts off around every measurement.
  - ESP-IDF: `portENTER_CRITICAL` on the measuring core.
  - Linux: process pinned with `sched_setaffinity`.
- Raw dump: the first 131072 samples of every run. The host recompute matches the device to the last bit; see `results/summary/recompute.txt`.

## Table placement

Placement comes from `nm` of the linked image, `results/raw/nm/`.

| platform | `S[]` (16 B, small and key schedule) | `SB2`/`PT` (fast) | `SB2C`/`PTC` (fast_const) |
|---|---|---|---|
| x86-64 | `.rodata` 0x5410, inside one 64 B line | `.bss` | `.rodata` |
| ESP32 | DROM 0x3F405D68, inside one 32 B cache line | DRAM 0x3FFB… | DROM 0x3F405D98 / 0x3F406598 |
| Cortex-M (RIOT) | flash (e.g. m3 0x080063A0, nRF52840 0x63D8: offset 8 in a 16-byte block) | RAM 0x2000… | flash |
| RP2040 | XIP flash 0x10008058 | SRAM 0x2000… | XIP flash |
| a8 | `.rodata` 0x4144, inside one 64 B line | `.bss` | `.rodata` |
