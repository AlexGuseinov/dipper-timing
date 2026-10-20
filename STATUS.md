# STATUS

## 2026-10-19 (evening, Baku time)

**Done**
- Research phase: see `docs/RESEARCH_NOTES.md`.
- Repo `dipper-timing` created. Upstream Dipper code copied at `3ae6788`; the sha256 values are recorded in `impl/SOURCE.md`.
- Variants: `small`, `fast` (verbatim), `fast_const`, `ct`, `leaky`.
  - `fast_const` uses generated const tables, checked byte-identical to `fast` (5120 B).
  - `ct`:
    - GIFT S-box formula, checked for all 16 inputs.
    - Permutation as 25 fixed mask/shift pairs, checked against the bit loop.
    - Passes TV1–TV4.
    - Matches `small` on 10^5 random (key, plaintext) pairs, round keys included.
  - `leaky`: positive control. It branches on bit 0 of (plaintext XOR rk[0]) and adds 32 dummy iterations.
- dudect core (`common/dudect.c`), shared by all platforms:
  - Welford statistics and the Welch t-test.
  - 10 crop levels p_k = 1−0.5^k, plus no-crop and a 2nd-order test.
  - Raw dump of the first 131072 samples.
  - Host recompute `analysis/recompute.py` gives **bit-identical** max |t| on macOS (dev) and IoT-LAB m3.
- Method gate passed:
  - Mac dev run: `leaky` detected in A and B.
  - m3 dev run (N = 3·10^4): `leaky` |t| = 6566 (A) and 15379 (B). The other variants gave identical cycle counts for every sample.

**Running** (started 19 Oct ~20:40 Baku; N = 10^6 per variant and test; seed 1 on node A, seed 2 on node B)
- IoT-LAB exp 455669, Grenoble, 5 h: m3-4 (s1), m3-11 (s2).
- IoT-LAB exp 455668, Saclay, 10 h:
  - samr21-3 / samr21-12
  - nrf52840dk-4 / nrf52840dk-8
  - st-iotnode-2 / st-iotnode-7
  - rpi-pico-2 / rpi-pico-7
- NodeMCU-32S on the Mac: seed 1, Wi-Fi off. Next on this board: seed 2, then Wi-Fi on.

**x86 done (19 Oct ~21:35 Baku)**: Contabo KVM guest, gcc 13.3 -O2, rdtsc+lfence, pinned to CPU 3 or CPU 6.
- Ali stopped the AND-count queue so the server was free.
- Static check (Valgrind 3.22 memcheck, ctgrind-style), `results/raw/ctgrind/SUMMARY.txt`:
  - small, fast and fast_const are flagged for secret-indexed loads, in both the key schedule and encryption.
  - leaky is flagged for a secret branch.
  - **ct: 0 reports.**
  - Limitation: memcheck treats a value loaded from a secret address as defined, so in `fast` the `PT[]` lookup after `SB2[]` is not reported separately.
- Timing:
  - Seeds 1 and 2 at N=10^6.
  - Seeds 3 and 4 at N=10^7 (extra).
  - Host recompute: 40/40 exact.
  - leaky is detected in all 4 runs. The gate passed.
  - At N=10^7: small and ct have max |t| ≤ 2.1. fast A gives 8.05 / 88.4 and fast B 5.35 / 20.7. fast_const A gives 9.20 / 33.8.

**ESP32 (NodeMCU-32S), Wi-Fi off, seed 1 done**:
- small, fast, fast_const and ct have max |t| ≤ 2.9. leaky gives 704.
- Recompute: 10/10 exact.
- Seed 2 is running.

**Not started**
- ESP32-C6 / LoPy4: Strasbourg. SSH host key not accepted yet; ESP-IDF builds are ready.
- a8 / rpi3: not started.

**Notes**
- Dev logs are kept in `results/dev/` and are not results.
- IoT-LAB logs begin with a short tail of the run started by flashing, followed by the run started by the reset. Analysis uses the last `#BEGIN` block.

