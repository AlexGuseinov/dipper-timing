# Results

Dipper-64/128 encryption, five C implementations, eleven platforms. The data was taken on 19–20 Oct 2026. Every number below was produced by code in this repository; the raw logs are in `results/raw/`.

**Rules used for every verdict:**
- **leak, repeated**: max |t| is above 4.5 in two independent runs (new seed, other node or CPU).
- **unclear**: above 4.5 in one run only.
- **no leak found at N**: never above 4.5.
- **constant**: every one of the 10^6 measurements had the same cycle count in both runs. The t statistic is undefined there; the timing simply did not vary.
- "No leak found" does not mean "proven constant-time".
- Thresholds are 4.5 (TVLA) and 10 (dudect). Both are shown, because the bold cells say whether every run was above 10 or only above 4.5.

## 1. Main table: max |t| per run (Test A: fixed vs random plaintext; Test B: fixed vs random key, key schedule timed)

### Main runs

| platform | test | small | fast | fast_const | ct | leaky |
|---|---|---|---|---|---|---|
| STM32F103 (IoT-LAB m3) | A | const. | const. | const. | const. | **116789.7 / 171796.8** |
| STM32F103 (IoT-LAB m3) | B | const. | const. | const. | const. | **175283.4 / 132884.9** |
| nRF52840 (IoT-LAB nrf52840dk) | A | **454.2 / 452.6** | 0.7 / 0.9 | **345.9 / 507.8** | 0.6 / 0.7 | **1405.4 / 1413.3** |
| nRF52840 (IoT-LAB nrf52840dk) | B | **767.8 / 768.1** | **1333.5 / 1334.4** | **1232.7 / 1234.0** | 0.6 / 0.5 | **1251.8 / 1253.6** |
| SAMR21G18A (IoT-LAB samr21) | A | **827.8 / 827.4** | const. | **807.1 / 804.2** | const. | **6445.2 / 6459.4** |
| SAMR21G18A (IoT-LAB samr21) | B | **868.0 / 868.7** | **725.6 / 725.2** | **812.3 / 895.6** | const. | **6132.9 / 6115.5** |
| STM32L475VG (IoT-LAB st-iotnode) | A | 0.2 / 1.0 | 0.7 / 0.9 | **1129.5 / 1132.0** | 0.9 / 0.2 | **5150.4 / 5201.1** |
| STM32L475VG (IoT-LAB st-iotnode) | B | 0.3 / 0.3 | 0.6 / 0.7 | **1078.2 / 1079.0** | 0.6 / 1.4 | **2710.5 / 2739.0** |
| RP2040 (IoT-LAB rpi-pico) | A | 1.2 / 1.4 | 0.7 / 0.6 | **11.0 / 10.1** | 1.0 / 2.5 | **1351.9 / 1336.7** |
| RP2040 (IoT-LAB rpi-pico) | B | 1.7 / 0.7 | 1.0 / 0.4 | **6.2 / 6.5** | 1.0 / 0.9 | **986.4 / 1018.3** |
| ESP32 NodeMCU-32S, Wi-Fi off | A | 1.8 / 1.7 | 2.8 / 2.2 | 1.2 / 1.7 | 1.9 / 2.7 | **703.7 / 705.2** |
| ESP32 NodeMCU-32S, Wi-Fi off | B | 0.7 / 0.8 | 1.4 / 1.7 | 1.1 / 0.6 | 2.2 / 1.1 | **704.4 / 704.3** |
| ESP32 NodeMCU-32S, Wi-Fi scan load | A | 1.8 / 1.3 | 0.9 / 1.4 | 1.5 / 0.3 | 1.0 / 0.7 | **704.3 / 704.4** |
| ESP32 NodeMCU-32S, Wi-Fi scan load | B | 2.7 / 1.2 | 1.5 / 0.9 | 0.1 / 1.9 | 0.5 / 1.1 | **705.0 / 706.4** |
| ESP32 rev 1 (IoT-LAB lopy4) | A | 1.8 / 1.7 | 2.8 / 2.2 | 1.2 / 1.7 | 1.9 / 2.7 | **703.7 / 705.2** |
| ESP32 rev 1 (IoT-LAB lopy4) | B | 0.7 / 0.8 | 1.4 / 1.7 | 1.1 / 0.6 | 2.2 / 1.1 | **704.4 / 704.3** |
| ESP32-C6 (IoT-LAB esp32-c6c1) | A | 1.8 / 1.7 | 1.2 / 0.8 | 1.6 / 1.4 | 0.6 / 1.7 | **1452.2 / 1453.3** |
| ESP32-C6 (IoT-LAB esp32-c6c1) | B | 1.5 / 2.5 | 0.7 / 1.5 | 1.2 / 0.8 | 1.0 / 0.9 | **1594.3 / 1594.9** |
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | A | 0.9 / 1.2 | 2.0 / 1.5 | 1.2 / 2.5 | 1.1 / 1.1 | **683.7 / 690.4** |
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | B | 2.0 / 2.6 | 3.1 / 1.6 | *7.6 / 1.5* | 1.7 / 1.3 | **610.5 / 668.9** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | A | 0.5 / 2.2 | **19.9 / 5.6** | **8.6 / 8.3** | 1.0 / 1.5 | **68.7 / 40.9** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | B | 1.6 / 1.6 | 1.8 / 3.6 | *2.3 / 7.4* | 0.9 / 1.1 | **45.3 / 26.2** |

