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

## Round 2 (ChatGPT, on commit dd75ca1)

| # | Point | Action |
|---|---|---|
| 1 | Placement raw logs not in the repository | My mistake: the `.log` files were git-ignored and no `.xz` copies were committed. Added the 14 logs as `.log.xz` (17 MB), plus the symbol maps of all placement builds (`results/raw/nm/placement/`). Added `make verify`: placement gives 168/168 exact (`results/summary/recompute_placement.txt`). |
| 2 | "16-byte buffer found" is too definite; microbenchmark code is not in RAM in the `ramfunc` build | Reworded to "16-byte block-dependent flash-read timing, consistent with a data read buffer". The paper now states that the microbenchmark code runs from flash in all builds. This is confirmed by the symbol map: `mb_pair` is at 0x1F50, the cipher functions at 0x2000xxxx. |
| 3 | nRF effect differs between configurations | Text now says the verdicts are the same, so the I-cache is not necessary for the leak. It also says the size and sign change (−6.0 / +1.3 / −6.4 cycles), so instruction fetches still influence the timing. |
| 4 | RP2040 mechanism not shown | Abstract, §5.4, Discussion and Conclusion now separate the three boards with a microbenchmark explanation from the RP2040, where only the placement dependence is shown. |
| 5 | "Smallest detectable difference" is not power | Renamed to "mean-shift threshold" (the expected t reaches 4.5; detected about half of the time). The assumption of stable variance is stated. |
| 6 | Damaged dumps in effect size | `effect_size.py` now uses only intact dumps (244 of 260). Table 4 is unchanged, because none of its runs had damage. |
| 7 | "compensates", "bound where attacks are possible"; Table 5 too small | Changed to "reduces this risk but does not formally control it" and "identify potential timing-attack surfaces". The placement table is split into Test A and Test B. |
| refs | ESP TRM years; Blackman–Vigna; ARMageddon; full titles; Nordic link; workshop links | ESP32 TRM v5.8 is now 2026 (PDF created July 2026) and ESP32-C6 TRM v1.2 is 2026 (March 2026), both checked in PDF metadata. Other changes: Article 36 added; ARMageddon pp. 549–564 (USENIX page); full GIFT and Fixslicing titles (Crossref); Nordic NVMC page URL; Hansen and Goodwill PDF URLs. |
