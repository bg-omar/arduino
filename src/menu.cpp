//
// Created by mr on 6/12/2024.
//

#include "menu.h"
#include "main_ra.h"
#include "sense_react.h"
#include "avoid_objects.h"

#include "logger.h"
#include "SD_card.h"
#include "displayAdafruit.h"
#include "menu_draft.h"
#include "oled_mode.h"
#include "iot_iconset_16x16.h"

const int NUM_ITEMS = 27;
const int MAX_ITEM_LENGTH = 20;

char menu_items [NUM_ITEMS] [MAX_ITEM_LENGTH] = {
		{ "use_adafruit" },
		{ "use_u8g2" },
		{ "small" },
		{ "display_demo" },
		{ "use_round" },
		{ "use_menu" },
		{ "log_debug" },
		{ "use_ps4" },
		{ "use_sd_card" },
		{ "use_gyro" },
		{ "use_compass" },
		{ "use_barometer" },
		{ "use_distance" },
		{ "use_i2c_scanner" },
		{ "use_pwm_board" },
		{ "use_dot" },
		{ "use_audio" },
		{ "use_switch" },
		{ "use_analog" },
		{ "use_light" },
		{ "use_robot" },
		{ "use_timers" },
		{ "use_matrix" },
		{ "use_matrix_preview" },
		{ "read_esp32" },
		{ "use_lcd" },
		{ "use_oled_sensors" }
};

struct MenuFlagEntry {
	const char* label;
	bool* flag;
};

static MenuFlagEntry menuFlags[NUM_ITEMS] = {
		{ "use_adafruit: ", &main::use_adafruit },
		{ "use_u8g2: ", &main::use_u8g2 },
		{ "small: ", &main::small },
		{ "display_demo: ", &main::display_demo },
		{ "use_round: ", &main::use_round },
		{ "use_menu: ", &main::use_menu },
		{ "log_debug: ", &main::log_debug },
		{ "use_ps4: ", &main::use_ps4 },
		{ "use_sd_card: ", &main::use_sd_card },
		{ "use_gyro: ", &main::use_gyro },
		{ "use_compass: ", &main::use_compass },
		{ "use_barometer: ", &main::use_barometer },
		{ "use_distance: ", &main::use_distance },
		{ "use_i2c_scanner: ", &main::use_i2c_scanner },
		{ "use_pwm_board: ", &main::use_pwm_board },
		{ "use_dot: ", &main::use_dot },
		{ "use_audio: ", &main::use_audio },
		{ "use_switch: ", &main::use_switch },
		{ "use_analog: ", &main::use_analog },
		{ "use_light: ", &main::use_light },
		{ "use_robot: ", &main::use_robot },
		{ "use_timers: ", &main::use_timers },
		{ "use_matrix: ", &main::use_matrix },
		{ "use_matrix_preview: ", &main::use_matrix_preview },
		{ "read_esp32: ", &main::read_esp32 },
		{ "use_lcd: ", &main::use_lcd },
		{ "use_oled_sensors: ", &main::use_oled_sensors },
};

static bool sMenuDraftBackup[NUM_ITEMS];
static OledMode sModeBeforeMenu = OledMode::Log;
static bool sMenuSessionOpen = false;

static bool* menuFlagPtrs[NUM_ITEMS];

static void initMenuFlagPtrs() {
	static bool ready = false;
	if (ready) {
		return;
	}
	for (int i = 0; i < NUM_ITEMS; ++i) {
		menuFlagPtrs[i] = menuFlags[i].flag;
	}
	ready = true;
}


// note - when changing the order of items above, make sure the other arrays referencing bitmaps
// also have the same order, for example array "bitmap_icons" for icons, and other arrays for screenshots and QR codes


int item_selected = 0; // which item in the menu is selected

int item_sel_previous; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_next; // next item - used in the menu screen to draw next item after the selected one

int current_screen = 0;   // 0 = menu, 1 = screenshot, 2 = qr