### Extra runs (N = 10^7, seeds 3 and 4: x86 on CPUs 3/6, a8 on nodes a8-10/a8-40)

| platform | test | small | fast | fast_const | ct | leaky |
|---|---|---|---|---|---|---|
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | A | 2.1 / 1.4 | 1.9 / 3.1 | **6.7 / 9.0** | 1.3 / 1.9 | **2174.5 / 2145.8** |
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | B | 1.5 / 1.3 | 3.5 / 2.0 | **13.8 / 9.6** | 2.3 / 1.6 | **1936.4 / 2123.7** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | A | 2.1 / 1.8 | **8.1 / 88.4** | **9.2 / 33.8** | 1.3 / 1.3 | **1193.5 / 1419.9** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | B | 1.6 / 0.7 | **5.3 / 20.7** | 1.8 / 3.6 | 0.8 / 2.1 | **386.8 / 366.8** |

Cells: max |t| per run (seed 1 / seed 2 …). **bold** = leak, repeated (> 4.5 in every run); *italic* = unclear (> 4.5 in one run only); const. = every measurement had the same cycle count.



Verdict per cell (platform × variant × test, with N and both max |t| values): `results/summary/leakage.csv`. Every run: `results/summary/runs.csv`.

Notes on single cells:
- **ESP32 NodeMCU, Wi-Fi scan load**: two runs (seeds 3 and 4) on the same board, because only one NodeMCU was available.
- **ESP32 rev 1 (lopy4)** gives max |t| identical to the NodeMCU (ESP32 rev 3) for the same seed, down to the last printed digit. In the one run with an undamaged dump (`ct`, Test B), all 131072 raw cycle counts are identical. With interrupts off, the cycle count on these ESP32 chips is a deterministic function of the inputs, across two chip revisions and two boards.
- **x86 `fast_const` Test B** is "unclear": 2.3 / 7.4 at N = 10^6, and 1.8 / 3.6 at N = 10^7.
- **a8 `fast_const` Test B** was "unclear" at N = 10^6 (7.6 / 1.5). At N = 10^7 (two more seeds) it is a repeated leak in both tests:
  - Test A: 6.7 / 9.0
  - Test B: 13.8 / 9.6

## 2. What was seen, per memory group

