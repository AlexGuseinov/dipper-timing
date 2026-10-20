# Response to the external review (ChatGPT, 20 Oct 2026)

Every point was checked against the code and the data, and none was found to be factually wrong.

| # | Point | Action | Where |
|---|---|---|---|
| 1 | `dudect_welch` returns 0 for zero variance with different means | Fixed in device and host code (±∞); regression test added. All 244 comparable raw dumps were checked: 0 affected tests, no verdict changes. | `common/dudect.c`, `analysis/recompute.py`, `tests/`, RESULTS §10, paper §4 |
| 2 | "Bit-exact verification" claim is too broad | Narrowed. The check covers the first 131072 samples and the device arithmetic, not the full run or the method. Counts: 240/2/18 for the main experiment, 168/168 for the placement experiment. | paper Abstract, §4, §5.2 |
| 3 | RAM/flash comparison is not isolated (`init_fast`, `const` codegen, addresses, key schedule `S` in flash) | New controlled experiment: same code, table pointer pointing to RAM or flash, for both `S` and the fast tables, with the key schedule included. It ran on 5 MCUs, 2 nodes each. | `impl/dipper_placement.c`, paper §3, §5.3 |
| 4 | nRF52840 result not explained | Ran with the I-cache off, with the code in RAM, and with an aligned `S`, plus a lookup microbenchmark. Found: an undocumented 16-byte flash data buffer (8 vs 11 cycles), independent of the I-cache. The main-build `S` straddled two blocks. | `common/microbench.c`, paper §5.3–5.4 |
| 5 | Threat model | New paragraph: tests show dependence, not key recovery. Test A and Test B are interpreted separately. | paper §4 |
| 6 | ESP32 cache explanation unverified; positive control too large | The cache explanation is now stated as a hypothesis. Added the detectable-difference figure (MDE) for every platform and a 1-iteration control `lk1` (3–7 cycles, detected everywhere). | paper §5.2, Table effect |
| 7 | Speed-up only for encryption with precomputed keys | Both figures are given: 3.5–9.4× for encryption only, 2.2–6.2× for one block per key. | paper Abstract, §5.5 |
| 8 | Novelty level | Framed as an applied measurement study with a controlled separation and a mechanism. The bitsliced formula is attributed to GIFT. | paper §3, §6 |
| minor | Cortex-M7 data cache | The intro now limits the claim to M0+/M3/M4. | paper §1 |
| minor | "eleven platforms" | Changed to ten hardware platforms and eleven configurations. | paper |
| minor | 18 damaged dumps vs nRF serial | Made consistent: all 18 are ESP32 dumps. The nRF link lost only block markers. | paper §5.2, Limitations |
| minor | memcheck is dynamic | Terminology changed. | paper §4, §5.1 |
| minor | Supplementary statistics | Signed t, test index and class means/sd are in `results/summary/runs.csv`, `effect_size.csv`, `placement_runs.csv`. | repo |
| minor | RPi deployment in Limitations | Removed from the paper; kept in `RESULTS.md` §6 as an experiment note. | paper |
