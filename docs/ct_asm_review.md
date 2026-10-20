# Review of the compiled `ct` code (20 Oct 2026)

**What was checked.** The functions `dipper_encrypt_block_ct` and `dipper_expand_128_ct` were disassembled from the ELF files that ran in the experiments; `ct_sub`, `ct_perm` and the rotations are inlined into them. The listings are in `results/raw/asm/`, and `analysis/ct_asm_scan.py` lists every control-flow instruction and every memory access.

For each ISA we checked two things:

1. Every conditional branch depends only on a loop counter or loop pointer.
2. Every load or store uses a public address: the stack, the round-key array at a counter-based offset, the 16-byte key copy, or a constant (literal pool).

| ISA / build | toolchain | `enc`: branches | `enc`: memory | `expand`: branches | `expand`: memory |
|---|---|---|---|---|---|
| x86-64 (Contabo, `dudect_host`) | gcc 13.3 -O2 | 1 `jne`: `cmp %r13,%rsi`, the rk pointer against its end | 1 load `(%rsi)` (rk[i]) | `jmp` + `jne`: `cmp %r9,-0x10(%rsp)`, the loop pointer | stack, key `(%rdi)`, rk store `(%r9)` |
| ARMv7-A (a8, `dudect_host`) | gcc 9.3 -O2 (on node) | 1 `bne` after `cmp r6, r2`, with both reloaded from the stack via `ldm sp,{r2,r6}` (rk pointer, end) | `[sp,#]`, rk `[r2,#]` | 1 `bne` after `cmp r5, lr` (lr = rk+216) | `[sp,#]`, key `[r0]`, rk `[r3,#]`/`strd [lr,#]` |
| Thumb-2 (M3: iotlab-m3; M4: b-l475e-iot01a, nrf52840dk) | arm-none-eabi-gcc 9.3.1 -O2 (RIOT) | 1 `bne.w`, the loop end | sp- and rk-relative with immediate offset, pc-relative literals | `b.n` + `bne.w`, the loop | as left, plus one `bl memcpy` with constant length 16 |
| Thumb-1 / ARMv6-M (M0+: samr21-xpro, rpi-pico) | same | `beq.n` after `cmp r3, r1` (r1 = rk pointer + 8 per round) + `b.n` | as above | `beq.n` after `cmp r1, ip` + `b.n` | as above, `bl memcpy` (16 bytes) |
| Xtensa LX6 (ESP32, ESP-IDF 6.1) | xtensa-esp32-elf-gcc -O2 | 1 `bnez a7`; a7 counts down from 28 | rk `l32i.n a4,0/4` (a4 += 8 per round), `l32r` literal constants | `beq a10,a12` (counter) + `j`, plus `callx8 memcpy` (a12 = 16) | stack `a1+#`, key `l16ui a1,#` |
| RISC-V RV32IMAC (ESP32-C6, ESP-IDF 6.1) | riscv32-esp-elf-gcc -O2 | 1 `bne a2,s11`, the rk pointer against its end | rk `lw 0/4(a2)`, stack `(sp)` | `bne t2,a1`, the loop; `jalr memcpy` | stack, rk |

**Findings.**

- None of the six ISAs has a branch whose condition depends on the key, the round keys or the plaintext. The only data-dependent flag setting is `adds rX,rX,rX` on ARMv7-A, which feeds `adc` in 64-bit shifts. Its flags are overwritten before the loop branch reads them.
- None of the six ISAs has a register-indexed load or store.
  - Thumb and ARMv7-A: no `[rX, rY]` addressing.
  - x86-64: the only `(%rA,%rB)` operands are `nop` padding and `lea` used for multiplication by 2, which do not access memory.
  - Xtensa: no `addx2/4/8`.
  - RISC-V: all loads are `(sp)`- or `(a2)`-relative, where `a2` is the rk pointer.
- No library helper is called for 64-bit shifts, because all shift amounts in `ct` are constants. The only call is `memcpy(K, k, 16)` in the key schedule. It has a constant length, so its branches depend only on the length and on pointer alignment, which are public.
- In the x86-64 code, the only memory read with a secret value in the encryption is the round key itself, at a public address.

**Caveats.**

- This is a reading of the listings plus a scripted scan. It is not formal verification such as ct-verif or Binsec/Rel.
- It covers the instruction level only. Data-dependent instruction timing, for example a multiplier with early termination, is not an issue here because `ct` uses no multiplications.
- Valgrind memcheck (ctgrind-style) found no reports for `ct` on x86-64; see `results/raw/ctgrind/SUMMARY.txt`.
