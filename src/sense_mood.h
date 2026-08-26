//
// Priority mood from sensor snapshot. Arduino-free for native tests.
//

#ifndef SENSE_MOOD_H
#define SENSE_MOOD_H

#include <cstdint>

#include "light_drive.h"
#include "pesto_emotion.h"
#include "sense_thresholds.h"

enum class SenseMotorIntent : uint8_t {
	Stop = 0,
	Forward,
	Back,
	Left,
	Right,
	SoftLeft,
	SoftRight,
	CreepForward,
	CreepLeft,
	CreepRight,
};

enum class SenseTrigger : uint8_t {
	None = 0,
	Mic,
	Light,
	Distance,
	Gyro,
	Baro,
	Compass,
	Seek,
	Avoid,
};

struct SenseSnapshot {
	float distanceCm;   // < 0 = invalid / unused
	bool gyroImpact;
	int micL;
	int micR;
	int lightL;
	int lightR;
	float headingDeg;   // < 0 = unused
	float baroDeltaAbs; // 0 if unused
	bool hasDistance;
	bool hasGyro;
	bool hasMic;
	bool hasLight;
	bool hasCompass;
	bool hasBaro;
};

struct SenseMoodResult {
	PestoEmotion emotion;
	SenseMotorIntent motor;
	SenseTrigger trigger;
	bool headOnly;
	bool isIdle;
};

inline SenseMoodResult sensePickMood(const SenseSnapshot& s, const SenseThresholds& t) {
	SenseMoodResult out{};
	out.emotion = PestoEmotion::Idle;
	out.motor = SenseMotorIntent::Stop;
	out.trigger = SenseTrigger::None;
	out.headOnly = false;
	out.isIdle = true;

	if (s.hasDistance && s.distanceCm >= 0.0f && s.distanceCm < t.distanceCm) {
		out.emotion = PestoEmotion::Alert;
		out.motor = SenseMotorIntent::Back;
		out.trigger = SenseTrigger::Distance;
		out.isIdle = false;
		return out;
	}

	if (s.hasGyro && s.gyroImpact) {
		out.emotion = PestoEmotion::Alert;
		out.motor = SenseMotorIntent::Stop;
		out.trigger = SenseTrigger::Gyro;
		out.isIdle = false;
		return out;
	}

	if (s.hasMic) {
		const int loudL = s.micL;
		const int loudR = s.micR;
		const int maxLoud = loudL > loudR ? loudL : loudR;
		if (maxLoud >= t.micLoud) {
			const int diff = loudL - loudR;
			if (diff > t.micBias) {
				out.emotion = PestoEmotion::LookLeft;
				out.motor = SenseMotorIntent::Left;
				out.trigger = SenseTrigger::Mic;
				out.isIdle = false;
				return out;
			}
			if (-diff > t.micBias) {
				out.emotion = PestoEmotion::LookRight;
				out.motor = SenseMotorIntent::Right;
				out.trigger = SenseTrigger::Mic;
				out.isIdle = false;
				return out;
			}
			out.emotion = PestoEmotion::Curious;
			out.motor = SenseMotorIntent::Stop;
			out.trigger = SenseTrigger::Mic;
			out.headOnly = true;
			out.isIdle = false;
			return out;
		}
	}

	if (s.hasLight) {
		const LightDriveIntent drive = lightDriveIntent(s.lightL, s.lightR, t);
		if (drive != LightDriveIntent::None) {
			out.trigger = SenseTrigger::Light;
			out.isIdle = false;
			const int diff = s.lightL - s.lightR;
			if (diff >= t.lightBias) {
				out.emotion = PestoEmotion::LookLeft;
			} else if (-diff >= t.lightBias) {
				out.emotion = PestoEmotion::LookRight;
			} else {
				out.emotion = PestoEmotion::Happy;
			}
			switch (drive) {
				case LightDriveIntent::CreepForward:
					out.motor = SenseMotorIntent::CreepForward;
					break;
				case LightDriveIntent::CreepLeft:
					out.motor = SenseMotorIntent::CreepLeft;
					break;
				case LightDriveIntent::CreepRight:
					out.motor = SenseMotorIntent::CreepRight;
					break;
				default:
					out.motor = SenseMotorIntent::Stop;
					out.headOnly = true;
					break;
			}
			return out;
		}
	}

	// Compass heading is always present; do not steal idle or park the head
	// in a cardinal sector (that left the ultrasonic ~45° right as "idle").

	if (s.hasBaro && s.baroDeltaAbs >= t.baroDelta) {
		out.emotion = PestoEmotion::Wink;
		out.motor = SenseMotorIntent::Stop;
		out.trigger = SenseTrigger::Baro;
		out.isIdle = false;
		return out;
	}

	return out;
}

inline SenseMoodResult sensePickMood(const SenseSnapshot& s) {
	return sensePickMood(s, senseDefaultThresholds());
}

#endif // SENSE_MOOD_H