**No data cache: STM32F103 (m3), nRF52840.**
- On the m3, every real variant (`small`, `fast`, `fast_const`, `ct`) took exactly the same number of cycles for all 10^6 inputs in both runs, in both tests. Only `leaky` varied, with |t| above 10^5.
- The nRF52840 was expected to behave the same way: per its product specification the cache sits on the instruction bus only. It does not. `small` and `fast_const` leak (Test A: 454 / 453 and 346 / 508). In Test B, `fast` leaks too. `fast` and `ct` are clean in Test A.
- The common factor in every leaking case on the nRF52840 is a **secret-indexed read of a table in flash**:
  - `S[]` in `small`, and in the key schedule shared by `small`, `fast` and `fast_const`;
  - `SB2C`/`PTC` in `fast_const`.
- Secret-indexed reads of tables in RAM (`fast`, Test A) do not leak.
- So flash data reads on this chip have data-dependent timing, even though the documented cache is instruction-only. The mechanism (NVMC read buffer, bus arbitration) is not documented and was not identified here.

**Small flash cache: SAMR21 (NVM cache 64 B), STM32L475 (ART D-cache 256 B), RP2040 (XIP 16 KB).**
- **SAMR21**:
  - Everything that reads a table in flash leaks, about 800 in both runs: `small`, `fast_const`, and `fast` in Test B through the key schedule's `S[]`.
  - The same `fast` code with its tables in RAM is *constant* in Test A: zero variance.
  - `ct` is constant in both tests.
- **STM32L475**:
  - `small` is clean, because its 16-byte `S[]` fits in the 256-byte ART data cache.
  - `fast_const` leaks strongly (about 1100), because its 2.5 KB tables do not fit.
  - `fast` with RAM tables and `ct` are clean.
- **RP2040**:
  - Only `fast_const` crosses the threshold: 11.0 / 10.1 in A and 6.2 / 6.5 in B. That is a repeated leak, but small. The 16 KB XIP cache holds the 2.5 KB of tables almost all the time.
  - `small` and `fast` are clean.
- In this group the result depends on table size against cache size, and on flash versus RAM placement. That is what the study set out to test.

**ESP family (32 KB flash cache): ESP32 NodeMCU, ESP32 rev 1 (lopy4), ESP32-C6.**
- No variant other than `leaky` crossed 4.5 on any ESP board: max |t| ≤ 2.8 at N = 10^6, two runs each. The `fast_const` tables (2.5 KB) and `S[]` stay in the 32 KB cache after warm-up.
- This also holds with the Wi-Fi radio active (continuous scans on the other core), in two runs.
- The ESP32 timing is not perfectly constant. `small` varies by up to 4 cycles (57545–57549), but this variation does not separate the fixed and random classes.

**CPU data caches: x86-64 (KVM guest), Cortex-A8.**
- **x86-64**:
  - `fast` leaks in Test A: 19.9 / 5.6 at N = 10^6, and 8.1 / 88.4 at N = 10^7 (two more seeds).
  - `fast_const` leaks in Test A: 8.6 / 8.3 at N = 10^6, and 9.2 / 33.8 at N = 10^7.
  - `fast` Test B leaks at N = 10^7 (5.3 / 20.7).
  - `small` and `ct` show no leak at N = 10^7 (≤ 2.1). `small`'s `S[]` lies inside one 64-byte line.
  - The host is a virtual machine on a shared server, so the noise level is high. The leak in `fast` only clearly separates at N = 10^7.
- **Cortex-A8**:
  - At N = 10^6, no variant other than `leaky` leaks, and `fast_const` B is unclear (7.6 / 1.5).
  - At N = 10^7 (two more seeds, on the same two nodes), `fast_const` leaks in both tests: A 6.7 / 9.0, B 13.8 / 9.6. That is above 4.5 in all runs, and above 10 once.
  - `fast` uses the *same* tables and the same code with the tables in `.bss`, and does not leak: A 1.9 / 3.1, B 3.5 / 2.0.
  - On Linux both copies are in RAM. They differ only in placement: `.rodata` at 0x4188, close to the code, against `.bss` at 0x21de0. That could mean different L1 set conflicts in the 16 KB 4-way L1D, but the mechanism was not identified here.
  - `small` and `ct` show no leak at N = 10^7 (≤ 2.3).
  - The timer is `clock_gettime` with about 77 ns resolution and a syscall per read, so the test needs a larger N here than with a cycle counter.

