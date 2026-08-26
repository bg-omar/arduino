//

// Created by mr on 11/18/2023.

//



#include "avoid_objects.h"



#include "Arduino.h"

#include "analog.h"

#include "avoid_roam.h"

#include "config.h"

#include "deadline.h"

#include "displayAdafruit.h"

#include "logger.h"

#include "main_ra.h"

#include "motor.h"

#include "oled_face_clip.h"

#include "PS4.h"

#include "pwm_board.h"

#include "robot_idle.h"

#include "sense_thresholds.h"



/********************************************** control ultrasonic sensor***************************************/

// section UltraSonic

/***************************************************************************************************************/

double  avoid_objects::distanceF,  avoid_objects::distanceR,  avoid_objects::distanceL;

uint32_t avoid_objects::distanceReadMs = 0;

long  avoid_objects::random2 = 0;

bool avoid_objects::active = false;



bool avoid_objects::cachedDistanceFresh(uint32_t now, uint32_t maxAgeMs) {

	return static_cast<uint32_t>(now - distanceReadMs) <= maxAgeMs;

}



enum AvoidStep {

	AvoidForward,

	AvoidNodUp,

	AvoidNodDown,

	AvoidLookLeft,

	AvoidLookRight,

	AvoidTurn

};



static AvoidStep avoidStep = AvoidForward;

static Deadline avoidDeadline;

static bool avoidTurnLeft = true;

static uint32_t lastNoEchoMs = 0;

static RobotIdleState sAvoidIdleState{};
static uint32_t sAvoidLastMs = 0;
static uint32_t sRainbowLastMs = 0;
static bool sHosted = false;
static AvoidRoamState sRoam{};

static void applyHostedRoamMotor(AvoidRoamMotor motor) {
	switch (motor) {
		case AvoidRoamMotor::Forward:
			Motor::Car_avoidForward();
			break;
		case AvoidRoamMotor::TurnLeft:
			Motor::Car_left();
			break;
		case AvoidRoamMotor::TurnRight:
			Motor::Car_right();
			break;
		case AvoidRoamMotor::Stop:
			Motor::Car_Stop();
			break;
	}
}

static void tickHostedRoamDanceHead(uint32_t now) {
	const AvoidRoamHeadAxis axis = avoidRoamDanceHeadAxis(now, sRoam);
	if (axis == AvoidRoamHeadAxis::XY) {
		pwm_board::posXY = random(1, 180);
		pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(pwm_board::posXY));
	} else if (axis == AvoidRoamHeadAxis::Z) {
		pwm_board::posZ = random(1, 160);
		pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(pwm_board::posZ));
	}
}



#define SPEED_OF_SOUND_CM_PER_US 0.0343 // Speed of sound in cm/µs (343 m/s)

#define CM_PER_MICROSECOND 58.00       // Derived constant for HC-SR04



double avoid_objects::checkDistance() {

	digitalWrite(Trig_PIN, LOW);

	delayMicroseconds(2);

	digitalWrite(Trig_PIN, HIGH);

	delayMicroseconds(10);

	digitalWrite(Trig_PIN, LOW);



	unsigned long duration = pulseIn(Echo_PIN, HIGH, 3000); // ~50 cm max; keeps loop responsive



	if (duration == 0) {

		uint32_t now = millis();

		if (elapsed(now, lastNoEchoMs, 1000)) {

			logger::logln("No echo detected");

		}

		return -1.0;

	}



	double distance = duration / CM_PER_MICROSECOND;

	distanceF = distance;

	distanceReadMs = millis();

	return distance;

}



double avoid_objects::checkDistanceLong(uint32_t timeoutUs) {

	digitalWrite(Trig_PIN, LOW);

	delayMicroseconds(2);

	digitalWrite(Trig_PIN, HIGH);

	delayMicroseconds(10);

	digitalWrite(Trig_PIN, LOW);



	const unsigned long duration = pulseIn(Echo_PIN, HIGH, timeoutUs);



	if (duration == 0) {

		return -1.0;

	}



	const double distance = duration / CM_PER_MICROSECOND;

	distanceF = distance;

	distanceReadMs = millis();

	return distance;

}



void avoid_objects::start() {

	active = true;

	sHosted = false;

	sRoam = AvoidRoamState{};

	avoidStep = AvoidForward;

	avoidDeadline.after(millis(), 0);

	sAvoidIdleState = RobotIdleState{};

	if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display) {

		displayAdafruit::setMode(OledMode::Avoid);

		displayAdafruit::setAvoidFace("Avoid");

	}

}



void avoid_objects::startHosted() {

	active = true;

	sHosted = true;

	sRoam = AvoidRoamState{};
	sRoam.wanderNextMs = millis();
	sRoam.danceNextMs = millis() + AVOID_ROAM_DANCE_GAP_MIN_MS;

	avoidStep = AvoidForward;

	avoidDeadline.after(millis(), 0);

	sAvoidIdleState = RobotIdleState{};

}



