#include "laser_beam.h"

#include <Arduino.h>
#include "config.h"
#include "main_ra.h"
#include "pwm_board.h"

bool laser_beam::sFull = false;
uint32_t laser_beam::sLastPressMs = 0;

void laser_beam::onPress(uint32_t nowMs) {
	sFull = true;
	sLastPressMs = nowMs;
	if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
		pwm_board::pwm.setPWM(LAZER_PIN, 0, LASER_PWM_FULL);
	}
}

void laser_beam::onRelease() {
	sFull = false;
}

void laser_beam::tick(uint32_t nowMs) {
	if (!sFull) {
		return;
	}
	if (static_cast<uint32_t>(nowMs - sLastPressMs) >= LASER_HOLD_TIMEOUT_MS) {
		sFull = false;
	}
}

bool laser_beam::isFull() {
	return sFull;
}

int laser_beam::pwmForDistance(int distanceMapped) {
	return sFull ? LASER_PWM_FULL : distanceMapped;
}
