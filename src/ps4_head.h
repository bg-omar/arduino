//
// Right-stick head aim: absolute targets + slew. Arduino-free for native tests.
// Encoded ranges: RX 8000-8255 (center 8127), RY 9000-9255 (center 9127).
//

#ifndef PS4_HEAD_H
#define PS4_HEAD_H

#include <cstdint>

constexpr int PS4_RX_CENTER = 8127;
constexpr int PS4_RY_CENTER = 9127;
constexpr int PS4_HEAD_DEADZONE = 20;
constexpr int PS4_HEAD_XY_MIN = 10;
constexpr int PS4_HEAD_XY_MAX = 170;
constexpr int PS4_HEAD_Z_MIN = 5;
constexpr int PS4_HEAD_Z_MAX = 150;
// Max pan/tilt speed (deg/s). Caps packet-rate jumps that made the head bounce.
constexpr int PS4_HEAD_DEG_PER_SEC = 120;
constexpr uint32_t PS4_HEAD_DT_CAP_MS = 50;

inline int ps4ClampInt(int v, int lo, int hi) {
	if (v < lo) {
		return lo;
	}
	if (v > hi) {
		return hi;
	}
	return v;
}

inline bool ps4IsRightX(int v) {
	return v >= 8000 && v <= 8255;
}

inline bool ps4IsRightY(int v) {
	return v >= 9000 && v <= 9255;
}

inline bool ps4HasRightStickAxis(int v) {
	return ps4IsRightX(v) || ps4IsRightY(v);
}

inline bool ps4UpdateRightStick(int a, int b, int& rx, int& ry) {
	bool updated = false;
	if (ps4IsRightX(a)) {
		rx = a - PS4_RX_CENTER;
		updated = true;
	} else if (ps4IsRightX(b)) {
		rx = b - PS4_RX_CENTER;
		updated = true;
	}

	if (ps4IsRightY(b)) {
		ry = b - PS4_RY_CENTER;
		updated = true;
	} else if (ps4IsRightY(a)) {
		ry = a - PS4_RY_CENTER;
		updated = true;
	}
	return updated;
}

inline bool ps4HeadStickActive(int rx, int ry, int deadzone = PS4_HEAD_DEADZONE) {
	return !(rx > -deadzone && rx < deadzone && ry > -deadzone && ry < deadzone);
}

// Stick left (-) → higher pan; stick up (-) → lower tilt (inverted Y for natural look).
inline int ps4HeadTargetXY(int rx) {
	const int x = ps4ClampInt(rx, -128, 127);
	const int mapped = PS4_HEAD_XY_MAX - ((x + 128) * (PS4_HEAD_XY_MAX - PS4_HEAD_XY_MIN)) / 255;
	return ps4ClampInt(mapped, PS4_HEAD_XY_MIN, PS4_HEAD_XY_MAX);
}

inline int ps4HeadTargetZ(int ry) {
	const int y = ps4ClampInt(-ry, -128, 127); // invert R-stick Y
	const int mapped = PS4_HEAD_Z_MAX - ((y + 128) * (PS4_HEAD_Z_MAX - PS4_HEAD_Z_MIN)) / 255;
	return ps4ClampInt(mapped, PS4_HEAD_Z_MIN, PS4_HEAD_Z_MAX);
}

inline int ps4HeadMaxDelta(uint32_t dtMs, int degPerSec = PS4_HEAD_DEG_PER_SEC) {
	if (dtMs == 0) {
		return 0;
	}
	if (dtMs > PS4_HEAD_DT_CAP_MS) {
		dtMs = PS4_HEAD_DT_CAP_MS;
	}
	const int delta = static_cast<int>((degPerSec * static_cast<int>(dtMs)) / 1000);
	return delta < 1 ? 1 : delta;
}

inline int ps4HeadSlew(int current, int target, int maxDelta) {
	const int d = target - current;
	if (d > maxDelta) {
		return current + maxDelta;
	}
	if (d < -maxDelta) {
		return current - maxDelta;
	}
	return target;
}

struct Ps4HeadTargets {
	int xy;
	int z;
	bool active;
};

inline Ps4HeadTargets ps4HeadTargetsFromStick(int rx, int ry, int deadzone = PS4_HEAD_DEADZONE) {
	Ps4HeadTargets out;
	out.active = ps4HeadStickActive(rx, ry, deadzone);
	if (!out.active) {
		out.xy = 0;
		out.z = 0;
		return out;
	}
	out.xy = ps4HeadTargetXY(rx);
	out.z = ps4HeadTargetZ(ry);
	return out;
}

#endif // PS4_HEAD_H
