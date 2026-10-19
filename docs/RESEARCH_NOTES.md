# Research notes (19 Oct 2026)

These notes were gathered before any experiment. Tags: [V] means checked at a primary source, [P] partly checked, [U] not checked.
Nothing in this file is a measured result.

## 1. Source code facts (checked locally)

- Dipper repo HEAD is `3ae6788c7348749eed2113b05fb6c5cfb2651a9e` (10 Oct 2026). `dipper-mcu-bench` used `26d0f05`. Diff `timing/` between the two before copying.
- Dipper's S-box is the GIFT S-box, `1A4C6F392DB7508E`.
- Dipper's bit permutation uses the GIFT-64 formula `P(i) = 4⌊i/16⌋ + 16((3⌊(i mod 16)/4⌋ + (i mod 4)) mod 4) + (i mod 4)`. The fixslicing / bitsliced GIFT-64 permutation tricks therefore apply directly.
- `small`:
  - `S[]` is indexed by secret nibbles, both in the rounds and in the key schedule.
  - `apply_perm` uses only public `P[i]` as the shift amount.
  - Variable 64-bit shifts on 32-bit cores may call libgcc (`__aeabi_llsl`). That routine branches on the shift amount, which is public, so it is harmless, but record it.
- `fast`:
  - `SB2` and `PT` are both indexed by secret data. `PT[j][nibble]` holds 64-bit entries, and one 16-entry row is 128 B, which is 2+ cache lines.
  - There is no own key schedule: it uses `dipper_expand_*` from `dipper.c`. Test B for `fast` therefore also exercises the `small` key schedule (`S[]` lookups).
  - `init_fast()` is called inside every encryption. It contains a branch on `ready`, which is public.
- GIFT S-box Boolean formula, with s0 as the LSB plane:
  `s1^=s0&s2; s0^=s1&s3; s2^=s0|s1; s3^=s2; s1^=s3; s3=~s3; s2^=s0&s1; swap(s0,s3)`.
  - Checked by script against all 16 inputs; it matches.
  - Source: GIFT CHES 2017, and the `aadomn/gift` `SBOX` macro without the NOT and swap.

## 2. Corrections to the brief (from datasheets)

| Item in brief | Finding | Source |
|---|---|---|
| samr21 "no cache (expected)" | **Cortex-M0+ r0p1**, with an **NVM cache of 8 lines × 64 bit**, on at reset. This cache serves data reads too (single AHB bus; this is an inference). It belongs in the "small flash cache" group, not "no cache". 1 WS at 48 MHz. | Atmel-42223G SAM R21 DS §9.1, §20.6.8, Tab. 42-30 [V] |
| m3 "no data cache" | Correct. Prefetch buffer 2×64 bit, ICode only. DCode reads are not buffered. 2 WS at 72 MHz. | RM0008 rev 6 §2.3 [V, old rev] |
| nrf52840 "check whether it caches data reads" | The I-cache sits on **ICODE only**, so data reads (DCODE) are not cached (inference). WFLASH = 2 wait states, WRAM = 0. In effect "no data cache". | nRF52840 PS, NVMC [V] |
| st-iotnode ART | I-cache 32×256 bit, **D-cache 8×256 bit (256 B), covers DCode literal/const reads**. ICEN and DCEN are on at reset (ACR = 0x600). 4 WS at 80 MHz. | RM0351 §3.3.4, §3.7.1 [V] |
| rpi-pico | XIP cache 16 KB, 2-way, 8 B lines, caches instruction and data reads. `.rodata` → flash for both SDK and RIOT. No CYCCNT; SysTick 24-bit wraps in 0.134 s at 125 MHz. | RP2040 DS §2.6.3 [V] |
| esp32-c6 timer | The HP core has **no standard `mcycle`**. `esp_cpu_get_cycle_count()` reads the custom `mpccr` (CSR 0x7E2), which **saturates instead of wrapping** (COUNT_SAT = 1). The cache is a 32 KB unified I/D cache, 4-way, 32 B lines. RIOT default clock is 80 MHz. | ESP32-C6 TRM v1.2, ESP-IDF `rv_utils.h` [V] |
| ESP32 / lopy4 | 32 KB 2-way cache per core, 32 B lines. `.rodata` goes to DROM via the cache by default (`DRAM_ATTR` moves it to RAM). Flash ops (Wi-Fi NVS writes) disable the cache. The LoPy4 chip variant is not documented: read it on the node with esptool. | ESP32 TRM v5.8, ESP-IDF docs [V] |
| a8 caches | **L1 16 KB I + 16 KB D** (not 32 KB), L2 256 KB, 64 B lines. User-space PMU access is off by default, so use `perf_event_open`. | TI SPRS550F, ARM DDI0344K [V] |
| rpi3 | A53 ×4, 1.2 GHz. L1D 32 KB, L2 512 KB. These cache sizes come only from Linux DTS, not from Broadcom. The Yocto `raspberrypi3` machine is likely **32-bit armhf**: check with `uname -m`. Grenoble only, 5 nodes. | bcm2837.dtsi [P], iot-lab-yocto [P] |
| IoT-LAB pages | m3 "256 KB ROM": ST says 512 KB. samr21 "Cortex M0": the datasheet says M0+. esp32-c6c1 listed as "ESP32": it is RISC-V. | iot-lab.github.io/docs/boards [V] |

