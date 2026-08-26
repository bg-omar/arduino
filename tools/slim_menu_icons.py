from pathlib import Path

path = Path(r"c:\workspace\projects\Arduino_Projects\Arduino-R4_UNO_Wall-Z\src\menu.cpp")
lines = path.read_text(encoding="utf-8").splitlines(keepends=True)

# head: through demo_mode_delay blank lines (line 113 index 112)
head = lines[:113]

# scrollbar + outline: lines 422-462 (1-based) → indices 421:462
chrome = lines[421:462]

# foot: from void menu::loopMenu
foot_idx = next(i for i, ln in enumerate(lines) if ln.startswith("void menu::loopMenu()"))
foot = lines[foot_idx:]

mid = r'''
#include "iot_iconset_16x16.h"

// Menu item icons (IoT 16x16 set) — order matches menu_items / menuFlags.
static const unsigned char* const bitmap_icons[NUM_ITEMS] = {
		window_icon16x16,       // use_adafruit
		window_icon16x16,       // use_u8g2
		minus_icon16x16,        // small
		face_icon16x16,         // display_demo
		bullet_icon16x16,       // use_round
		tool_icon16x16,         // use_menu
		warning_icon16x16,      // log_debug
		bluetooth_icon16x16,    // use_ps4
		bat2_icon16x16,         // use_sd_card
		siren_icon16x16,        // use_gyro
		signal3_icon16x16,      // use_compass
		temperature_icon16x16,  // use_barometer
		humidity2_icon16x16,    // use_distance
		wifi1_icon16x16,        // use_i2c_scanner
		wallplug_icon16x16,     // use_pwm_board
		bulb_on_icon16x16,      // use_dot
		speak_icon16x16,        // use_audio
		powerbutton_icon16x16,  // use_switch
		plug_icon16x16,         // use_analog
		sun_icon16x16,          // use_light
		person_icon16x16,       // use_robot
		timer_icon16x16,        // use_timers
		window_icon16x16,       // use_matrix
		window_icon16x16,       // use_matrix_preview
		wifi2_icon16x16,        // read_esp32
		mobile_icon16x16,       // use_lcd
		humidity_icon16x16,     // use_oled_sensors
};


'''

# Patch includes in head
head_text = "".join(head)
if '#include "iot_iconset_16x16.h"' not in head_text:
    head_text = head_text.replace(
        '#include "oled_mode.h"\n',
        '#include "oled_mode.h"\n#include "iot_iconset_16x16.h"\n',
    )

# Patch foot: ON/OFF text → check/cancel icons
foot_text = "".join(foot)
old_prev = """\t\tdisplayAdafruit::display.setCursor(25, 15);
\t\tdisplayAdafruit::display.print(menu_items[item_sel_previous]);
\t\tdisplayAdafruit::display.setCursor(100, 15);
\t\tdisplayAdafruit::display.print(*menuFlags[item_sel_previous].flag ? "ON" : "OFF");
\t\tdisplayAdafruit::display.drawBitmap( 4, 2, bitmap_icons[item_sel_previous], 16, 16, SH110X_WHITE);


\t\t// draw selected item as icon + label + ON/OFF
\t\tdisplayAdafruit::display.setCursor(25, 15+20+2);
\t\tdisplayAdafruit::display.print(menu_items[item_selected]);
\t\tdisplayAdafruit::display.setCursor(100, 15+20+2);
\t\tdisplayAdafruit::display.print(*menuFlags[item_selected].flag ? "ON" : "OFF");
\t\tdisplayAdafruit::display.drawBitmap( 4, 24, bitmap_icons[item_selected], 16, 16, SH110X_WHITE);

\t\t// draw next item as icon + label + ON/OFF
\t\tdisplayAdafruit::display.setCursor(25, 15+20+20+2+2);
\t\tdisplayAdafruit::display.print(menu_items[item_sel_next]);
\t\tdisplayAdafruit::display.setCursor(100, 15+20+20+2+2);
\t\tdisplayAdafruit::display.print(*menuFlags[item_sel_next].flag ? "ON" : "OFF");
\t\tdisplayAdafruit::display.drawBitmap( 4, 46, bitmap_icons[item_sel_next], 16, 16, SH110X_WHITE);
"""

new_prev = """\t\tdisplayAdafruit::display.setCursor(25, 15);
\t\tdisplayAdafruit::display.print(menu_items[item_sel_previous]);
\t\tdisplayAdafruit::display.drawBitmap(4, 2, bitmap_icons[item_sel_previous], 16, 16, SH110X_WHITE);
\t\tdisplayAdafruit::display.drawBitmap(
\t\t\t108, 2,
\t\t\t*menuFlags[item_sel_previous].flag ? check_icon16x16 : cancel_icon16x16,
\t\t\t16, 16, SH110X_WHITE);

\t\t// draw selected item as icon + label + check/cancel
\t\tdisplayAdafruit::display.setCursor(25, 15+20+2);
\t\tdisplayAdafruit::display.print(menu_items[item_selected]);
\t\tdisplayAdafruit::display.drawBitmap(4, 24, bitmap_icons[item_selected], 16, 16, SH110X_WHITE);
\t\tdisplayAdafruit::display.drawBitmap(
\t\t\t108, 24,
\t\t\t*menuFlags[item_selected].flag ? check_icon16x16 : cancel_icon16x16,
\t\t\t16, 16, SH110X_WHITE);

\t\t// draw next item as icon + label + check/cancel
\t\tdisplayAdafruit::display.setCursor(25, 15+20+20+2+2);
\t\tdisplayAdafruit::display.print(menu_items[item_sel_next]);
\t\tdisplayAdafruit::display.drawBitmap(4, 46, bitmap_icons[item_sel_next], 16, 16, SH110X_WHITE);
\t\tdisplayAdafruit::display.drawBitmap(
\t\t\t108, 46,
\t\t\t*menuFlags[item_sel_next].flag ? check_icon16x16 : cancel_icon16x16,
\t\t\t16, 16, SH110X_WHITE);
"""

if old_prev not in foot_text:
    raise SystemExit("menu draw block not found for patch")
foot_text = foot_text.replace(old_prev, new_prev)

# Avoid duplicate include if we put it in mid only
# Remove from mid since added to head
mid = mid.replace('#include "iot_iconset_16x16.h"\n\n', "")

out = head_text + "".join(chrome) + "\n" + mid + foot_text
path.write_text(out, encoding="utf-8")
print(f"menu.cpp now {len(out.splitlines())} lines (was {len(lines)})")
