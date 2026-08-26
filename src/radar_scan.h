//
// 180° pan × tilt ultrasonic grid → XYZ point cloud. Arduino-free math for native tests.
//
#ifndef RADAR_SCAN_H
#define RADAR_SCAN_H

#include <cmath>
#include <cstdint>

constexpr int RADAR_PAN_MIN_XY = 0;
constexpr int RADAR_PAN_MAX_XY = 180;
constexpr int RADAR_PAN_STEP_XY = 10;
constexpr int RADAR_TILT_START_Z = 135;
constexpr int RADAR_TILT_END_Z = 0;
constexpr int RADAR_TILT_STEP_Z = 15;

constexpr int RADAR_PAN_COLS = ((RADAR_PAN_MAX_XY - RADAR_PAN_MIN_XY) / RADAR_PAN_STEP_XY) + 1;
constexpr int RADAR_TILT_ROWS =
	((RADAR_TILT_START_Z - RADAR_TILT_END_Z) / RADAR_TILT_STEP_Z) + 1;
constexpr int RADAR_CELL_COUNT = RADAR_PAN_COLS * RADAR_TILT_ROWS;

constexpr uint32_t RADAR_SETTLE_MS = 40;
constexpr double RADAR_CM_PER_US = 58.0;
constexpr int RADAR_MAX_CM = 500;
constexpr uint32_t RADAR_PING_TIMEOUT_US =
	static_cast<uint32_t>(RADAR_MAX_CM * RADAR_CM_PER_US);
constexpr uint32_t RADAR_SENSE_IDLE_AFTER_MS = 6000;
constexpr uint32_t RADAR_SENSE_COOLDOWN_MS = 60000;

struct RadarXYZ {
	double x;
	double y;
	double z;
};

inline float radarDegToRad(float deg) {
	return deg * 3.14159265f / 180.0f;
}

inline float radarPanDegFromServo(int servoXY) {
	return 90.0f - static_cast<float>(servoXY);
}

inline float radarTiltDegFromServo(int servoZ) {
	return 90.0f - static_cast<float>(servoZ);
}

inline bool radarDistanceValid(double distCm) {
	return distCm >= 0.0 && distCm <= static_cast<double>(RADAR_MAX_CM);
}

inline RadarXYZ radarDistToXYZ(double distCm, float panDeg, float tiltDeg) {
	const float panRad = radarDegToRad(panDeg);
	const float tiltRad = radarDegToRad(tiltDeg);
	const float d = static_cast<float>(distCm);
	const float cosT = cosf(tiltRad);
	RadarXYZ out{};
	out.x = static_cast<double>(d * cosT * sinf(panRad));
	out.y = static_cast<double>(d * cosT * cosf(panRad));
	out.z = static_cast<double>(d * sinf(tiltRad));
	return out;
}

inline int radarPanServoForCol(int col) {
	return RADAR_PAN_MIN_XY + col * RADAR_PAN_STEP_XY;
}

inline int radarTiltServoForRow(int row) {
	return RADAR_TILT_START_Z - row * RADAR_TILT_STEP_Z;
}

inline int radarColForCell(int cellIndex, int row) {
	const bool reverse = (row % 2) != 0;
	const int col = cellIndex % RADAR_PAN_COLS;
	if (reverse) {
		return (RADAR_PAN_COLS - 1) - col;
	}
	return col;
}

inline int radarRowForCell(int cellIndex) {
	return cellIndex / RADAR_PAN_COLS;
}

inline void radarCellServo(int cellIndex, int& servoXY, int& servoZ) {
	const int row = radarRowForCell(cellIndex);
	const int col = radarColForCell(cellIndex, row);
	servoXY = radarPanServoForCol(col);
	servoZ = radarTiltServoForRow(row);
}

inline bool radarCellIndexValid(int cellIndex) {
	return cellIndex >= 0 && cellIndex < RADAR_CELL_COUNT;
}

inline int radarNextCellIndex(int cellIndex) {
	return cellIndex + 1;
}

inline bool radarScanComplete(int cellIndex) {
	return cellIndex >= RADAR_CELL_COUNT;
}

class radar_scan {
public:
	static void start();
	static void startSenseIdle();
	static void stop();
	static void tick();
	static bool isActive();
	static int progressCell();
	static int progressTotal();

private:
	static bool active;
};

#endif // RADAR_SCAN_H
