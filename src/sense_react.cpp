#include "sense_react.h"



#include <Arduino.h>

#include <cmath>



#include "analog.h"

#include "avoid_objects.h"

#include "barometer.h"

#include "compass.h"

#include "config.h"

#include "dancing.h"

#include "deadline.h"

#include "displayAdafruit.h"

#include "follow_light.h"

#include "gyroscope.h"

#include "main_ra.h"

#include "motor.h"

#include "oled_face_clip.h"

#include "pesto_matrix.h"

#include "PS4.h"

#include "pwm_board.h"

#include "robot_idle.h"

#include "sense_avoid.h"

#include "sense_mood.h"

#include "sense_seek.h"

#include "sense_trigger.h"

#include "sense_playful.h"

#include "radar_scan.h"



bool sense_react::active = false;



static uint32_t sLastTickMs = 0;

static float sLastBaro = 0.0f;

static bool sHaveBaro = false;

static int sMicBaseL = 0;

static int sMicBaseR = 0;

static uint32_t sIdleSinceMs = 0;

static RobotIdleState sIdleState{};

static SenseSeekState sSeekState{};

static SenseAvoidState sAvoidState{};

static SensePlayfulMode sPlayfulMode = SensePlayfulMode::None;

static uint32_t sBurstStartMs = 0;

static uint32_t sNextBurstEarliestMs = 0;

static uint32_t sRadarCooldownUntilMs = 0;

static bool sRadarWasActive = false;

static bool sRadarTriggeredThisIdle = false;



static void stopPlayfulBurst(uint32_t now) {

	if (sPlayfulMode == SensePlayfulMode::None) {

		return;

	}

	dancing::stop();

	avoid_objects::stop();

	Follow_light::stop();

	Motor::Car_Stop();

	sPlayfulMode = SensePlayfulMode::None;

	sBurstStartMs = 0;

	sNextBurstEarliestMs = now + SENSE_PLAYFUL_COOLDOWN_MS;

	if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display

		&& sense_react::isActive()) {

		displayAdafruit::setMode(OledMode::SenseReact);

	}

}



static void startPlayfulBurst(SensePlayfulMode mode, uint32_t now) {

	sPlayfulMode = mode;

	sBurstStartMs = now;

	switch (mode) {

		case SensePlayfulMode::Dance:

			dancing::start();

			break;

		case SensePlayfulMode::Avoid:

			avoid_objects::startHosted();

			break;

		case SensePlayfulMode::FollowLight:

			Follow_light::start();

			break;

		default:

			break;

	}

}



static bool isCreepIntent(SenseMotorIntent intent) {

	return intent == SenseMotorIntent::CreepForward || intent == SenseMotorIntent::CreepLeft

		|| intent == SenseMotorIntent::CreepRight;

}



static void applyMotor(SenseMotorIntent intent) {

	switch (intent) {

		case SenseMotorIntent::Forward:

			Motor::Car_front();

			break;

		case SenseMotorIntent::Back:

			Motor::Car_Back();

			break;

		case SenseMotorIntent::Left:

		case SenseMotorIntent::SoftLeft:

			Motor::Car_left();

			break;

		case SenseMotorIntent::Right:

		case SenseMotorIntent::SoftRight:

			Motor::Car_right();

			break;

		case SenseMotorIntent::CreepForward:

			Motor::Car_creepForward();

			break;

		case SenseMotorIntent::CreepLeft:

			Motor::Car_creepLeft();

			break;

		case SenseMotorIntent::CreepRight:

			Motor::Car_creepRight();

			break;

		case SenseMotorIntent::Stop:

		default:

			Motor::Car_Stop();

			break;

	}

}



static void setHeadPosition(int xy, int z) {

	if (!FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {

		return;

	}

	pwm_board::posXY = xy;

	pwm_board::posZ = z;

	pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(xy));

	pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(z));

}



