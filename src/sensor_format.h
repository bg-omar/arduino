//
// Sensor OLED / impact-log line formatters — Arduino-free for native tests.
//

#ifndef SENSOR_FORMAT_H
#define SENSOR_FORMAT_H

#include <cstdio>

inline int formatHeadingLine(char* buf, size_t n, double headingDeg, const char* cardinal) {
	return snprintf(buf, n, "%.0f %s", headingDeg, cardinal != nullptr ? cardinal : "?");
}

inline int formatAccelLine(char* buf, size_t n, float ax, float ay, float az) {
	return snprintf(buf, n, "A%.1f %.1f %.1f", ax, ay, az);
}

inline int formatGyroLine(char* buf, size_t n, float gx, float gy, float gz) {
	return snprintf(buf, n, "G%.1f %.1f %.1f", gx, gy, gz);
}

inline int formatAccelGyroCompact(char* buf, size_t n,
	float ax, float ay, float az, float gx, float gy, float gz) {
	return snprintf(buf, n, "A%.1f %.1f %.1f G%.1f %.1f %.1f",
		ax, ay, az, gx, gy, gz);
}

inline int formatBaroLine(char* buf, size_t n, float tempC, float pressureHpa) {
	return snprintf(buf, n, "%.1fC %.0fhPa", tempC, pressureHpa);
}

inline int formatMicLine(char* buf, size_t n, int micL, int micR) {
	return snprintf(buf, n, "M%d/%d", micL, micR);
}

inline int formatLightLine(char* buf, size_t n, int lightL, int lightR) {
	return snprintf(buf, n, "Lt%d/%d", lightL, lightR);
}

inline int formatMicLightCompact(char* buf, size_t n,
	int micL, int micR, int lightL, int lightR) {
	return snprintf(buf, n, "M%d/%d Lt%d/%d", micL, micR, lightL, lightR);
}

inline int formatDistLine(char* buf, size_t n, double distCm) {
	return snprintf(buf, n, "%.0f cm", distCm);
}

constexpr int SENSOR_DIST_DISPLAY_MAX_CM = 500;
constexpr int SENSOR_ANALOG_MAX = 2048;
constexpr int SENSOR_BAR_MAX_PX = 40;

inline int sensorBarPx(int value, int maxVal, int maxPx) {
	if (maxVal <= 0 || maxPx <= 0 || value <= 0) {
		return 0;
	}
	int px = (value * maxPx) / maxVal;
	if (px > maxPx) {
		px = maxPx;
	}
	return px;
}

inline int formatDistMeters(char* buf, size_t n, double distCm) {
	if (distCm < 0.0) {
		return snprintf(buf, n, "-- m");
	}
	if (distCm > static_cast<double>(SENSOR_DIST_DISPLAY_MAX_CM)) {
		distCm = static_cast<double>(SENSOR_DIST_DISPLAY_MAX_CM);
	}
	return snprintf(buf, n, "%.1f m", distCm / 100.0);
}

inline int distDisplayBarPx(double distCm) {
	if (distCm < 0.0) {
		return 0;
	}
	return sensorBarPx(static_cast<int>(distCm), SENSOR_DIST_DISPLAY_MAX_CM, 50);
}

inline int formatBaroTempCompact(char* buf, size_t n, float tempC) {
	return snprintf(buf, n, "%.0fC", tempC);
}

inline int formatBaroDistCompact(char* buf, size_t n,
	float tempC, float pressureHpa, double distCm) {
	return snprintf(buf, n, "%.1fC %.0fh %.0fcm", tempC, pressureHpa, distCm);
}

inline int formatImpactAccelGyro(char* buf, size_t n,
	float ax, float ay, float az, float gx, float gy, float gz) {
	return snprintf(buf, n, "IMP A%.1f,%.1f,%.1f G%.1f,%.1f,%.1f",
		ax, ay, az, gx, gy, gz);
}

inline int formatImpactBaro(char* buf, size_t n, float tempC, float pressureHpa) {
	return snprintf(buf, n, " B T%.1f P%.0f", tempC, pressureHpa);
}

inline int formatImpactMicLight(char* buf, size_t n,
	int micL, int micR, int lightL, int lightR) {
	return snprintf(buf, n, " M%d/%d L%d/%d", micL, micR, lightL, lightR);
}

inline int formatImpactDist(char* buf, size_t n, double distCm) {
	return snprintf(buf, n, " D%.0f", distCm);
}

#endif // SENSOR_FORMAT_H
