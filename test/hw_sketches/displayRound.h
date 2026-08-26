//
// Created by mr on 6/3/2024.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_DISPLAYROUND_H
#define ARDUINO_R4_UNO_WALL_Z_DISPLAYROUND_H


#include "Adafruit_SPIDevice.h"

class displayRound {

public:
	static void menuSetup();

	static void menuLoop();

	static void menu(const int *keyPressed);;


};


#endif //ARDUINO_R4_UNO_WALL_Z_DISPLAYROUND_H
