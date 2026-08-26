//
// Created by mr on 11/17/2023.
//

#include "logger.h"
#include "timers.h"
#include "gyroscope.h"
#include "main_ra.h"
#include "pwm_board.h"
#include "compass.h"
#include "barometer.h"
#include "analog.h"
#include "avoid_objects.h"
#include "config.h"
#include "deadline.h"
#include "sensor_format.h"
#include <Arduino.h>
#include <cstdio>
#include <cmath>

Adafruit_MPU6050 mpu; // Set the gyroscope

float   gyroscope::ax,  gyroscope::ay, gyroscope::az, gyroscope::gx, gyroscope::gy, gyroscope::gz, gyroscope::baseAx, gyroscope::baseAy, gyroscope::baseAz, gyroscope::baseGx, gyroscope::baseGy, gyroscope::baseGz, gyroscope::temperature = 0;


void gyroscope::gyroRead(){
    sensors_event_t a, gyro, temp;
    mpu.getEvent(&a, &gyro, &temp);

    gyroscope::temperature = temp.temperature;  gyroscope::ax = a.acceleration.x - gyroscope::baseAx;
    gyroscope::ay = a.acceleration.y - gyroscope::baseAy;  gyroscope::az = a.acceleration.z - gyroscope::baseAz;
    gyroscope::gx = gyro.gyro.x - gyroscope::baseGx;  gyroscope::gy = gyro.gyro.y - gyroscope::baseGy;  gyroscope::gz = gyro.gyro.z - gyroscope::baseGz;
}

void gyroscope::gyroFunc(){
	//compass::displayCompass();
	#if LOG_VERBOSE
		if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
			(gyroscope::ax > 0)
							? logger::log(" +"), logger::logFloat(((gyroscope::ax)))
							: logger::log(" "),  logger::logFloat(((gyroscope::ax)));
			(gyroscope::ay > 0)
							? logger::log(" +"), logger::logFloat(((gyroscope::ay)))
							: logger::log(" "),  logger::logFloat(((gyroscope::ay)));
			(gyroscope::az > 0)
							? logger::log(" +"), logger::logFloat(((gyroscope::az)))
							: logger::log(" "),  logger::logFloat(((gyroscope::az)));
			(gyroscope::gx > 0)
							? logger::log(" +"), logger::logFloat(((gyroscope::gx)))
							: logger::log(" "),  logger::logFloat(((gyroscope::gx)));
			(gyroscope::gy > 0)
							? logger::log(" +"), logger::logFloat(((gyroscope::gy)))
							: logger::log(" "),  logger::logFloat(((gyroscope::gy)));
			(gyroscope::gz > 0)
							? logger::log(" +"), logger::logFloatln(((gyroscope::gz)))
							: logger::log(" "),  logger::logFloatln(((gyroscope::gz)));
		}
	#endif
}

static void logImpactSnapshot() {
	char line[48];
	formatImpactAccelGyro(line, sizeof(line),
		gyroscope::ax, gyroscope::ay, gyroscope::az,
		gyroscope::gx, gyroscope::gy, gyroscope::gz);
	logger::logln(line);

	if (FEATURE_ENABLED(main::use_barometer, USE_BAROMETER) && barometer::bmp != nullptr) {
		if (barometer::bmp->takeForcedMeasurement()) {
			formatImpactBaro(line, sizeof(line),
				barometer::bmp->readTemperature(),
				barometer::bmp->readPressure() / 100.0f);
			logger::logln(line);
		}
	}

	if (FEATURE_ENABLED(main::use_analog, USE_ANALOG) || FEATURE_ENABLED(main::use_audio, USE_AUDIO)
		|| FEATURE_ENABLED(main::use_light, USE_LIGHT)) {
		formatImpactMicLight(line, sizeof(line),
			static_cast<int>(analog::ext_analog_1),
			static_cast<int>(analog::ext_analog_3),
			static_cast<int>(analog::ext_analog_0),
			static_cast<int>(analog::ext_analog_2));
		logger::logln(line);
	}

	if (FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) {
		formatImpactDist(line, sizeof(line), avoid_objects::distanceF);
		logger::logln(line);
	}
}

void gyroscope::gyroDetectMovement() {
	static uint32_t lastImpactMs = 0;
	gyroscope::gyroRead();
	const bool accelHit = (fabsf(gyroscope::ax) + fabsf(gyroscope::ay) + fabsf(gyroscope::az)) > THRESHOLD;
	const bool gyroHit = (fabsf(gyroscope::gx) + fabsf(gyroscope::gy) + fabsf(gyroscope::gz)) > THRESHOLD;
	if (!accelHit && !gyroHit) {
		return;
	}
	gyroscope::gyroFunc();
	const uint32_t now = millis();
	if (elapsed(now, lastImpactMs, 250)) {
		logImpactSnapshot();
	}
}
void gyroscope::gyroCalibrate_sensor() {
    float totX = 0;  float totY = 0;  float totZ = 0;  float totgX = 0;  float totgY = 0;  float totgZ = 0;
    sensors_event_t a, gyro, temp;
    delay(10);
    for (size_t i = 0; i < 10; i++) {
        mpu.getEvent(&a, &gyro, &temp);      delay(10);
        totX += a.acceleration.x;      delay(10);
        totY += a.acceleration.y;      delay(10);
        totZ += a.acceleration.z;      delay(10);
        totgX += gyro.gyro.x;      delay(10);
        totgY += gyro.gyro.y;      delay(10);
        totgZ += gyro.gyro.z;      delay(10);
    }
    gyroscope::baseAx = totX / 10;  gyroscope::baseAy = totY / 10;  gyroscope::baseAz = totZ / 10;  gyroscope::baseGx = totgX / 10;  gyroscope::baseGy = totgY / 10;  gyroscope::baseGz = totgZ / 10;
	#if LOG_VERBOSE
		logger::log(" Ax: ");  logger::logFloat(((baseAx)));
		logger::log(" Ay: ");  logger::logFloat(((baseAy)));
		logger::log(" Az: ");  logger::logFloatln(((baseAz)));

		logger::log(" Gx: ");  logger::logFloat(((baseGx)));
		logger::log(" Gy: ");  logger::logFloat(((baseGy)));
		logger::log(" Gz: ");  logger::logFloatln(((baseGz)));
	#endif
}

void gyroscope::gyroSetup() {
    // Try to initialize!
    if (!mpu.begin()) {
        logger::logln("MPU6050 not found");
        delay(500);
		main::use_gyro = false;
    } else {
        logger::logln("MPU6050 ok");
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); /// 5, 10, 21, 44, 94, 184, 260(off)
        gyroscope::gyroCalibrate_sensor();
        gyroscope::gyroFunc();
        delay(500);
    }
}