int demo_mode_state = 0; // demo mode state = which screen and menu item to display
int demo_mode_delay = 0; // demo mode delay = used to slow down the screen switching

// 'scrollbar_background', 8x64px
constexpr static const unsigned char bitmap_scrollbar_background [] PROGMEM = {
		0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
		0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
		0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
		0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
		0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
		0x00, 0x40, 0x00, 0x00, };


// 'item_sel_outline', 128x21px
constexpr static const unsigned char bitmap_item_sel_outline [] PROGMEM = {
		0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0C, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0xF8, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03,
};


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


void menu::loopMenu() {
	if (main::display_demo) { // when demo mode is active, automatically switch between all the screens and menu items
		displayAdafruit::setMode(OledMode::Menu);
		demo_mode_delay++; // increase demo mode delay
		if (demo_mode_delay > 15) { // after some time, switch to another screen - change this value to make it slower/faster
			demo_mode_delay = 0;
			demo_mode_state++; // increase counter
			if (demo_mode_state >= NUM_ITEMS*3) {demo_mode_state=0;} // jump back to the first screen
		}

		if (demo_mode_state % 3 == 0) {current_screen = 0; item_selected = demo_mode_state/3; } // menu screen
	} // end demo mode section

	if (displayAdafruit::getMode() != OledMode::Menu) {
		return;
	}
	if (!displayAdafruit::isMenuDirty() && !main::display_demo) {
		return;
	}



	// set correct values for the previous and next items
	item_sel_previous = item_selected - 1;
	if (item_sel_previous < 0) {item_sel_previous = NUM_ITEMS - 1;} // previous item would be below first = make it the last
	item_sel_next = item_selected + 1;
	if (item_sel_next >= NUM_ITEMS) {item_sel_next = 0;} // next item would be after last = make it the first



	displayAdafruit::display.clearDisplay();  // clear buffer for storing display content in RAM
	displayAdafruit::display.setFont(&TomThumb);

	if (current_screen == 0) { // MENU SCREEN

		// selected item background
		displayAdafruit::display.drawBitmap(0, 22, bitmap_item_sel_outline, 128, 21, SH110X_WHITE);

		// draw previous item as icon + label + ON/OFF
		displayAdafruit::display.setCursor(25, 15);
		displayAdafruit::display.print(menu_items[item_sel_previous]);
		displayAdafruit::display.drawBitmap(4, 2, bitmap_icons[item_sel_previous], 16, 16, SH110X_WHITE);
		displayAdafruit::display.drawBitmap(
			108, 2,
			*menuFlags[item_sel_previous].flag ? check_icon16x16 : cancel_icon16x16,
			16, 16, SH110X_WHITE);

		// draw selected item as icon + label + check/cancel
		displayAdafruit::display.setCursor(25, 15+20+2);
		displayAdafruit::display.print(menu_items[item_selected]);
		displayAdafruit::display.drawBitmap(4, 24, bitmap_icons[item_selected], 16, 16, SH110X_WHITE);
		displayAdafruit::display.drawBitmap(
			108, 24,
			*menuFlags[item_selected].flag ? check_icon16x16 : cancel_icon16x16,
			16, 16, SH110X_WHITE);

		// draw next item as icon + label + check/cancel
		displayAdafruit::display.setCursor(25, 15+20+20+2+2);
		displayAdafruit::display.print(menu_items[item_sel_next]);
		displayAdafruit::display.drawBitmap(4, 46, bitmap_icons[item_sel_next], 16, 16, SH110X_WHITE);
		displayAdafruit::display.drawBitmap(
			108, 46,
			*menuFlags[item_sel_next].flag ? check_icon16x16 : cancel_icon16x16,
			16, 16, SH110X_WHITE);

		// draw scrollbar background
		displayAdafruit::display.drawBitmap(128-8, 0, bitmap_scrollbar_background, 8, 64, SH110X_WHITE);

		// draw scrollbar handle
		displayAdafruit::display.drawRect(125, 64/NUM_ITEMS * item_selected, 3, 64/NUM_ITEMS, SH110X_WHITE);

	}

	displayAdafruit::display.display(); // send buffer from RAM to display controller
	if (!main::display_demo) {
		displayAdafruit::clearMenuDirty();
	}
}