Revised platform grouping (hypotheses):
- **No data cache**: m3 (STM32F103), nrf52840 (I-cache only).
- **Small flash cache that caches data**: samr21 (64 B), st-iotnode (256 B D-cache), rpi-pico (16 KB XIP), ESP32/C6 (32 KB).
- **CPU data caches**: x86, a8, rpi3.

## 3. x86 server

- Contabo VPS: KVM guest, "AMD EPYC Processor" family 23 model 1 (Zen 1), 8 vCPU.
  - Caches: L1d 32 KB per vCPU, L2 512 KB per vCPU, L3 8 MB.
  - Clocksource is `kvm-clock`. The CPU flags show `rdtscp` and `tsc_known_freq`, but `constant_tsc` is not shown.
  - gcc 13.3.0, kernel 6.8.0-147.
  - No valgrind is installed.
- As of 19 Oct, load is 8.00: tmux `queue2` (AND-count x^17) is using all cores until about 22 Oct. Timing runs before that would be noisy and would slow the queue.
- It is a VM, so rdtsc is virtualised and there are noisy neighbours. The paper must call this platform "x86-64 KVM guest", not bare metal.

## 4. IoT-LAB tooling (current docs + cli-tools 3.2.1 source)

- Auth: `iotlab-auth -u <login>` stores credentials in `~/.iotlabrc`. Existing key: `~/.ssh/iotlab_rsa`. Prior login: see `dipper-mcu-bench/riot/iotlab_run.sh`.
- Availability: `iotlab-status --nodes --site <s> --archi <a> --state Alive`. Use `--nodes-ids` for compact output.
- Submit:
  - Physical: `iotlab-experiment submit -n NAME -d MIN -l saclay,nrf52840dk,3+7`.
  - Alias: `-l 2,archi=samr21:at86rf233+site=saclay`.
  - One submit cannot mix the two forms.
- Then `iotlab-experiment wait`, `get -d` (deployment) or `-n`, and `stop`. `get -r` is deprecated.
- Flashing: `iotlab-node --flash fw.{elf,bin} -l site,archi,id`. `--update` is deprecated. A `.bin` file is written at offset 0 plus the board ROM start.
- ESP32 / ESP32-C6:
  - Submit without firmware, then `iotlab-node --flash merged.bin`.
  - To build the merged image, take the `write_flash` line and change it to `esptool merge_bin -o merged.bin ...`.
  - Serial runs at 115200, with `crnl` translation.