**`ct` (constant-time version).**
- No leak found on any platform, and zero variance on m3 and SAMR21.
- The static checks agree:
  - Valgrind memcheck (ctgrind-style, x86-64): 0 reports.
  - Manual review of the compiled code for x86-64, ARMv7-A, Thumb-2 (M3/M4), ARMv6-M (M0+), Xtensa LX6 and RV32IMAC (`docs/ct_asm_review.md`) found no secret-dependent branch or address.

**`leaky` (positive control).** It was detected on every platform, in both tests and both runs, with |t| from 26 (x86, Test B, N = 10^6) up to 1.7·10^5 (m3). The method finds a known leak everywhere.

## 3. Static checks

Valgrind 3.22 memcheck, ctgrind-style, on x86-64; secret = key (key schedule) or plaintext + round keys (encryption). Details: `results/raw/ctgrind/SUMMARY.txt`.

| variant | key schedule | encryption |
|---|---|---|
| small | 3 secret-indexed loads (`S[]`, `dipper.c:100–102`) | secret-indexed load (`S[]`, `dipper.c:31`) |
| fast | same as small (shared key schedule) | secret-indexed load (`SB2[]`) |
| fast_const | same as small | secret-indexed load (`SB2C[]`) |
| ct | **0** | **0** |
| leaky | same as small | secret-dependent branch (`dipper_leaky.c:17`) + `S[]` |

Known limitation: memcheck treats a value loaded from a secret address as defined. The `PT[]` lookups that follow `SB2[]` in `fast` are therefore not reported separately, although the source code shows that they are indexed by the state.

## 4. Host recompute of the device statistics

For every run the first 131072 raw cycle counts were dumped over serial and the full statistic was recomputed on the host. Out of 260 runs:
- **240 are bit-identical** to the device value (same IEEE-754 double).
- **2 agree to 1e-3** (lopy4-5).
- **18 could not be compared**, because the dump was damaged on the serial link: lost bytes, merged lines, or a digit lost inside a word, which gives an impossible cycle count below half the calibration minimum. All 18 are on ESP32 boards: 16 on the two IoT-LAB LoPy4 nodes and 2 on the own board during the Wi-Fi runs. The ESP32-C6 dumps were undamaged.
- **No unexplained mismatch.**

Details: `results/summary/recompute.txt`. The verdicts use the device values, which are computed before the serial link and are not affected by it.

## 5. Cost

| platform | small enc | fast enc | fast_const enc | ct enc | key schedule (small) | key schedule (ct) |
|---|---|---|---|---|---|---|
| STM32F103 (IoT-LAB m3) | 77616 | 21209 | 26223 | 8937 | 2284 | 4455 |
| nRF52840 (IoT-LAB nrf52840dk) | 54577 | 15202 | 17264 | 6531 | 1636 | 3264 |
| SAMR21G18A (IoT-LAB samr21) | 80247 | 21199 | 22606 | 13527 | 3120 | 8032 |
| STM32L475VG (IoT-LAB st-iotnode) | 53772 | 15196 | 17826 | 5694 | 1534 | 3266 |
| RP2040 (IoT-LAB rpi-pico) | 73613 | 20176 | 20164 | 10356 | 2522 | 6664 |
| ESP32 NodeMCU-32S, Wi-Fi off | 57532 | 18543 | 18560 | 6180 | 1967 | 4368 |
| ESP32 NodeMCU-32S, Wi-Fi scan load | 57532 | 18544 | 18560 | 6180 | 1967 | 4368 |
| ESP32 rev 1 (IoT-LAB lopy4) | 57532 | 18543 | 18560 | 6180 | 1967 | 4368 |
| ESP32-C6 (IoT-LAB esp32-c6c1) | 49402 | 15480 | 16052 | 5992 | 1727 | 3381 |
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | 44668 | 17306 | 17180 | 5018 | 1355 | 3322 |
| x86-64 (KVM guest, AMD EPYC Zen 1) | 4821 | 1809 | 1908 | 1388 | 391 | 1003 |

