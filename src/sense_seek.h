//
// Idle → seek phase — Arduino-free for native tests.
//
#ifndef SENSE_SEEK_H
#define SENSE_SEEK_H

#include <cstdint>

#include "sense_mood.h"
#include "sense_thresholds.h"

constexpr uint32_t SENSE_SEEK_STEP_MS = 1500;
constexpr int SENSE_SEEK_HEAD_LEFT = 140;
constexpr int SENSE_SEEK_HEAD_RIGHT = 40;
constexpr int SENSE_SEEK_HEAD_CENTER = 90;
constexpr int SENSE_SEEK_HEAD_Z = 135;

enum class SenseSeekStep : uint8_t {
	Center,
	PanLeft,
	HoldLeft,
	PanRight,
	HoldRight,
	SoftForward,
};

struct SenseSeekState {
	SenseSeekStep step = SenseSeekStep::Center;
	uint32_t stepStartMs = 0;
	bool started = false;
};

struct SenseSeekOutput {
	bool active;
	int headXY;
	int headZ;
	SenseMotorIntent motorHint;
};

inline bool senseSeekActive(uint32_t idleStreakMs) {
	return idleStreakMs >= SENSE_SEEK_AFTER_MS;
}

inline SenseSeekStep senseSeekAdvanceStep(SenseSeekStep step) {
	switch (step) {
		case SenseSeekStep::Center:
			return SenseSeekStep::PanLeft;
		case SenseSeekStep::PanLeft:
			return SenseSeekStep::HoldLeft;
		case SenseSeekStep::HoldLeft:
			return SenseSeekStep::PanRight;
		case SenseSeekStep::PanRight:
			return SenseSeekStep::HoldRight;
		case SenseSeekStep::HoldRight:
			return SenseSeekStep::SoftForward;
		case SenseSeekStep::SoftForward:
		default:
			return SenseSeekStep::Center;
	}
}

inline int senseSeekTiltZ(int lightL, int lightR) {
	const int diff = lightL - lightR;
	if (diff > 20) {
		return SENSE_SEEK_HEAD_Z - 8;
	}
	if (diff < -20) {
		return SENSE_SEEK_HEAD_Z + 8;
	}
	return SENSE_SEEK_HEAD_Z;
}

inline SenseSeekOutput senseSeekStep(SenseSeekState& st, uint32_t now, uint32_t idleStreakMs,
		int lightL = 0, int lightR = 0) {
	SenseSeekOutput out{};
	out.active = senseSeekActive(idleStreakMs);
	out.headXY = SENSE_SEEK_HEAD_CENTER;
	out.headZ = senseSeekTiltZ(lightL, lightR);
	out.motorHint = SenseMotorIntent::Stop;

	if (!out.active) {
		st.step = SenseSeekStep::Center;
		st.stepStartMs = 0;
		st.started = false;
		return out;
	}

	if (!st.started) {
		st.started = true;
		st.stepStartMs = now;
	} else if ((now - st.stepStartMs) >= SENSE_SEEK_STEP_MS) {
		st.step = senseSeekAdvanceStep(st.step);
		st.stepStartMs = now;
	}

	switch (st.step) {
		case SenseSeekStep::PanLeft:
		case SenseSeekStep::HoldLeft:
			out.headXY = SENSE_SEEK_HEAD_LEFT;
			break;
		case SenseSeekStep::PanRight:
		case SenseSeekStep::HoldRight:
			out.headXY = SENSE_SEEK_HEAD_RIGHT;
			break;
		case SenseSeekStep::SoftForward:
			out.headXY = SENSE_SEEK_HEAD_CENTER;
			out.motorHint = SenseMotorIntent::CreepForward;
			break;
		case SenseSeekStep::Center:
		default:
			out.headXY = SENSE_SEEK_HEAD_CENTER;
			break;
	}

	return out;
}

#endif // SENSE_SEEK_H
