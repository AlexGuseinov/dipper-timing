#!/usr/bin/env python3
"""Reset an ESP32 board over USB serial and log its output until "#END".

Each line is written as "receive_time;tag;line" (same format as the IoT-LAB
logs). The reset uses the usual DTR/RTS auto-reset circuit (EN low, IO0 high).

  .venv/bin/python scripts/esp/capture.py /dev/cu.usbserial-0001 results/raw/x.log [tag]
"""
import sys
import time

import serial

port, log = sys.argv[1], sys.argv[2]
tag = sys.argv[3] if len(sys.argv) > 3 else "esp32"
s = serial.Serial(port, 115200, timeout=1)
s.dtr = False          # IO0 high: normal boot
s.rts = True           # EN low: hold in reset
time.sleep(0.2)
s.reset_input_buffer()
s.rts = False          # release reset
seen_begin = False
with open(log, "w") as f:
    buf = b""
    while True:
        chunk = s.read(4096)
        if not chunk:
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode("ascii", "replace").rstrip("\r")
            f.write("%.3f;%s;%s\n" % (time.time(), tag, line))
            f.flush()
            if line.startswith("#BEGIN"):
                seen_begin = True
            if seen_begin and line.startswith("#END"):
                sys.exit(0)