Median of 9 runs of 64 chained blocks, per block. The x86 numbers are TSC ticks and the a8 numbers are ns. In relative terms, `ct` costs 0.11–0.29 of `small` on every platform: it is the **fastest** variant everywhere (`figures/cost_relative.png`). The upstream throughput numbers of the Dipper repository and of `dipper-mcu-bench` are not re-reported here.

Footprint of the linked builds (encryption + 128-bit key schedule; `results/summary/footprint.csv`):

| platform | variant | code (B) | tables | round keys |
|---|---|---|---|---|
| STM32F103 (m3) | small | 520 | RAM 128 B; flash 16 B | 224 B RAM |
| STM32F103 (m3) | fast | 508 | RAM 5248 B; flash 16 B | 224 B RAM |
| STM32F103 (m3) | fast_const | 484 | RAM 128 B; flash 2576 B | 224 B RAM |
| STM32F103 (m3) | ct | 1110 | none | 224 B RAM |
| STM32F103 (m3) | leaky | 566 | RAM 128 B; flash 16 B | 224 B RAM |
| nRF52840 | small | 520 | RAM 128 B; flash 16 B | 224 B RAM |
| nRF52840 | fast | 500 | RAM 5248 B; flash 16 B | 224 B RAM |
| nRF52840 | fast_const | 476 | RAM 128 B; flash 2576 B | 224 B RAM |
| nRF52840 | ct | 1140 | none | 224 B RAM |
| nRF52840 | leaky | 562 | RAM 128 B; flash 16 B | 224 B RAM |
| SAMR21 | small | 596 | RAM 128 B; flash 16 B | 224 B RAM |
| SAMR21 | fast | 596 | RAM 5248 B; flash 16 B | 224 B RAM |
| SAMR21 | fast_const | 568 | RAM 128 B; flash 2576 B | 224 B RAM |
| SAMR21 | ct | 1260 | none | 224 B RAM |
| SAMR21 | leaky | 632 | RAM 128 B; flash 16 B | 224 B RAM |
| STM32L475 (st-iotnode) | small | 520 | RAM 128 B; flash 16 B | 224 B RAM |
| STM32L475 (st-iotnode) | fast | 500 | RAM 5248 B; flash 16 B | 224 B RAM |
| STM32L475 (st-iotnode) | fast_const | 476 | RAM 128 B; flash 2576 B | 224 B RAM |
| STM32L475 (st-iotnode) | ct | 1140 | none | 224 B RAM |
| STM32L475 (st-iotnode) | leaky | 562 | RAM 128 B; flash 16 B | 224 B RAM |
| RP2040 | small | 596 | RAM 128 B; flash 16 B | 224 B RAM |
| RP2040 | fast | 596 | RAM 5248 B; flash 16 B | 224 B RAM |
| RP2040 | fast_const | 568 | RAM 128 B; flash 2576 B | 224 B RAM |
| RP2040 | ct | 1260 | none | 224 B RAM |
| RP2040 | leaky | 632 | RAM 128 B; flash 16 B | 224 B RAM |
| ESP32 (NodeMCU-32S) | small | 506 | RAM (DRAM) 128 B; flash (DROM, cached) 16 B | 224 B RAM |
| ESP32 (NodeMCU-32S) | fast | 534 | RAM (DRAM) 5248 B; flash (DROM, cached) 16 B | 224 B RAM |
| ESP32 (NodeMCU-32S) | fast_const | 510 | RAM (DRAM) 128 B; flash (DROM, cached) 2576 B | 224 B RAM |
| ESP32 (NodeMCU-32S) | ct | 1075 | none | 224 B RAM |
| ESP32 (NodeMCU-32S) | leaky | 558 | RAM (DRAM) 128 B; flash (DROM, cached) 16 B | 224 B RAM |
| ESP32-C6 | small | 628 | RAM 128 B; flash (cached) 16 B | 224 B RAM |
| ESP32-C6 | fast | 640 | RAM 5248 B; flash (cached) 16 B | 224 B RAM |
| ESP32-C6 | fast_const | 612 | RAM 128 B; flash (cached) 2576 B | 224 B RAM |
| ESP32-C6 | ct | 1422 | none | 224 B RAM |
| ESP32-C6 | leaky | 668 | RAM 128 B; flash (cached) 16 B | 224 B RAM |
| AM3505 Cortex-A8 (Linux) | small | 608 | process memory 144 B | 224 B RAM |
| AM3505 Cortex-A8 (Linux) | fast | 628 | process memory 5264 B | 224 B RAM |
| AM3505 Cortex-A8 (Linux) | fast_const | 580 | process memory 2704 B | 224 B RAM |
| AM3505 Cortex-A8 (Linux) | ct | 1496 | none | 224 B RAM |
| AM3505 Cortex-A8 (Linux) | leaky | 688 | process memory 144 B | 224 B RAM |
| x86-64 (Linux) | small | 534 | process memory 144 B | 224 B RAM |
| x86-64 (Linux) | fast | 577 | process memory 5264 B | 224 B RAM |
| x86-64 (Linux) | fast_const | 544 | process memory 2704 B | 224 B RAM |
| x86-64 (Linux) | ct | 1365 | none | 224 B RAM |
| x86-64 (Linux) | leaky | 589 | process memory 144 B | 224 B RAM |

