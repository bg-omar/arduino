//
// Sense React threshold scaling — Arduino-free for native tests.
//
#ifndef SENSE_THRESHOLDS_H
#define SENSE_THRESHOLDS_H

#include <cstdint>

constexpr float SENSE_DISTANCE_ALERT_CM = 35.0f;
constexpr int SENSE_LIGHT_THRESH = 650;
constexpr int SENSE_LIGHT_BIAS = 60;
constexpr int SENSE_MIC_LOUD = 80;
constexpr int SENSE_MIC_BIAS = 40;
constexpr float SENSE_BARO_DELTA = 0.5f;

constexpr uint32_t SENSE_SEEK_AFTER_MS = 4000;
constexpr uint32_t SENSE_SEEK_RAMP_MS = 8000;

struct SenseThresholds {
	float distanceCm;
	int micLoud;
	int micBias;
	int lightThresh;
	int lightBias;
	float baroDelta;
};

inline SenseThresholds senseDefaultThresholds() {
	return SenseThresholds{
		SENSE_DISTANCE_ALERT_CM,
		SENSE_MIC_LOUD,
		SENSE_MIC_BIAS,
		SENSE_LIGHT_THRESH,
		SENSE_LIGHT_BIAS,
		SENSE_BARO_DELTA,
	};
}

inline float senseThresholdScale(uint32_t idleStreakMs) {
	if (idleStreakMs <= SENSE_SEEK_AFTER_MS) {
		return 1.0f;
	}
	const uint32_t rampMs = idleStreakMs - SENSE_SEEK_AFTER_MS;
	if (rampMs >= SENSE_SEEK_RAMP_MS) {
		return 0.6f;
	}
	return 1.0f - (0.4f * (float)rampMs / (float)SENSE_SEEK_RAMP_MS);
}

inline SenseThresholds senseEffectiveThresholds(uint32_t idleStreakMs) {
	const SenseThresholds base = senseDefaultThresholds();
	const float scale = senseThresholdScale(idleStreakMs);
	return SenseThresholds{
		base.distanceCm * scale,
		(int)(base.micLoud * scale),
		(int)(base.micBias * scale),
		(int)(base.lightThresh * scale),
		(int)(base.lightBias * scale),
		base.baroDelta * scale,
	};
}

#endif // SENSE_THRESHOLDS_H
