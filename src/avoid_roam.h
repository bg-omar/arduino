//
// Hosted avoid free-roam overlays — light follow + dance bursts (Arduino-free).
//
#ifndef AVOID_ROAM_H
#define AVOID_ROAM_H

#include <cstdint>

#include "light_drive.h"
#include "sense_thresholds.h"

constexpr uint32_t AVOID_ROAM_DANCE_MS = 2000;
constexpr uint32_t AVOID_ROAM_DANCE_GAP_MIN_MS = 8000;
constexpr uint32_t AVOID_ROAM_DANCE_GAP_RANGE_MS = 4000;
constexpr uint32_t AVOID_ROAM_DANCE_HEAD_MS = 500;
constexpr uint8_t AVOID_ROAM_DANCE_START_CHANCE = 25;
constexpr uint8_t AVOID_ROAM_WANDER_START_CHANCE = 30;
constexpr uint32_t AVOID_ROAM_WANDER_PULSE_MS = 400;
constexpr uint32_t AVOID_ROAM_WANDER_GAP_MIN_MS = 2000;
constexpr uint32_t AVOID_ROAM_WANDER_GAP_RANGE_MS = 2000;

enum class AvoidRoamMotor : uint8_t { Forward, TurnLeft, TurnRight, Stop };

enum class AvoidRoamHeadAxis : uint8_t { None, XY, Z };

struct AvoidRoamConfig {
	uint32_t danceMs = AVOID_ROAM_DANCE_MS;
	uint32_t danceGapMinMs = AVOID_ROAM_DANCE_GAP_MIN_MS;
	uint32_t danceGapRangeMs = AVOID_ROAM_DANCE_GAP_RANGE_MS;
	uint8_t danceStartChance = AVOID_ROAM_DANCE_START_CHANCE;
	uint8_t wanderStartChance = AVOID_ROAM_WANDER_START_CHANCE;
	uint32_t wanderPulseMs = AVOID_ROAM_WANDER_PULSE_MS;
	uint32_t wanderGapMinMs = AVOID_ROAM_WANDER_GAP_MIN_MS;
	uint32_t wanderGapRangeMs = AVOID_ROAM_WANDER_GAP_RANGE_MS;
};

struct AvoidRoamState {
	bool wandering = false;
	bool wanderLeft = false;
	uint32_t wanderUntilMs = 0;
	uint32_t wanderNextMs = 0;
	uint32_t danceUntilMs = 0;
	uint32_t danceNextMs = 0;
	int dancePose = 0;
	uint32_t danceHeadLastMs = 0;
};

inline AvoidRoamMotor avoidRoamMotorFromLight(LightDriveIntent intent) {
	switch (intent) {
		case LightDriveIntent::CreepForward:
			return AvoidRoamMotor::Forward;
		case LightDriveIntent::CreepLeft:
			return AvoidRoamMotor::TurnLeft;
		case LightDriveIntent::CreepRight:
			return AvoidRoamMotor::TurnRight;
		default:
			return AvoidRoamMotor::Stop;
	}
}

inline bool avoidRoamIsDancing(uint32_t now, const AvoidRoamState& state) {
	return state.danceUntilMs > now;
}

inline AvoidRoamHeadAxis avoidRoamDanceHeadAxis(uint32_t now, AvoidRoamState& state) {
	if (!avoidRoamIsDancing(now, state)) {
		return AvoidRoamHeadAxis::None;
	}
	if ((now - state.danceHeadLastMs) < AVOID_ROAM_DANCE_HEAD_MS) {
		return AvoidRoamHeadAxis::None;
	}
	state.danceHeadLastMs = now;
	const AvoidRoamHeadAxis axis =
		(state.dancePose % 2 == 0) ? AvoidRoamHeadAxis::XY : AvoidRoamHeadAxis::Z;
	state.dancePose = (state.dancePose + 1) % 6;
	return axis;
}

inline AvoidRoamMotor avoidRoamHostedMotor(
		uint32_t now,
		AvoidRoamState& state,
		int lightL,
		int lightR,
		uint32_t rollDance,
		uint32_t rollWander,
		uint32_t rollWanderGap,
		uint32_t rollDanceGap,
		uint32_t rollWanderSide,
		const AvoidRoamConfig& cfg = {},
		const SenseThresholds& thresholds = senseDefaultThresholds()) {
	if (avoidRoamIsDancing(now, state)) {
		return AvoidRoamMotor::Stop;
	}

	const LightDriveIntent light = lightDriveIntent(lightL, lightR, thresholds);
	if (lightDriveActive(light)) {
		state.wandering = false;
		return avoidRoamMotorFromLight(light);
	}

	if (state.wandering) {
		if (now < state.wanderUntilMs) {
			return state.wanderLeft ? AvoidRoamMotor::TurnLeft : AvoidRoamMotor::TurnRight;
		}
		state.wandering = false;
		state.wanderNextMs =
			now + cfg.wanderGapMinMs + (rollWanderGap % cfg.wanderGapRangeMs);
	}

	if (now >= state.danceNextMs && rollDance < cfg.danceStartChance) {
		state.danceUntilMs = now + cfg.danceMs;
		state.dancePose = 0;
		state.danceHeadLastMs = now;
		state.danceNextMs =
			now + cfg.danceGapMinMs + (rollDanceGap % cfg.danceGapRangeMs);
		return AvoidRoamMotor::Stop;
	}

	if (now >= state.wanderNextMs && rollWander < cfg.wanderStartChance) {
		state.wandering = true;
		state.wanderUntilMs = now + cfg.wanderPulseMs;
		state.wanderLeft = (rollWanderSide % 2) == 0;
		return state.wanderLeft ? AvoidRoamMotor::TurnLeft : AvoidRoamMotor::TurnRight;
	}

	return AvoidRoamMotor::Forward;
}

#endif // AVOID_ROAM_H