## 2026-10-20 00:45 Baku
- Finished, logs in `results/raw/`:
  - IoT-LAB: m3, samr21, st-iotnode and rpi-pico, both seeds, 10/10 pairs.
  - nrf52840dk: both seeds have 10/10 pairs. Its serial drops bytes and loses the `#END`/`#BEGIN` lines around `ztimer_sleep`; the first full block is complete.
  - ESP32 Wi-Fi off: seeds 1 and 2.
- Both IoT-LAB experiments (455668, 455669) stopped.
- The ESP32 Wi-Fi-on run (seed 3) was stopped early because Ali unplugged the board for the night. The partial log is in `results/dev/`, and the run has to be redone.
- Next:
  - recompute all logs on the host;
  - build the summary table (`RESULTS.md`);
  - redo ESP32 Wi-Fi on;
  - Strasbourg ESP32-C6 / LoPy4;
  - a8 / rpi3.

## 2026-10-20 ~10:40 Baku: all planned runs finished
- Added since last entry:
  - ESP32 NodeMCU Wi-Fi scan load (seed 3, single run).
  - ESP32-C6: esp32-c6c1-1 s1, -3 s2.
  - LoPy4: lopy4-17 s1, lopy4-5 s2 (rerun after an accidental power cycle).
  - a8: a8-10 s1, a8-40 s2.
- rpi3 could not be run: deployment failed three times on all four nodes, and power-cycle commands failed too.
- All IoT-LAB experiments stopped: 455668, 455669, 455731, 455732, 455733, 455734, 455735.
- Host recompute over 230 runs: 211 bit-identical, 2 agree to 1e-3, 17 not comparable (serial damage on ESP32 links), 0 unexplained.
- Written:
  - `RESULTS.md` (verdicts and group paragraphs) and `ENVIRONMENT.md`;
  - `docs/ct_asm_review.md` (6 ISAs);
  - footprint and stack use;
  - figures in `figures/`.
- Raw logs are committed as `.log.xz` (19 MB); the uncompressed `.log` files stay local and git-ignored.
- Open for Ali:
  - second Wi-Fi-on run?
  - a8 at N=10^7 for the unclear `fast_const` B?
  - target journal (AMA Herald, Word; or JMTE, LaTeX)?

## 2026-10-20 ~11:55 Baku: extra runs done
- ESP32 Wi-Fi scan load, second run (seed 4). Both Wi-Fi runs are clean except `leaky`, which is now a repeated leak.
- a8 at N = 10^7 (seeds 3 and 4, a8-10 and a8-40, exp 455738, stopped):
  - `fast_const` gives a repeated leak in A (6.7 / 9.0) and B (13.8 / 9.6).
  - `fast`, with the same tables in `.bss`, does not leak.
  - `small` and `ct` are clean.
- Host recompute: 260 runs. 240 bit-identical, 2 to 1e-3, 18 serial-damaged; 0 unexplained.
- `RESULTS.md`, `ENVIRONMENT.md`, tables and figures updated.
- Open for Ali: target journal (AMA Herald or JMTE), then the paper.

## 2026-10-20 ~19:15 Baku: review revision
- Every point of the ChatGPT review was addressed; see `docs/REVIEW_RESPONSE.md`.
- Welch zero-variance bug fixed; no impact on the reported data.
- Placement experiment (exp 455791 and 455792, stopped) and lookup microbenchmark done.
- Effect sizes added.
- Paper rewritten. `RESULTS.md` §8–10 added.

## 2026-10-20 19:59 Baku: submitted
- Submitted to the Journal of Modern Technology and Engineering (Jomard Publishing), manuscript ID 1967, status "Awaiting approval".
- Title: "Timing-Leakage Assessment of Dipper Software Implementations".
- Authors: Huseynli (corresponding), Imamverdiyev, Alizadeh.
