
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
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | A | 0.9 / 1.2 | 2.0 / 1.5 | 1.2 / 2.5 | 1.1 / 1.1 | **683.7 / 690.4** |
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | B | 2.0 / 2.6 | 3.1 / 1.6 | *7.6 / 1.5* | 1.7 / 1.3 | **610.5 / 668.9** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | A | 0.5 / 2.2 | **19.9 / 5.6** | **8.6 / 8.3** | 1.0 / 1.5 | **68.7 / 40.9** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | B | 1.6 / 1.6 | 1.8 / 3.6 | *2.3 / 7.4* | 0.9 / 1.1 | **45.3 / 26.2** |

### Extra runs (x86, N = 10^7, seeds 3 and 4)

| platform | test | small | fast | fast_const | ct | leaky |
|---|---|---|---|---|---|---|
| x86-64 (KVM guest, AMD EPYC Zen 1) | A | 2.1 / 1.8 | **8.1 / 88.4** | **9.2 / 33.8** | 1.3 / 1.3 | **1193.5 / 1419.9** |
| x86-64 (KVM guest, AMD EPYC Zen 1) | B | 1.6 / 0.7 | **5.3 / 20.7** | 1.8 / 3.6 | 0.8 / 2.1 | **386.8 / 366.8** |

Cells: max |t| per run (seed 1 / seed 2 …). **bold** = leak, repeated (> 4.5 in every run); *italic* = unclear (> 4.5 in one run only); const. = every measurement had the same cycle count.


### Cost (median per block; cycles, x86: TSC ticks, a8: ns via clock_gettime)

| platform | small enc | fast enc | fast_const enc | ct enc | key schedule (small) | key schedule (ct) |
|---|---|---|---|---|---|---|
| STM32F103 (IoT-LAB m3) | 77616 | 21209 | 26223 | 8937 | 2284 | 4455 |
| nRF52840 (IoT-LAB nrf52840dk) | 54577 | 15202 | 17264 | 6531 | 1636 | 3264 |
| SAMR21G18A (IoT-LAB samr21) | 80247 | 21199 | 22606 | 13527 | 3120 | 8032 |
| STM32L475VG (IoT-LAB st-iotnode) | 53772 | 15196 | 17826 | 5694 | 1534 | 3266 |
| RP2040 (IoT-LAB rpi-pico) | 73613 | 20176 | 20164 | 10356 | 2522 | 6664 |
| ESP32 NodeMCU-32S, Wi-Fi off | 57532 | 18543 | 18560 | 6180 | 1967 | 4368 |
| TI AM3505 Cortex-A8 (IoT-LAB a8, Linux) | 44668 | 17306 | 17180 | 5018 | 1355 | 3322 |
| x86-64 (KVM guest, AMD EPYC Zen 1) | 4821 | 1809 | 1908 | 1388 | 391 | 1003 |
