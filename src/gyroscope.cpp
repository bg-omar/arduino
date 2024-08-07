//
// Created by mr on 11/17/2023.
//

#include "logger.h"
#include "timers.h"
#include "gyroscope.h"
#include "main_ra.h"
#include "pwm_board.h"
#include "compass.h"

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
    gyroscope::gyroRead();
	#if LOG_DEBUG
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
	#endif
}

void gyroscope::gyroDetectMovement() {

    gyroscope::gyroRead();
    if(( abs(gyroscope::ax) + abs(gyroscope::ay) + abs(gyroscope::az)) > THRESHOLD){
        gyroscope::gyroFunc();
        //timers::timerTwoActive = true;      timers::timerTreeActive = true;      timers::timerButton = R1;
    }
    if(( abs(gyroscope::gx) + abs(gyroscope::gy) + abs(gyroscope::gz)) > THRESHOLD){
        gyroscope::gyroFunc();
       // timers::timerTwoActive = true;      timers::timerTreeActive = true;      timers::timerButton = L1;
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
	#if USE_U8G2
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
        logger::logln("MPU6050 Found!    ");
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); /// 5, 10, 21, 44, 94, 184, 260(off)
        gyroscope::gyroCalibrate_sensor();
        gyroscope::gyroFunc();
        delay(500);
    }
}