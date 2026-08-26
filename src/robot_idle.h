//
// Passive idle motion — Arduino-free for native tests.
//
#ifndef ROBOT_IDLE_H
#define ROBOT_IDLE_H

#include <cmath>
#include <cstdint>

constexpr int ROBOT_IDLE_CENTER_XY = 90;
constexpr int ROBOT_IDLE_CENTER_Z = 135;
constexpr int ROBOT_IDLE_AMP_XY = 4;
constexpr int ROBOT_IDLE_AMP_Z = 3;
constexpr uint32_t ROBOT_IDLE_HEAD_PERIOD_MS = 4000;
constexpr uint32_t ROBOT_IDLE_TRACK_PULSE_MS = 60;
constexpr uint32_t ROBOT_IDLE_TRACK_INTERVAL_MS = 2500;
constexpr int ROBOT_IDLE_TRACK_PWM = 40;

enum class RobotTrackPulse : uint8_t { None, Left, Right };

struct RobotIdleConfig {
	int headCenterXY = ROBOT_IDLE_CENTER_XY;
	int headCenterZ = ROBOT_IDLE_CENTER_Z;
	int headAmpXY = ROBOT_IDLE_AMP_XY;
	int headAmpZ = ROBOT_IDLE_AMP_Z;
	uint32_t headPeriodMs = ROBOT_IDLE_HEAD_PERIOD_MS;
	uint32_t trackPulseMs = ROBOT_IDLE_TRACK_PULSE_MS;
	uint32_t trackIntervalMs = ROBOT_IDLE_TRACK_INTERVAL_MS;
	int trackPwm = ROBOT_IDLE_TRACK_PWM;
};

struct RobotIdleState {
	uint32_t startMs = 0;
	uint32_t lastPulseStartMs = 0;
	bool trackLeft = true;
};

struct RobotIdleOutput {
	int headXY;
	int headZ;
	RobotTrackPulse trackPulse;
	int trackPwm;
};

inline float robotIdlePhaseRad(uint32_t nowMs, uint32_t periodMs) {
	if (periodMs == 0) {
		return 0.0f;
	}
	const float t = (float)(nowMs % periodMs) / (float)periodMs;
	return t * 2.0f * 3.14159265f;
}

inline RobotIdleOutput robotIdleStep(RobotIdleState& st, uint32_t nowMs,
		const RobotIdleConfig& cfg = RobotIdleConfig{}, bool allowTrackPulse = true) {
	if (st.startMs == 0) {
		st.startMs = nowMs;
		st.lastPulseStartMs = nowMs;
	}

	const float phase = robotIdlePhaseRad(nowMs - st.startMs, cfg.headPeriodMs);
	const float s = sinf(phase);
	const float c = cosf(phase * 0.7f);

	RobotIdleOutput out{};
	out.headXY = cfg.headCenterXY + (int)(s * (float)cfg.headAmpXY);
	out.headZ = cfg.headCenterZ + (int)(c * (float)cfg.headAmpZ);
	out.trackPulse = RobotTrackPulse::None;
	out.trackPwm = 0;

	if (!allowTrackPulse) {
		return out;
	}

	if ((nowMs - st.lastPulseStartMs) >= cfg.trackIntervalMs) {
		st.lastPulseStartMs = nowMs;
		st.trackLeft = !st.trackLeft;
	}

	const uint32_t sincePulse = nowMs - st.lastPulseStartMs;
	if (sincePulse < cfg.trackPulseMs) {
		out.trackPulse = st.trackLeft ? RobotTrackPulse::Left : RobotTrackPulse::Right;
		out.trackPwm = cfg.trackPwm;
	}

	return out;
}

#endif // ROBOT_IDLE_H