static void logFlag(const char* name, bool value) {
	logger::log(name);
	logger::logln(value ? "true" : "false");
}

static void logItem(int item) {
	logger::log("Item: ");
	logger::logIntln(item);
}

void menu::down() {
	if (!sMenuSessionOpen) {
		return;
	}
	displayAdafruit::markMenuDirty();
	if (current_screen == 0) {
		item_selected = item_selected + 1; // select next item
		if (item_selected >= NUM_ITEMS) { // last item was selected, jump to first menu item
			item_selected = 0;
		}
	}
	logItem(item_selected);

}

void menu::up() {
	if (!sMenuSessionOpen) {
		return;
	}
	displayAdafruit::markMenuDirty();
	if (current_screen == 0) {
		item_selected = item_selected - 1; // select previous item
		if (item_selected < 0) { // if first item was selected, jump to last item
			item_selected = NUM_ITEMS - 1;
		}
	}
	logItem(item_selected);
}

void menu::select() {
	if (!sMenuSessionOpen) {
		return;
	}
	displayAdafruit::markMenuDirty();
	if (item_selected < 0 || item_selected >= NUM_ITEMS) {
		return;
	}

	*menuFlags[item_selected].flag = !*menuFlags[item_selected].flag;
	logFlag(menuFlags[item_selected].label, *menuFlags[item_selected].flag);
}

bool menu::isOpen() {
	return sMenuSessionOpen;
}

void menu::toggle() {
	if (sMenuSessionOpen) {
		closeDiscard();
	} else {
		open();
	}
}

void menu::open() {
	initMenuFlagPtrs();
	if (sMenuSessionOpen) {
		closeDiscard();
		return;
	}
	sModeBeforeMenu = displayAdafruit::getMode();
	if (sModeBeforeMenu == OledMode::Menu || sModeBeforeMenu == OledMode::None) {
		sModeBeforeMenu = OledMode::Log;
	}
	menuDraftSnapshot(sMenuDraftBackup, menuFlagPtrs, NUM_ITEMS);
	sMenuSessionOpen = true;
	current_screen = 0;
	displayAdafruit::setMode(OledMode::Menu);
	displayAdafruit::markMenuDirty();
	logger::logln("Menu open");
}

static OledMode oledModeAfterMenu() {
	if (sense_react::isActive()) {
		return OledMode::SenseReact;
	}
	if (avoid_objects::isActive()) {
		return OledMode::Avoid;
	}
	return sModeBeforeMenu;
}

void menu::closeDiscard() {
	if (!sMenuSessionOpen) {
		return;
	}
	initMenuFlagPtrs();
	menuDraftRestore(menuFlagPtrs, sMenuDraftBackup, NUM_ITEMS);
	sMenuSessionOpen = false;
	displayAdafruit::setMode(oledModeAfterMenu());
	logger::logln("Menu close (discard)");
}

void menu::undo() {
	closeDiscard();
}

void menu::save() {
	if (!sMenuSessionOpen) {
		return;
	}
	initMenuFlagPtrs();
	SD_card::configSaveSD();
	menuDraftSnapshot(sMenuDraftBackup, menuFlagPtrs, NUM_ITEMS);
	displayAdafruit::markMenuDirty();
	logger::logln("Menu saved");
}

void menu::toggleSensorsPage() {
	if (!FEATURE_ENABLED(main::use_oled_sensors, USE_OLED_SENSORS)) {
		return;
	}
	if (sMenuSessionOpen) {
		return;
	}
	if (displayAdafruit::getMode() == OledMode::Sensors) {
		displayAdafruit::setMode(OledMode::Log);
	} else {
		displayAdafruit::setMode(OledMode::Sensors);
		displayAdafruit::drawSensorsPage();
	}
}

