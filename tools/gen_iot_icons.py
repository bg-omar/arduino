import re
from pathlib import Path

raw = Path(
    r"c:\workspace\projects\Arduino_Projects\Arduino-R4_UNO_Wall-Z"
    r"\IoT_16x16_icon_set_oled_2_color\iot_iconset_16x16.c"
).read_text(encoding="utf-8", errors="replace")
pat = re.compile(r"unsigned char (\w+)\s*\[\s*\]\s*=\s*\{([^}]*)\}", re.S)
icons = pat.findall(raw)
keep = {
    "home_icon16x16",
    "arrow_up_icon16x16",
    "arrow_down_icon16x16",
    "bluetooth_icon16x16",
    "bulb_icon16x16",
    "bulb_on_icon16x16",
    "bulb_off_icon16x16",
    "bullet_icon16x16",
    "cancel_icon16x16",
    "check_icon16x16",
    "speak_icon16x16",
    "heart_icon16x16",
    "nocon_icon16x16",
    "tool_icon16x16",
    "plug_icon16x16",
    "powerbutton_icon16x16",
    "wallplug_icon16x16",
    "wifi1_icon16x16",
    "wifi2_icon16x16",
    "clock_icon16x16",
    "timer_icon16x16",
    "water_tap_icon16x16",
    "humidity_icon16x16",
    "humidity2_icon16x16",
    "sun_icon16x16",
    "temperature_icon16x16",
    "person_icon16x16",
    "warning_icon16x16",
    "siren_icon16x16",
    "mobile_icon16x16",
    "signal3_icon16x16",
    "face_icon16x16",
    "lock_closed_icon16x16",
    "lock_open_icon16x16",
    "window_icon16x16",
    "bat2_icon16x16",
    "plus_icon16x16",
    "minus_icon16x16",
}
byte_re = re.compile(r"0[bB][01]+|0[xX][0-9a-fA-F]+")
h = [
    "// IoT 16x16 icon set (subset) — Artur Funk, GPLv3",
    "// Origin: http://engsta.com/iot-icon-set-for-i2c-oled-displays/",
    "#ifndef IOT_ICONSET_16X16_H",
    "#define IOT_ICONSET_16X16_H",
    "",
    "#include <Arduino.h>",
    "",
]
c = ['#include "iot_iconset_16x16.h"', ""]
count = 0
for name, body in icons:
    if name not in keep:
        continue
    bytes_ = byte_re.findall(body)
    if len(bytes_) != 32:
        raise SystemExit(f"{name} has {len(bytes_)} bytes")
    h.append(f"extern const unsigned char {name}[32] PROGMEM;")
    c.append(f"const unsigned char {name}[32] PROGMEM = {{")
    for i in range(0, 32, 2):
        c.append(f"\t{bytes_[i]}, {bytes_[i+1]},")
    c.append("};")
    c.append("")
    count += 1
h += ["", "#endif // IOT_ICONSET_16X16_H", ""]
base = Path(r"c:\workspace\projects\Arduino_Projects\Arduino-R4_UNO_Wall-Z\src")
(base / "iot_iconset_16x16.h").write_text("\n".join(h), encoding="utf-8")
(base / "iot_iconset_16x16.cpp").write_text("\n".join(c), encoding="utf-8")
print(f"wrote {count} icons")
