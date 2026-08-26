//
// Created by mr on 6/8/2024.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_SD_CARD_H
#define ARDUINO_R4_UNO_WALL_Z_SD_CARD_H


#include "Adafruit_GFX.h"

class SD_card {

public:
	static void initSD();
	static void configLoadSD();

	static void configSaveSD();

	static bool fileExists(const char* path);
	static bool openDataFile(const char* path, uint8_t mode);
	static void closeDataFile();
	static void printLine(const char* line);
};


#endif //ARDUINO_R4_UNO_WALL_Z_SD_CARD_H
