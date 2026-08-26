#include "barometer.h"
#include <Wire.h>
#include "main_ra.h"
#include "logger.h"

// Define constants
constexpr float SEALEVELPRESSURE_HPA = 1013.25;  // Standard sea-level pressure

// Dynamic pointer for Adafruit_BMP280 to avoid static initialization issues
Adafruit_BMP280* barometer::bmp = nullptr;

void barometer::baroSetup() {
	// Initialize the barometer object
	barometer::bmp = new Adafruit_BMP280();

	if (!barometer::bmp->begin()) {
		if (main::log_debug) {
		logger::logln("BMP280 fail");
			logger::log("SensorID was: 0x");
			logger::logHexln(barometer::bmp->sensorID(), 16);
		}
		main::use_barometer = false;
		delete barometer::bmp;  // Clean up memory if initialization fails
		barometer::bmp = nullptr;
		return;
	}

	logger::logln("BMP280 ok");

	// Default settings from datasheet
	barometer::bmp->setSampling(
			Adafruit_BMP280::MODE_FORCED,     /* Operating Mode */
			Adafruit_BMP280::SAMPLING_X2,     /* Temperature oversampling */
			Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
			Adafruit_BMP280::FILTER_X16,      /* Filtering */
			Adafruit_BMP280::STANDBY_MS_500); /* Standby time */
}

void barometer::baroMeter() {
	if (!barometer::bmp) {
		logger::logln("Barometer not initialized!");
		return;
	}

	// Trigger a forced measurement
	if (barometer::bmp->takeForcedMeasurement()) {
		// Retrieve and log temperature
		float temp = barometer::bmp->readTemperature();
		logger::log("Temperature: ");
		logger::logFloat(temp);
		logger::logln(" °C");

		// Retrieve and log pressure
		float press = barometer::bmp->readPressure() / 100.0f;  // Convert to hPa
		logger::log("Pressure: ");
		logger::logFloat(press);
		logger::logln(" hPa");

		// Retrieve and log altitude
		float altitude = barometer::bmp->readAltitude(SEALEVELPRESSURE_HPA);
		logger::log("Altitude: ");
		logger::logFloat(altitude);
		logger::logln(" m");
	} else {
		logger::logln("Forced measurement failed!");
	}
}
