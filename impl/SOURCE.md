# Source of the upstream files

`dipper.c`, `dipper_fast.c` and `selftest_upstream.c` (= `timing/selftest.c`) are copied unchanged from
https://github.com/AlexGuseinov/Dipper-lightweight-cipher at commit
`3ae6788c7348749eed2113b05fb6c5cfb2651a9e` (folder `timing/`). The folder is unchanged since `26d0f05`, the commit used by `dipper-mcu-bench`.

| file | sha256 |
|---|---|
| dipper.c | e4867aabb5c796614318e4178e84a2ed5b5bdd28ef9a24fa6479411fb0bac129 |
| dipper_fast.c | bb3344794a02fbdf64049d80885a47c5eac45bebbe7fc24804c572d1e6dea7fe |
| selftest_upstream.c | d591b03688f901d9a34fc3235603f0f94ed5b59339e6f974645a0255a5acb71a |

Files written for this study: `dipper_fast_const.c`, `dipper_ct.c`, `dipper_leaky.c`,
`variants.[ch]`, `check_tables.c`, and the generated `fast_const_tables.h`, `ct_perm.h` (from `gen_tables.py`).
