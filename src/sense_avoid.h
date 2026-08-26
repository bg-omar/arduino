//
// Mini obstacle avoid overlay during light creep — Arduino-free for native tests.
//
#ifndef SENSE_AVOID_H
#define SENSE_AVOID_H

#include <cstdint>

#include "pesto_emotion.h"
#include "sense_mood.h"

constexpr float SENSE_AVOID_BLOCK_CM = 35.0f;
constexpr float SENSE_AVOID_STOP_CM = 25.0f;
constexpr float SENSE_AVOID_SIDE_CM = 50.0f;
constexpr uint32_t SENSE_AVOID_NOD_MS = 10;
constexpr uint32_t SENSE_AVOID_LOOK_MS = 200;
constexpr uint32_t SENSE_AVOID_TURN_MS = 500;

enum class SenseAvoidPhase : uint8_t {
	Clear,
	Blocked,
	NodUp,
	NodDown,
	LookL,
	LookR,
	Turn,
	Resume,
};

struct SenseAvoidState {
	SenseAvoidPhase phase = SenseAvoidPhase::Clear;
	uint32_t phaseStartMs = 0;
	bool turnLeft = true;
};

struct SenseAvoidOutput {
	SenseAvoidPhase phase;
	bool overrideMotor;
	SenseMotorIntent motor;
	PestoEmotion headEmotion;
	int headXY;
	int headZ;
};

inline bool senseAvoidNeedsClearance(float distF) {
	return distF >= 0.0f && distF < SENSE_AVOID_BLOCK_CM;
}

inline bool senseAvoidPickTurnLeft(float distL, float distR) {
	const bool lValid = distL >= 0.0f;
	const bool rValid = distR >= 0.0f;
	if (lValid && rValid && (distL < SENSE_AVOID_SIDE_CM || distR < SENSE_AVOID_SIDE_CM)) {
		return distL > distR;
	}
	return true;
}

inline SenseAvoidOutput senseAvoidStep(SenseAvoidState& st, float distF, float distL, float distR,
		uint32_t now) {
	SenseAvoidOutput out{};
	out.phase = st.phase;
	out.overrideMotor = false;
	out.motor = SenseMotorIntent::Stop;
	out.headEmotion = PestoEmotion::Idle;
	out.headXY = 90;
	out.headZ = 135;

	if (st.phase == SenseAvoidPhase::Clear) {
		if (!senseAvoidNeedsClearance(distF)) {
			return out;
		}
		st.phase = SenseAvoidPhase::Blocked;
		st.phaseStartMs = now;
	}

	out.phase = st.phase;
	out.overrideMotor = true;
	out.headEmotion = PestoEmotion::Alert;

	switch (st.phase) {
		case SenseAvoidPhase::Blocked:
			out.motor = SenseMotorIntent::Stop;
			st.phase = SenseAvoidPhase::NodUp;
			st.phaseStartMs = now;
			out.headZ = 115;
			break;

		case SenseAvoidPhase::NodUp:
			if ((now - st.phaseStartMs) < SENSE_AVOID_NOD_MS) {
				out.headZ = 115;
				break;
			}
			st.phase = SenseAvoidPhase::NodDown;
			st.phaseStartMs = now;
			out.headZ = 90;
			break;

		case SenseAvoidPhase::NodDown:
			if ((now - st.phaseStartMs) < SENSE_AVOID_NOD_MS) {
				out.headZ = 90;
				break;
			}
			st.phase = SenseAvoidPhase::LookL;
			st.phaseStartMs = now;
			out.headXY = 160;
			break;

		case SenseAvoidPhase::LookL:
			if ((now - st.phaseStartMs) < SENSE_AVOID_LOOK_MS) {
				out.headXY = 160;
				break;
			}
			st.phase = SenseAvoidPhase::LookR;
			st.phaseStartMs = now;
			out.headXY = 20;
			break;

		case SenseAvoidPhase::LookR:
			if ((now - st.phaseStartMs) < SENSE_AVOID_LOOK_MS) {
				out.headXY = 20;
				break;
			}
			st.turnLeft = senseAvoidPickTurnLeft(distL, distR);
			st.phase = SenseAvoidPhase::Turn;
			st.phaseStartMs = now;
			out.motor = st.turnLeft ? SenseMotorIntent::Left : SenseMotorIntent::Right;
			out.headXY = 90;
			out.headZ = 135;
			break;

		case SenseAvoidPhase::Turn:
			if ((now - st.phaseStartMs) < SENSE_AVOID_TURN_MS) {
				out.motor = st.turnLeft ? SenseMotorIntent::Left : SenseMotorIntent::Right;
				break;
			}
			st.phase = SenseAvoidPhase::Resume;
			st.phaseStartMs = now;
			out.motor = SenseMotorIntent::CreepForward;
			break;

		case SenseAvoidPhase::Resume:
			if ((now - st.phaseStartMs) < 200) {
				out.motor = SenseMotorIntent::CreepForward;
				break;
			}
			st.phase = SenseAvoidPhase::Clear;
			st.phaseStartMs = 0;
			out.overrideMotor = false;
			out.headEmotion = PestoEmotion::Idle;
			break;

		default:
			break;
	}

	out.phase = st.phase;
	return out;
}

#endif // SENSE_AVOID_H
