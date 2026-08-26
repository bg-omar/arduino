#include "radar_scan.h"

#include <Arduino.h>
#include <cstring>

#include "avoid_objects.h"
#include "config.h"
#include "deadline.h"
#include "logger.h"
#include "main_ra.h"
#include "motor.h"
#include "pwm_board.h"
#include "robot_modes.h"
#include "SD_card.h"

#include "SdFat.h"

enum class RadarPhase : uint8_t {
	Idle,
	Move,
	Settle,
	Ping,
	Write,
	Done,
};

static RadarPhase sPhase = RadarPhase::Idle;
static int sCellIndex = 0;
static uint32_t sPhaseStartMs = 0;
static double sLastDistCm = -1.0;
static int sServoXY = 90;
static int sServoZ = 135;
static bool sFromSenseIdle = false;

bool radar_scan::active = false;

int radar_scan::progressCell() {
	return sCellIndex;
}

int radar_scan::progressTotal() {
	return RADAR_CELL_COUNT;
}

static double radarPingCm() {
	digitalWrite(Trig_PIN, LOW);
	delayMicroseconds(2);
	digitalWrite(Trig_PIN, HIGH);
	delayMicroseconds(10);
	digitalWrite(Trig_PIN, LOW);

	const unsigned long duration = pulseIn(Echo_PIN, HIGH, RADAR_PING_TIMEOUT_US);
	if (duration == 0) {
		return -1.0;
	}
	return duration / RADAR_CM_PER_US;
}

static bool radarOpenNextFile(char* path, size_t pathLen) {
	for (int n = 1; n <= 999; ++n) {
		snprintf(path, pathLen, "SCAN%03d.CSV", n);
		if (!SD_card::fileExists(path)) {
			return true;
		}
	}
	return false;
}

static void radarCloseFile() {
	SD_card::closeDataFile();
}

static bool radarBeginFile(char* path, size_t pathLen) {
	if (!radarOpenNextFile(path, pathLen)) {
		return false;
	}
	if (!SD_card::openDataFile(path, FILE_WRITE)) {
		return false;
	}
	SD_card::printLine("# Wall-Z radar");
	SD_card::printLine("# pan_deg,tilt_deg,dist_cm,x_cm,y_cm,z_cm");
	return true;
}

static void radarWritePoint(double distCm, int servoXY, int servoZ) {
	const float panDeg = radarPanDegFromServo(servoXY);
	const float tiltDeg = radarTiltDegFromServo(servoZ);
	const RadarXYZ xyz = radarDistToXYZ(distCm, panDeg, tiltDeg);
	char line[80];
	snprintf(line, sizeof(line), "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
		static_cast<double>(panDeg),
		static_cast<double>(tiltDeg),
		distCm,
		xyz.x,
		xyz.y,
		xyz.z);
	SD_card::printLine(line);
}

static void radarSetHead(int xy, int z) {
	if (!FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
		return;
	}
	pwm_board::posXY = xy;
	pwm_board::posZ = z;
	pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(xy));
	pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(z));
}

void radar_scan::start() {
	if (active) {
		return;
	}
	if (!FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) {
		return;
	}
	if (!main::use_sd_card) {
		logger::logln("Radar: SD required");
		return;
	}
	robot_modes::stopAll();
	Motor::Car_Stop();
	sFromSenseIdle = false;
	sCellIndex = 0;
	sPhase = RadarPhase::Move;
	sPhaseStartMs = millis();
	active = true;
	logger::logln("Radar scan start");
}

void radar_scan::startSenseIdle() {
	if (active) {
		return;
	}
	sFromSenseIdle = true;
	start();
}

void radar_scan::stop() {
	if (!active) {
		return;
	}
	radarCloseFile();
	sPhase = RadarPhase::Idle;
	sCellIndex = 0;
	active = false;
	sFromSenseIdle = false;
	Motor::Car_Stop();
	logger::logln("Radar scan stop");
}

bool radar_scan::isActive() {
	return active;
}

void radar_scan::tick() {
	if (!active) {
		return;
	}

	const uint32_t now = millis();

	switch (sPhase) {
		case RadarPhase::Move:
			if (sCellIndex == 0) {
				char path[16];
				if (!radarBeginFile(path, sizeof(path))) {
					stop();
					return;
				}
				logger::log("Radar file ");
				logger::logln(path);
			}
			radarCellServo(sCellIndex, sServoXY, sServoZ);
			radarSetHead(sServoXY, sServoZ);
			sPhase = RadarPhase::Settle;
			sPhaseStartMs = now;
			break;

		case RadarPhase::Settle:
			if (!elapsed(now, sPhaseStartMs, RADAR_SETTLE_MS)) {
				return;
			}
			sPhase = RadarPhase::Ping;
			break;

		case RadarPhase::Ping:
			sLastDistCm = radarPingCm();
			if (radarDistanceValid(sLastDistCm)) {
				avoid_objects::distanceF = sLastDistCm;
				avoid_objects::distanceReadMs = now;
			}
			sPhase = RadarPhase::Write;
			break;

		case RadarPhase::Write:
			if (radarDistanceValid(sLastDistCm)) {
				radarWritePoint(sLastDistCm, sServoXY, sServoZ);
			}
			if ((sCellIndex % 19) == 0) {
				logger::log("Radar ");
				logger::logInt(sCellIndex);
				logger::log("/");
				logger::logIntln(RADAR_CELL_COUNT);
			}
			sCellIndex = radarNextCellIndex(sCellIndex);
			if (radarScanComplete(sCellIndex)) {
				sPhase = RadarPhase::Done;
			} else {
				sPhase = RadarPhase::Move;
			}
			break;

		case RadarPhase::Done:
			radarCloseFile();
			sPhase = RadarPhase::Idle;
			active = false;
			sFromSenseIdle = false;
			logger::logln("Radar scan done");
			break;

		case RadarPhase::Idle:
		default:
			break;
	}
}
