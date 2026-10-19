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

**Not started**
- x86: waits until the AND-count queue on Contabo finishes (~22 Oct).
- ESP32-C6 / LoPy4: Strasbourg. SSH host key not accepted yet; ESP-IDF builds are ready.
- a8 / rpi3: not started.

**Notes**
- Dev logs are kept in `results/dev/` and are not results.
- IoT-LAB logs begin with a short tail of the run started by flashing, followed by the run started by the reset. Analysis uses the last `#BEGIN` block.