static void applyHead(PestoEmotion e) {

	if (!FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {

		return;

	}

	int xy = pwm_board::posXY;

	int z = pwm_board::posZ;

	switch (e) {

		case PestoEmotion::LookLeft:

			xy = 140;

			break;

		case PestoEmotion::LookRight:

			xy = 40;

			break;

		case PestoEmotion::LookUp:

			z = 40;

			break;

		case PestoEmotion::LookDown:

			z = 120;

			break;

		case PestoEmotion::Alert:

			z = 60;

			break;

		default:

			return;

	}

	setHeadPosition(xy, z);

}



static void applyTrackPulse(RobotTrackPulse pulse, int pwm) {

	switch (pulse) {

		case RobotTrackPulse::Left:

			digitalWrite(L_ROT, LOW);

			analogWrite(L_PWM, pwm);

			digitalWrite(R_ROT, HIGH);

			analogWrite(R_PWM, 0);

			break;

		case RobotTrackPulse::Right:

			digitalWrite(L_ROT, HIGH);

			analogWrite(L_PWM, 0);

			digitalWrite(R_ROT, LOW);

			analogWrite(R_PWM, pwm);

			break;

		case RobotTrackPulse::None:

		default:

			break;

	}

}



static uint32_t idleStreakMs(uint32_t now) {

	if (sIdleSinceMs == 0) {

		return 0;

	}

	return now - sIdleSinceMs;

}



void sense_react::start() {

	active = true;

	sLastTickMs = 0;

	sHaveBaro = false;

	sIdleSinceMs = 0;

	sIdleState = RobotIdleState{};

	sSeekState = SenseSeekState{};

	sAvoidState = SenseAvoidState{};

	sPlayfulMode = SensePlayfulMode::None;

	sBurstStartMs = 0;

	sNextBurstEarliestMs = 0;

	sRadarCooldownUntilMs = 0;

	sRadarWasActive = false;

	sRadarTriggeredThisIdle = false;

	sMicBaseL = analog::ext_analog_1;

	sMicBaseR = analog::ext_analog_3;

	setHeadPosition(ROBOT_IDLE_CENTER_XY, ROBOT_IDLE_CENTER_Z);

	if (FEATURE_ENABLED(main::use_dot, USE_DOT)) {

		Pesto::showEmotion(PestoEmotion::Curious, millis());

	}

	if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display) {

		displayAdafruit::setMode(OledMode::SenseReact);

		displayAdafruit::setSenseReactFace(OledFaceClipId::IdleBlink, "Idle");

	}

}



void sense_react::stop() {

	if (!active) {

		return;

	}

	active = false;

	stopPlayfulBurst(millis());

	Motor::Car_Stop();

	if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display

		&& displayAdafruit::getMode() == OledMode::SenseReact) {

		displayAdafruit::setMode(OledMode::Log);

	}

}



bool sense_react::isActive() {

	return active;

}



