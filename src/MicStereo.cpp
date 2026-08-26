//
// Created by mr on 3/24/2024.
//

#include "MicStereo.h"
#include "Arduino.h"
#include "config.h"
#include "logger.h"
#include "main_ra.h"
#include "pwm_board.h"
#include "analog.h"
#include "sense_react.h"
#include "PS4.h"
#include "deadline.h"
#include "mic_peak.h"

constexpr uint32_t kMicSampleWindowMs = 50;

int MicStereo::baseRSound = 0;
int MicStereo::baseLSound = 0;

static MicPeakState sPeak;
static uint32_t sWindowStartMs = 0;
static bool sWindowActive = false;

void MicStereo::MicSetup() {
	MicStereo::baseRSound = analog::ext_analog_3;
	MicStereo::baseLSound = analog::ext_analog_1;
	#if LOG_VERBOSE
		if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
			logger::log(" L-Mic: ");
			logger::logInt(baseLSound);
			logger::log(" R-Mic: ");
			logger::logIntln(baseRSound);
		}
	#endif
	micPeakReset(sPeak);
	sWindowActive = false;
}

static void micApplyLeds(unsigned int micL255, unsigned int micR255) {
	if (micR255 > MicStereo::baseRSound) {
		#if LOG_VERBOSE
			if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
				logger::log(" R-Mic: ");
				logger::logInt(micR255);
			}
		#endif
		if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
			pwm_board::RGBled(micR255, micR255, 0);
		}
	}
	if (micL255 > MicStereo::baseLSound) {
		#if LOG_VERBOSE
			if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
				logger::log(" L-Mic: ");
				logger::logIntln(micL255);
			}
		#endif
		if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
			pwm_board::RGBled(0, micR255, micL255);
		}
	} else if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
		pwm_board::RGBled(0, micR255, 0);
	}
}

void MicStereo::MicLoop() {
	if (sense_react::isActive() || PS4::isManualControlActive()) {
		return;
	}

	const uint32_t now = millis();
	if (!sWindowActive) {
		sWindowStartMs = now;
		micPeakReset(sPeak);
		sWindowActive = true;
	}

	micPeakSample(sPeak,
		static_cast<uint16_t>(analog::ext_analog_1),
		static_cast<uint16_t>(analog::ext_analog_3));

	if (static_cast<uint32_t>(now - sWindowStartMs) < kMicSampleWindowMs) {
		return;
	}

	const unsigned int micR255 = map(analog::ext_analog_3, 0, 1023, 0, 255);
	const unsigned int micL255 = map(analog::ext_analog_1, 0, 1023, 0, 255);
	micApplyLeds(micL255, micR255);

	micPeakReset(sPeak);
	sWindowStartMs = now;
}
