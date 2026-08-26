//
// Created by mr on 6/8/2024.
//

#include "SD_card.h"
#include "main_ra.h"
#include "config.h"

#include <SPI.h>
#include "SdFat.h"
#include "logger.h"
#include "config_summary.h"
#include "log_buffer.h"
#include <cstring>

SdFat SD;
File file;
char line[40];
const char* delim = ",";

void openFile(int rule);

struct ConfigEntry {
	const char* key;
	bool* flag;
};

static ConfigEntry configEntries[] = {
	{"USE_ADAFRUIT", &main::use_adafruit},
	{"USE_U8G2", &main::use_u8g2},
	{"SMALL", &main::small},
	{"DISPLAY_DEMO", &main::display_demo},
	{"USE_ROUND", &main::use_round},
	{"USE_MENU", &main::use_menu},
	{"LOG_DEBUG", &main::log_debug},
	{"USE_PS4", &main::use_ps4},
	{"USE_SD_CARD", &main::use_sd_card},
	{"USE_GYRO", &main::use_gyro},
	{"USE_COMPASS", &main::use_compass},
	{"USE_BAROMETER", &main::use_barometer},
	{"USE_DISTANCE", &main::use_distance},
	{"USE_I2C_SCANNER", &main::use_i2c_scanner},
	{"USE_PWM_BOARD", &main::use_pwm_board},
	{"USE_DOT", &main::use_dot},
	{"USE_AUDIO", &main::use_audio},
	{"USE_SWITCH", &main::use_switch},
	{"USE_ANALOG", &main::use_analog},
	{"USE_LIGHT", &main::use_light},
	{"USE_ROBOT", &main::use_robot},
	{"USE_TIMERS", &main::use_timers},
	{"USE_MATRIX", &main::use_matrix},
	{"USE_MATRIX_PREVIEW", &main::use_matrix_preview},
	{"READ_ESP32", &main::read_esp32},
	{"USE_LCD", &main::use_lcd},
	{"USE_OLED_SENSORS", &main::use_oled_sensors},
};

static bool applyConfigKey(const char* key, int value) {
	if (strcmp(key, "USE_MIC") == 0) {
		main::use_audio = (value == 1);
		return true;
	}
	if (strcmp(key, "USE_HM_10_BLE") == 0 || strcmp(key, "USE_IRREMOTE") == 0) {
		return true;
	}

	for (size_t i = 0; i < sizeof(configEntries) / sizeof(configEntries[0]); ++i) {
		if (strcmp(key, configEntries[i].key) == 0) {
			*configEntries[i].flag = (value == 1);
			return true;
		}
	}
	return false;
}

static void logAllConfigFlags() {
	const size_t n = sizeof(configEntries) / sizeof(configEntries[0]);
	char line[LogBuffer::kLineLength];
	for (size_t i = 0; i < n; i += 2) {
		const bool hasSecond = (i + 1) < n;
		formatFlagPair(
				line,
				sizeof(line),
				configDisplayName(configEntries[i].key),
				*configEntries[i].flag,
				hasSecond ? configDisplayName(configEntries[i + 1].key) : "",
				hasSecond,
				hasSecond ? *configEntries[i + 1].flag : false);
		logger::logln(line);
	}
}

//------------------------------------------------------------------------------
void logSdError(const char* msg) {
	logger::logln(msg);
}
//------------------------------------------------------------------------------
void SD_card::initSD() {
	if (!SD.begin(CS_PIN, SD_SCK_MHZ(16))) {
		logger::logln("SD fail");
		main::use_sd_card = false;
	} else {
		logger::logln("SD ok");
		main::use_sd_card = true;
		configLoadSD();
	}
}

//------------------------------------------------------------------------------
char* skipSpace(char* str) {
	while (isspace(static_cast<unsigned char>(*str))) {
		str++;
	}
	return str;
}
//------------------------------------------------------------------------------

bool parseLine(char* str) {
	char* key = strtok(str, delim);
	if (!key) {
		return false;
	}

	char* valStr = strtok(nullptr, delim);
	if (!valStr) {
		return false;
	}

	char* ptr;
	int32_t i32 = strtol(valStr, &ptr, 0);
	if (valStr == ptr || *skipSpace(ptr)) {
		return false;
	}

	if (!applyConfigKey(key, static_cast<int>(i32))) {
		#if LOG_VERBOSE
			logger::log("SD unknown key: ");
			logger::logln(key);
		#endif
	}

	return strtok(nullptr, delim) == nullptr;
}
//------------------------------------------------------------------------------

void SD_card::configLoadSD() {
	openFile(FILE_WRITE);
	if (!file) {
		return;
	}
	file.rewind();

	while (file.available()) {
		int n = file.fgets(line, sizeof(line));
		if (n <= 0) {
			logSdError("fgets failed");
			break;
		}
		if (line[n - 1] != '\n' && n == (static_cast<int>(sizeof(line)) - 1)) {
			logSdError("line too long");
			continue;
		}
		if (!parseLine(line)) {
			logSdError("parseLine failed");
			continue;
		}
	}

	logger::logln("SD loaded");
	logAllConfigFlags();
	file.close();
}