void avoid_objects::stop() {

	if (!active) {

		return;

	}

	active = false;

	sHosted = false;

	sRoam = AvoidRoamState{};

	Motor::Car_Stop();

	if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display

		&& displayAdafruit::getMode() == OledMode::Avoid) {

		displayAdafruit::setMode(OledMode::Log);

	}

}



bool avoid_objects::isActive() {

	return active;

}



void avoid_objects::tick() {

	if (!active) {

		return;

	}

	if (PS4::exitLoop()) {

		stop();

		return;

	}

	if (PS4::isManualControlActive()) {

		return;

	}

	const uint32_t now = millis();

	if (!elapsed(now, sAvoidLastMs, 40)) {

		return;

	}

	sAvoidLastMs = now;

	if (elapsed(now, sRainbowLastMs, 50)) {

		sRainbowLastMs = now;

		pwm_board::RainbowColor();

	}



	switch (avoidStep) {

		case AvoidForward:

			if (!cachedDistanceFresh(now, 80)) {

				distanceF = checkDistance();

			}

			if (distanceF < 0 || (distanceF >= 0 && distanceF < 25)) {

				Motor::Car_Stop();

				sRoam.wandering = false;

				pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(115));

				avoidStep = AvoidNodUp;

				avoidDeadline.after(now, 10);

				if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display) {

					displayAdafruit::setSenseReactFace(OledFaceClipId::Alert, "Near");

				}

			} else {

				if (sHosted) {

					int lightL = 0;

					int lightR = 0;

					if (FEATURE_ENABLED(main::use_light, USE_LIGHT)) {

						lightL = analog::ext_analog_0;

						lightR = analog::ext_analog_2;

					}

					const AvoidRoamMotor roamMotor = avoidRoamHostedMotor(

						now,

						sRoam,

						lightL,

						lightR,

						static_cast<uint32_t>(random(0, 100)),

						static_cast<uint32_t>(random(0, 100)),

						static_cast<uint32_t>(random(0, AVOID_ROAM_WANDER_GAP_RANGE_MS)),

						static_cast<uint32_t>(random(0, AVOID_ROAM_DANCE_GAP_RANGE_MS)),

						static_cast<uint32_t>(random(0, 2)));

					applyHostedRoamMotor(roamMotor);

					if (avoidRoamIsDancing(now, sRoam)) {

						tickHostedRoamDanceHead(now);

					}

				} else {

					Motor::Car_front();

				}

				if (!PS4::isManualControlActive()

					&& FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)

					&& !(sHosted && avoidRoamIsDancing(now, sRoam))) {

					const RobotIdleOutput idle = robotIdleStep(sAvoidIdleState, now, RobotIdleConfig{},

						false);

					pwm_board::posXY = idle.headXY;

					pwm_board::posZ = idle.headZ;

					pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(idle.headXY));

					pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(idle.headZ));

				}

				if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display) {

					displayAdafruit::setAvoidFace("Avoid");

				}

			}

			break;



		case AvoidNodUp:

			if (!avoidDeadline.isDue(now)) {

				return;

			}

			pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(90));

			avoidStep = AvoidNodDown;

			avoidDeadline.after(now, 10);

			break;



		case AvoidNodDown:

			if (!avoidDeadline.isDue(now)) {

				return;

			}

			pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(160));

			avoidStep = AvoidLookLeft;

			avoidDeadline.after(now, 200);

			break;



		case AvoidLookLeft:

			if (!avoidDeadline.isDue(now)) {

				return;

			}

			distanceL = checkDistance();

			pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(20));

			avoidStep = AvoidLookRight;

			avoidDeadline.after(now, 200);

			break;



		case AvoidLookRight:

			if (!avoidDeadline.isDue(now)) {

				return;

			}

			distanceR = checkDistance();

			pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(90));

			random2 = random(1, 100);

			if (distanceL < 50 || distanceR < 50) {

				avoidTurnLeft = distanceL > distanceR;

			} else {

				avoidTurnLeft = ((long)(random2) % (long)(2)) == 0;

			}

			if (avoidTurnLeft) {

				Motor::Car_left();

			} else {

				Motor::Car_right();

			}

			avoidStep = AvoidTurn;

			avoidDeadline.after(now, 500);

			if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display) {

				displayAdafruit::setSenseReactFace(OledFaceClipId::Alert, "Near");

			}

			break;



		case AvoidTurn:

			if (!avoidDeadline.isDue(now)) {

				return;

			}

			if (sHosted) {

				Motor::Car_avoidForward();

			} else {

				Motor::Car_front();

			}

			avoidStep = AvoidForward;

			break;

	}

}



void avoid_objects::avoid() {

	start();

}

