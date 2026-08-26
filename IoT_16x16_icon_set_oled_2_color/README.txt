IoT 16x16 bi-color icon set
============================
Author: Artur Funk (GPLv3)
Origin: http://engsta.com/iot-icon-set-for-i2c-oled-displays/

The original `iot_iconset_16x16.c` / `.h` in this folder are the vendor samples
(broken include / non-extern declarations). The firmware uses a cleaned PROGMEM
subset instead:

  src/iot_iconset_16x16.h
  src/iot_iconset_16x16.cpp

Regenerate the subset with:

  python tools/gen_iot_icons.py
