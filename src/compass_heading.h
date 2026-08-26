//
// Compass heading on horizontal X/Z plane (Y up) — Arduino-free for native tests.
//

#ifndef COMPASS_HEADING_H
#define COMPASS_HEADING_H

#include <cmath>
#include <cstdint>

#ifndef COMPASS_PI
#define COMPASS_PI 3.14159265358979323846
#endif

enum class CompassCardinal : uint8_t {
	N = 0,
	E = 1,
	S = 2,
	W = 3,
};

inline float normalizeDegrees360(float deg) {
	while (deg < 0.0f) {
		deg += 360.0f;
	}
	while (deg >= 360.0f) {
		deg -= 360.0f;
	}
	return deg;
}

/** Horizontal heading: atan2(z, x) + declination (radians); Y is vertical. */
inline float compassHeadingDegrees(float magX, float magZ, float declinationRad = 0.0f) {
	const float heading = std::atan2(magZ, magX) + declinationRad;
	const float deg = heading * (180.0f / static_cast<float>(COMPASS_PI));
	return normalizeDegrees360(deg);
}

inline CompassCardinal cardinalFromDegrees(float deg) {
	deg = normalizeDegrees360(deg);
	if (deg >= 315.0f || deg < 45.0f) {
		return CompassCardinal::N;
	}
	if (deg < 135.0f) {
		return CompassCardinal::E;
	}
	if (deg < 225.0f) {
		return CompassCardinal::S;
	}
	return CompassCardinal::W;
}

inline const char* cardinalToString(CompassCardinal c) {
	switch (c) {
		case CompassCardinal::N: return "N";
		case CompassCardinal::E: return "E";
		case CompassCardinal::S: return "S";
		case CompassCardinal::W: return "W";
	}
	return "?";
}

#endif // COMPASS_HEADING_H
