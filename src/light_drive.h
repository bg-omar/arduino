//
// Slow light-follow intent — Arduino-free for native tests.
//
#ifndef LIGHT_DRIVE_H
#define LIGHT_DRIVE_H

#include "sense_thresholds.h"

constexpr uint8_t MOTOR_CREEP_PWM = 110;
constexpr uint8_t MOTOR_AVOID_PWM = 230;

enum class LightDriveIntent : uint8_t { None, CreepForward, CreepLeft, CreepRight };

inline int lightDriveAbs(int v) {
	return v < 0 ? -v : v;
}

inline LightDriveIntent lightDriveIntent(int lightL, int lightR, const SenseThresholds& t) {
	const int diff = lightL - lightR;
	if (lightDriveAbs(diff) >= t.lightBias) {
		return diff > 0 ? LightDriveIntent::CreepLeft : LightDriveIntent::CreepRight;
	}
	if (lightL >= t.lightThresh && lightR >= t.lightThresh) {
		return LightDriveIntent::CreepForward;
	}
	if (lightR >= t.lightThresh) {
		return LightDriveIntent::CreepRight;
	}
	if (lightL >= t.lightThresh) {
		return LightDriveIntent::CreepLeft;
	}
	return LightDriveIntent::None;
}

inline bool lightDriveActive(LightDriveIntent intent) {
	return intent != LightDriveIntent::None;
}

#endif // LIGHT_DRIVE_H