void sense_react::tick() {

	if (!active) {

		return;

	}

	if (PS4::exitLoop()) {

		stop();

		return;

	}



	const uint32_t now = millis();

	if (!elapsed(now, sLastTickMs, 90)) {

		return;

	}

	sLastTickMs = now;



	if (PS4::isManualControlActive()) {

		stopPlayfulBurst(now);

		sIdleSinceMs = 0;

		sAvoidState = SenseAvoidState{};

		return;

	}



	if (radar_scan::isActive()) {

		sRadarWasActive = true;

		return;

	}

	if (sRadarWasActive) {

		sRadarWasActive = false;

		sRadarCooldownUntilMs = now + RADAR_SENSE_COOLDOWN_MS;

	}



	if (sPlayfulMode != SensePlayfulMode::None) {

		if (sensePlayfulBurstExpired(now, sBurstStartMs)) {

			stopPlayfulBurst(now);

		} else {

			return;

		}

	}



	SenseSnapshot snap{};

	snap.distanceCm = -1.0f;

	snap.headingDeg = -1.0f;

	snap.baroDeltaAbs = 0.0f;



	if (FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) {

		snap.hasDistance = true;

		if (avoid_objects::cachedDistanceFresh(now, 80)) {

			snap.distanceCm = static_cast<float>(avoid_objects::distanceF);

		} else {

			snap.distanceCm = static_cast<float>(avoid_objects::checkDistance());

		}

	}



	if (FEATURE_ENABLED(main::use_gyro, USE_GYRO) || main::Found_Gyro) {

		snap.hasGyro = true;

		gyroscope::gyroRead();

		const float aSum = fabsf(gyroscope::ax) + fabsf(gyroscope::ay) + fabsf(gyroscope::az);

		const float gSum = fabsf(gyroscope::gx) + fabsf(gyroscope::gy) + fabsf(gyroscope::gz);

		snap.gyroImpact = (aSum > THRESHOLD) || (gSum > THRESHOLD);

	}



	if (FEATURE_ENABLED(main::use_audio, USE_AUDIO) || FEATURE_ENABLED(main::use_analog, USE_ANALOG)) {

		snap.hasMic = true;

		const int rawL = analog::ext_analog_1;

		const int rawR = analog::ext_analog_3;

		snap.micL = pestoAbs(rawL - sMicBaseL);

		snap.micR = pestoAbs(rawR - sMicBaseR);

	}



	if (FEATURE_ENABLED(main::use_light, USE_LIGHT)) {

		snap.hasLight = true;

		snap.lightL = analog::ext_analog_0;

		snap.lightR = analog::ext_analog_2;

	}



	if ((FEATURE_ENABLED(main::use_compass, USE_COMPASS) || main::Found_Compass)

		&& compass::mag != nullptr) {

		snap.hasCompass = true;

		snap.headingDeg = static_cast<float>(compass::readCompass());

	}



	if (FEATURE_ENABLED(main::use_barometer, USE_BAROMETER) && barometer::bmp != nullptr) {

		snap.hasBaro = true;

		if (barometer::bmp->takeForcedMeasurement()) {

			const float p = barometer::bmp->readPressure() / 100.0f;

			if (sHaveBaro) {

				snap.baroDeltaAbs = fabsf(p - sLastBaro);

			}

			sLastBaro = p;

			sHaveBaro = true;

		}

	}



	const uint32_t streak = idleStreakMs(now);

	const SenseThresholds thresholds = senseEffectiveThresholds(streak);

	const SenseMoodResult mood = sensePickMood(snap, thresholds);



	if (mood.isIdle) {

		if (sIdleSinceMs == 0) {

			sIdleSinceMs = now;

		}

	} else {

		sIdleSinceMs = 0;

		sSeekState = SenseSeekState{};

		sRadarTriggeredThisIdle = false;

		if (!isCreepIntent(mood.motor)) {

			sAvoidState = SenseAvoidState{};

		}

	}



	const uint32_t streakNow = idleStreakMs(now);

	const bool seeking = mood.isIdle && senseSeekActive(streakNow);

	SenseTrigger oledTrigger = mood.trigger;

	const char* oledLabel = senseTriggerLabel(mood.trigger, mood.emotion);



	if (seeking) {

		oledTrigger = SenseTrigger::Seek;

		oledLabel = "Seek";

	}



	PestoEmotion faceEmotion = mood.emotion;

	SenseMotorIntent motorIntent = mood.motor;

	bool headFromIdle = false;



	if (mood.isIdle) {

		if (seeking) {

			const SenseSeekOutput seek = senseSeekStep(sSeekState, now, streakNow, snap.lightL,

				snap.lightR);

			setHeadPosition(seek.headXY, seek.headZ);

			headFromIdle = true;

			motorIntent = seek.motorHint;

			if (FEATURE_ENABLED(main::use_dot, USE_DOT)) {

				Pesto::showEmotion(PestoEmotion::Curious, now);

			}

		} else {

			const RobotIdleOutput idle = robotIdleStep(sIdleState, now, RobotIdleConfig{}, true);

			setHeadPosition(idle.headXY, idle.headZ);

			headFromIdle = true;

			if (idle.trackPulse != RobotTrackPulse::None) {

				applyTrackPulse(idle.trackPulse, idle.trackPwm);

				motorIntent = SenseMotorIntent::Stop;

			} else {

				motorIntent = SenseMotorIntent::Stop;

			}

		}

	}

	if (isCreepIntent(motorIntent) && FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) {
		float distL = -1.0f;
		float distR = -1.0f;
		if (sAvoidState.phase == SenseAvoidPhase::LookL
			&& (now - sAvoidState.phaseStartMs) >= (SENSE_AVOID_LOOK_MS - 5)) {
			distL = static_cast<float>(avoid_objects::checkDistance());
		} else if (sAvoidState.phase == SenseAvoidPhase::LookR
			&& (now - sAvoidState.phaseStartMs) >= (SENSE_AVOID_LOOK_MS - 5)) {
			distR = static_cast<float>(avoid_objects::checkDistance());
		}
		const SenseAvoidOutput avoid = senseAvoidStep(sAvoidState, snap.distanceCm, distL, distR, now);

		if (avoid.overrideMotor) {

			motorIntent = avoid.motor;

			if (avoid.headEmotion != PestoEmotion::Idle || avoid.headXY != 90) {

				setHeadPosition(avoid.headXY, avoid.headZ);

				headFromIdle = true;

			}

			if (avoid.phase != SenseAvoidPhase::Clear) {

				faceEmotion = PestoEmotion::Alert;

				oledTrigger = SenseTrigger::Distance;

				oledLabel = "Near";

			}

		}

	}



	if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {

		pwm_board::applyEmotionLeds(faceEmotion);

	}

	if (FEATURE_ENABLED(main::use_dot, USE_DOT) && faceEmotion != PestoEmotion::Idle) {

		Pesto::showEmotion(faceEmotion, now);

	}



	if (!headFromIdle && (mood.headOnly || faceEmotion == PestoEmotion::Alert

			|| faceEmotion == PestoEmotion::LookLeft || faceEmotion == PestoEmotion::LookRight

			|| faceEmotion == PestoEmotion::LookUp || faceEmotion == PestoEmotion::LookDown)) {

		applyHead(faceEmotion);

	}



	if (mood.headOnly && !mood.isIdle) {

		Motor::Car_Stop();

	} else {

		applyMotor(motorIntent);

	}



	if (FEATURE_ENABLED(main::use_adafruit, USE_ADAFRUIT) && main::Found_Display

		&& displayAdafruit::getMode() == OledMode::SenseReact) {

		const OledFaceClipId clip = oledFaceForSense(faceEmotion, oledTrigger, seeking);

		displayAdafruit::setSenseReactFace(clip, oledLabel, faceEmotion);

	}



	if (mood.isIdle && !seeking

		&& sensePlayfulShouldStart(streakNow, seeking, false, sPlayfulMode, now,

			sNextBurstEarliestMs)) {

		startPlayfulBurst(sensePlayfulPickIdle(), now);

	}



	if (mood.isIdle && !seeking && !sRadarTriggeredThisIdle

		&& main::use_sd_card

		&& now >= sRadarCooldownUntilMs

		&& streakNow >= RADAR_SENSE_IDLE_AFTER_MS) {

		radar_scan::startSenseIdle();

		sRadarTriggeredThisIdle = true;

	}

}

