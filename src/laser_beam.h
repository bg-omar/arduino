//
// L3 laser full-on while pressed (no toggle).
//

#ifndef LASER_BEAM_H
#define LASER_BEAM_H

#include <cstdint>

constexpr int LASER_PWM_FULL = 4000;
constexpr uint32_t LASER_HOLD_TIMEOUT_MS = 280;

class laser_beam {
public:
	static void onPress(uint32_t nowMs);
	static void onRelease();
	static void tick(uint32_t nowMs);
	static bool isFull();
	// Prefer full PWM while held; otherwise return distanceMapped.
	static int pwmForDistance(int distanceMapped);

private:
	static bool sFull;
	static uint32_t sLastPressMs;
};

#endif // LASER_BEAM_H