void SD_card::configSaveSD() {
	openFile(FILE_WRITE);
	if (!file) {
		return;
	}
	logger::logln("Saving to SD");
	file.rewind();

	String use_adafruit_string =	 	"USE_ADAFRUIT," + 		String(main::use_adafruit) + "\r\n";
	String use_u8g2_string =		 	"USE_U8G2," + 			String(main::use_u8g2) + "\r\n";
	String small_string =			 	"SMALL," + 				String(main::small) + "\r\n";
	String display_demo_string =	 	"DISPLAY_DEMO," + 		String(main::display_demo) + "\r\n";
	String use_round_string =		 	"USE_ROUND," + 			String(main::use_round) + "\r\n";
	String use_menu_string =		 	"USE_MENU," + 			String(main::use_menu) + "\r\n";
	String log_debug_string =		 	"LOG_DEBUG," + 			String(main::log_debug) + "\r\n";
	String use_ps4_string =			 	"USE_PS4," + 			String(main::use_ps4) + "\r\n";
	String use_sd_card_string =		 	"USE_SD_CARD," + 		String(main::use_sd_card) + "\r\n";
	String use_gyro_string =		 	"USE_GYRO," + 			String(main::use_gyro) + "\r\n";
	String use_compass_string =		 	"USE_COMPASS," + 		String(main::use_compass) + "\r\n";
	String use_barometer_string =	 	"USE_BAROMETER," + 		String(main::use_barometer) + "\r\n";
	String use_distance_string =	 	"USE_DISTANCE," + 		String(main::use_distance) + "\r\n";
	String use_i2c_scanner_string =	 	"USE_I2C_SCANNER," + 	String(main::use_i2c_scanner) + "\r\n";
	String use_pwm_board_string =	 	"USE_PWM_BOARD," + 		String(main::use_pwm_board) + "\r\n";
	String use_dot_string =			 	"USE_DOT," + 			String(main::use_dot) + "\r\n";
	String use_audio_string =		 	"USE_AUDIO," + 			String(main::use_audio) + "\r\n";
	String use_switch_string =		 	"USE_SWITCH," + 		String(main::use_switch) + "\r\n";
	String use_analog_string =		 	"USE_ANALOG," + 		String(main::use_analog) + "\r\n";
	String use_light_string =		 	"USE_LIGHT," + 			String(main::use_light) + "\r\n";
	String use_robot_string =		 	"USE_ROBOT," + 			String(main::use_robot) + "\r\n";
	String use_timers_string =		 	"USE_TIMERS," + 		String(main::use_timers) + "\r\n";
	String use_matrix_string =		 	"USE_MATRIX," + 		String(main::use_matrix) + "\r\n";
	String use_matrix_preview_string =	"USE_MATRIX_PREVIEW," + String(main::use_matrix_preview) + "\r\n";
	String read_esp32_string =		 	"READ_ESP32," + 		String(main::read_esp32) + "\r\n";
	String use_lcd_string =			 	"USE_LCD," + 			String(main::use_lcd) + "\r\n";
	String use_oled_sensors_string =	"USE_OLED_SENSORS," + 	String(main::use_oled_sensors) + "\r\n";

	file.print(F(
			use_adafruit_string +
			use_u8g2_string +
			small_string +
			display_demo_string +
			use_round_string +
			use_menu_string +
			log_debug_string +
			use_ps4_string +
			use_sd_card_string +
			use_gyro_string +
			use_compass_string +
			use_barometer_string +
			use_distance_string +
			use_i2c_scanner_string +
			use_pwm_board_string +
			use_dot_string +
			use_audio_string +
			use_switch_string +
			use_analog_string +
			use_light_string +
			use_robot_string +
			use_timers_string +
			use_matrix_string +
			use_matrix_preview_string +
			read_esp32_string +
			use_lcd_string +
			use_oled_sensors_string
   ));

	file.close();
	logger::logln("Save Complete");
}

void openFile(int rule) {
	file = SD.open("SETUP.TXT", rule);
	if (!file) {
		logSdError("open failed");
	}
}

static File sDataFile;

bool SD_card::fileExists(const char* path) {
	if (!main::use_sd_card) {
		return false;
	}
	return SD.exists(path);
}

bool SD_card::openDataFile(const char* path, uint8_t mode) {
	if (!main::use_sd_card) {
		return false;
	}
	if (sDataFile) {
		sDataFile.close();
	}
	sDataFile = SD.open(path, mode);
	return static_cast<bool>(sDataFile);
}

void SD_card::closeDataFile() {
	if (sDataFile) {
		sDataFile.close();
	}
}

void SD_card::printLine(const char* line) {
	if (!sDataFile) {
		return;
	}
	sDataFile.println(line);
}
