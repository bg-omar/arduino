//
// Created by mr on 4/16/2024.
//

#include "analog.h"
#include "config.h"
#include "main_ra.h"
#include "logger.h"

#include "ADS1X15.h"
#include "Wire.h"

int16_t analog::ext_analog_0 = 0;
int16_t analog::ext_analog_1 = 0;
int16_t analog::ext_analog_2 = 0;
int16_t analog::ext_analog_3 = 0;

// Declare ADS as a pointer
ADS1015* ADS = nullptr;

void analog::analogSetup()
{
	// Initialize ADS1015 object
	ADS = new ADS1015(0x48);

	if (!ADS->begin()) {
		logger::logln("ADS fail");
		main::use_analog = false;
	} else {
		logger::logln("ADS ok");
	}
}

void analog::analogLoop()
{
	if (!ADS) return; // Ensure ADS is initialized

	ADS->setGain(0);
	analog::ext_analog_0 = ADS->readADC(light_L_PIN);
	analog::ext_analog_1 = ADS->readADC(MIC_L_PIN);
	analog::ext_analog_2 = ADS->readADC(light_R_PIN);
	analog::ext_analog_3 = ADS->readADC(MIC_R_PIN);

#if LOG_VERBOSE
	if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
		logger::log("light_L: ");
		logger::logInt(ext_analog_0);
		logger::log(" ");
		logger::log("light_R: ");
		logger::logInt(ext_analog_2);
		logger::log(" \t");
		logger::log(" MIC_L_PIN: ");
		logger::logInt(ext_analog_1);
		logger::log(" MIC_R_PIN: ");
		logger::logIntln(ext_analog_3);
	}
#endif
}
