//
// Left-stick / L2R2 tank mix with deadzone. Arduino-free for native tests.
// Encoded ranges: LX 6000-6255, LY 7000-7255, L2 4000-4255, R2 5000-5255.
//

#ifndef PS4_DRIVE_H
#define PS4_DRIVE_H

#include <cstdint>

constexpr int PS4_STICK_DEADZONE = 20;
constexpr int PS4_LX_CENTER = 6128;
constexpr int PS4_LY_CENTER = 7128;
// Allow slow main loop (sonar/mic/gyro) without stopping between Serial1 batches.
constexpr uint32_t PS4_DRIVE_TIMEOUT_MS = 500;

inline int ps4ApplyDeadzone(int value, int deadzone = PS4_STICK_DEADZONE) {
	if (value > -deadzone && value < deadzone) {
		return 0;
	}
	return value;
}

inline bool ps4IsLeftX(int v) {
	return v >= 5999 && v <= 6255;
}

inline bool ps4IsLeftY(int v) {
	return v >= 6999 && v <= 7255;
}

inline bool ps4HasLeftStickAxis(int v) {
	return ps4IsLeftX(v) || ps4IsLeftY(v);
}

inline bool ps4IsL2(int v) {
	return v >= 4000 && v <= 4255;
}

inline bool ps4IsR2(int v) {
	return v >= 5000 && v <= 5255;
}

inline bool ps4UpdateLeftStick(int a, int b, int& lx, int& ly) {
	bool updated = false;
	if (ps4IsLeftX(a)) {
		lx = a - PS4_LX_CENTER;
		updated = true;
	} else if (ps4IsLeftX(b)) {
		lx = b - PS4_LX_CENTER;
		updated = true;
	}

	if (ps4IsLeftY(b)) {
		ly = b - PS4_LY_CENTER;
		updated = true;
	} else if (ps4IsLeftY(a)) {
		ly = a - PS4_LY_CENTER;
		updated = true;
	}
	return updated;
}

struct Ps4TankOutput {
	int leftVel;
	int rightVel;
	bool stop;
};

inline Ps4TankOutput ps4TankFromStick(int lx, int ly, int deadzone = PS4_STICK_DEADZONE) {
	const int x = ps4ApplyDeadzone(lx, deadzone);
	const int y = ps4ApplyDeadzone(ly, deadzone);
	Ps4TankOutput out;
	out.stop = (x == 0 && y == 0);
	out.leftVel = y - x;
	out.rightVel = y + x;
	return out;
}

inline int ps4PwmFromVel(int vel) {
	int pwm = vel * 2;
	if (pwm < 0) {
		pwm = -pwm;
	}
	if (pwm > 255) {
		pwm = 255;
	}
	return pwm;
}

inline bool ps4DriveTimedOut(uint32_t now, uint32_t lastDriveMs, uint32_t timeoutMs = PS4_DRIVE_TIMEOUT_MS) {
	return static_cast<uint32_t>(now - lastDriveMs) >= timeoutMs;
}

#endif // PS4_DRIVE_H