Stack use (gcc `-fstack-usage`, Cortex-M builds), in bytes:

| core | `small` enc | `ct` enc | `small` key schedule | `ct` key schedule |
|---|---|---|---|---|
| M3 / M4 | 48 | 36–48 | 64–72 | 88 |
| M0+ | 48 | 72 | 64 | 112 |

Source: `results/summary/footprint_riot_raw.txt`.

## 6. Deviations from the brief and limits

- **rpi3**: not run. On 20 Oct, all four nodes listed as Alive failed deployment in three experiments, and also failed power-cycle commands. They were not reachable by SSH.
- **Calibration batch**: 8000 samples on m3 and 2500 on samr21, instead of 10^4, because of RAM.
- **x86** is a KVM virtual machine on a shared host (Contabo VPS), not bare metal. No system settings were changed.
- **a8** has no cycle counter in user space: `perf_event_open` gives ENOSYS, so `clock_gettime` is used at about 77 ns resolution. This is under the brief's 100 ns limit, but each read costs about 2.8 µs.
- **ESP32 Wi-Fi-on**: both runs are on the same board, with different seeds. The load is continuous active scanning, because no access point credentials were used.
- **nRF52840 and ESP serial links** lose bytes. On the nRF52840 the `#END`/`#BEGIN` lines around the 1 s sleep were lost. The analysis takes the first complete block and checks the dumps.
- **One LoPy4 run** (seed 2) was restarted from scratch, after an accidental power cycle caused by an operator error. Only the complete rerun is used; the partial log is in `results/dev/`.
- The test is first-order and second-order Welch t-tests with cropping. Other leakage forms (multivariate, frequency-domain) were not tested.

## 7. Figures

- `figures/maxt_vs_N_testA.png`, `figures/maxt_vs_N_testB.png`: max |t| against N, one panel per platform, for the N = 10^6 runs.
- `figures/cost_relative.png`: encryption cost relative to `small`.