- rpi-pico: flashed as `.elf` via OpenOCD + Debug Probe. RIOT testbed makefiles have no rpi-pico/esp mapping.
- Serial:
  - Port 20000 is **exclusive**, one client at a time. The earlier "Connection refused" on nrf52840dk-10 is consistent with another client being connected. No `crnl` on J-Link nodes, so firmware must print `\r\n` and flush.
  - `serial_aggregator -i ID -l site,archi,ids` prints `ts;node;msg`.
- Linux nodes:
  - From the frontend, `ssh root@node-a8-<id>`. Use `~/shared` (NFS) for binaries.
  - The image probably has gcc and valgrind (`tools-sdk` in the recipe), but this needs checking on a live node.
  - The documented kernel version (3.19) disagrees with the repo (5.4). Check with `uname -a`.
- Sites:
  - samr21: Saclay 15, Lille 5.
  - nrf52840dk: Saclay 10.
  - st-iotnode: Saclay 10.
  - rpi-pico: Saclay 10.
  - esp32-c6c1: Strasbourg 5.
  - lopy4: Strasbourg 30.
  - rpi3: Grenoble 5.
  - m3 and a8: many sites.
- ESP-IDF is not on the frontend, so build locally or on the server.

## 5. Literature (for the later paper)

- dudect: Reparaz, Balasch, Verbauwhede, DATE 2017, DOI 10.23919/DATE.2017.7927267, ePrint 2016/1123 [V].
  - Code details [V]:
    - 100 crop percentiles `p_i = 1 − 0.5^(10(i+1)/100)`, taken from the first batch.
    - The test set is: raw, 100 cropped, and one 2nd-order test (after 10^4 samples).
    - The reported value is the max |t| over tests with more than 10^4 samples.
    - Thresholds: 10 means "probably not CT", 500 means "definitely". The paper itself uses 4.5.
- TVLA: Goodwill, Jun, Jaffe, Rohatgi, NIST NIAT 2011 [P]. Schneider–Moradi, CHES 2015, ePrint 2015/207 [V].
- ctgrind: Langley 2010 blog [V]. TIMECOP (SUPERCOP) [V].
- ct-verif: Almeida et al., USENIX Sec 2016 [V].
- Binsec/Rel: Daniel–Bardin–Rezk, S&P 2020 [V]. It shows that compilers introduce CT violations, which supports reading the compiled `ct` code.
- Jancar et al., S&P 2022, ePrint 2021/1650 [V]. Geimer et al., CCS 2023 (tool evaluation) [V].
- Cache attacks:
  - Bernstein 2005 [V].
  - Osvik–Shamir–Tromer, CT-RSA 2006 [V], and J. Cryptology 2010 [V].
  - Bogdanov et al., CT-RSA 2010 (embedded) [V].
  - ARMageddon, USENIX Sec 2016 [P].
- Closest precedent: Blatchley Hansen, Hoy Nielsen, Eskildsen, "Toolchain for Timing Leakage Analysis of NIST LWC Candidates", NIST LWC Workshop 2020 [V].
  - It ran dudect + ctgrind on x86 only.
  - GIFT/SKINNY-based reference code was flagged for S-box table indexing.
- GIFT: Banik et al., CHES 2017 [V]. Fixslicing: Adomnicai–Najm–Peyrin, TCHES 2020(3) [V].
- MCU context:
  - Schwabe–Stoffelen, SAC 2016 (flash wait states and prefetch on M3/M4) [V].
  - BUSted (Cortex-M33 bus contention) [P].
- FIT IoT-LAB: Adjih et al., WF-IoT 2015, DOI 10.1109/WF-IoT.2015.7389098 [V].
- **Gap**: searches found no peer-reviewed timing evaluation through MCU flash caches (ART, ESP32 cache, XIP, NVM cache), no dudect on MCUs, and no IoT-LAB timing/side-channel use. Phrase as "to our knowledge", not as a proven first.
