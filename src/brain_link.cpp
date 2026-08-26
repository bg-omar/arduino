#include "brain_link.h"

#include <Arduino.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "PS4.h"
#include "analog.h"
#include "avoid_objects.h"
#include "brain_protocol.h"
#include "config.h"
#include "gyroscope.h"
#include "main_ra.h"
#include "manual_demo.h"
#include "motor.h"
#include "pwm_board.h"
#include "robot_modes.h"
#include "shared_control.h"

namespace {
constexpr uint32_t kTelemetryPeriodMs = 100;
constexpr uint32_t kHeartbeatTimeoutMs = 1200;
constexpr uint32_t kMaxMoveMs = 350;

bool sArmed = false;
bool sManualOverride = false;
bool sMoveActive = false;
uint32_t sMoveDeadlineMs = 0;
uint32_t sLastHeartbeatMs = 0;
uint32_t sLastTelemetryMs = 0;
char sRxLine[96] = {};
uint8_t sRxPos = 0;

bool deadlineReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}

int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void sendAck(const char* text) {
    SERIAL_AT.print("A,");
    SERIAL_AT.println(text);
}

bool ps4DriveManualActive() {
    return PS4::manualSnapshot().drive_active;
}

void cancelBrainMotion(bool stopMotor) {
    // A manual DRIVE command may already have replaced the motor PWM, so a
    // Brain stop must not overwrite it. Head-only manual input does not own
    // the motors and therefore does not suppress a required motor stop.
    if (stopMotor && sMoveActive && !ps4DriveManualActive()) {
        Motor::Car_Stop();
    }
    sMoveActive = false;
    sMoveDeadlineMs = 0;
}

void setArmed(bool armed, uint32_t now) {
    if (!armed) {
        cancelBrainMotion(true);
        sArmed = false;
        sManualOverride = false;
        sendAck("ARM,0");
        return;
    }
    // v0.6: an already-connected PS4 is compatible with Brain autonomy.
    // Active manual input temporarily owns the actuators; arming is retained.
    if (robot_modes::anyActive()) {
        sendAck("REJECT,ROBOTMODE");
        return;
    }
    sArmed = true;
    sManualOverride = PS4::isManualControlActive();
    sLastHeartbeatMs = now;
    sendAck("ARM,1");
}

void applyHead(int xy, int z) {
    if (!FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
        sendAck("REJECT,NO_HEAD");
        return;
    }
    xy = clampInt(xy, 10, 170);
    z = clampInt(z, 5, 150);
    pwm_board::posXY = xy;
    pwm_board::posZ = z;
    pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(xy));
    pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(z));
    sendAck("HEAD,OK");
}

int readDistanceMm(uint32_t now) {
    if (!FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) return -1;
    if (avoid_objects::cachedDistanceFresh(now, 250) && avoid_objects::distanceF >= 0.0) {
        return static_cast<int>(std::lround(avoid_objects::distanceF * 10.0));
    }
    const double cm = avoid_objects::checkDistanceLong(BRAIN_SONAR_TIMEOUT_US);
    if (cm < 0.0) return -1;
    return static_cast<int>(std::lround(cm * 10.0));
}

bool forwardIsSafe(uint32_t now) {
    if (!FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) return false;
    return brainForwardIsSafe(readDistanceMm(now));
}

void applyMove(char direction, uint32_t durationMs, uint32_t now) {
    if (!sArmed) {
        sendAck("REJECT,DISARMED");
        return;
    }
    if (PS4::isManualControlActive()) {
        cancelBrainMotion(false);
        sManualOverride = true;
        sendAck("REJECT,MANUAL_OVERRIDE");
        return;
    }
    if (robot_modes::anyActive()) {
        sendAck("REJECT,ROBOTMODE");
        return;
    }

    durationMs = clampInt(static_cast<int>(durationMs), 50, static_cast<int>(kMaxMoveMs));
    switch (direction) {
        case 'F':
            if (!forwardIsSafe(now)) {
                Motor::Car_Stop();
                sendAck("REJECT,SONAR");
                return;
            }
            Motor::Car_creepForward();
            break;
        case 'B':
            Motor::Car_creepBack();
            break;
        case 'L': Motor::Car_creepLeft(); break;
        case 'R': Motor::Car_creepRight(); break;
        default:
            sendAck("REJECT,DIRECTION");
            return;
    }
    sMoveActive = true;
    sMoveDeadlineMs = now + durationMs;
    sendAck("MOVE,OK");
}

void handleCommand(char* line, uint32_t now) {
    if (line == nullptr || line[0] != 'B' || line[1] != ',') return;
    char* save = nullptr;
    char* token = strtok_r(line + 2, ",", &save);
    if (token == nullptr) return;

    if (strcmp(token, "HB") == 0) {
        sLastHeartbeatMs = now;
        return;
    }
    if (strcmp(token, "STOP") == 0) {
        cancelBrainMotion(false);
        sArmed = false;
        Motor::Car_Stop();
        sendAck("STOP,OK");
        return;
    }
    if (strcmp(token, "BRAKE") == 0) {
        cancelBrainMotion(true);
        sendAck("BRAKE,OK");
        return;
    }
    if (strcmp(token, "ARM") == 0) {
        char* value = strtok_r(nullptr, ",", &save);
        setArmed(value != nullptr && atoi(value) != 0, now);
        return;
    }
    if (strcmp(token, "HEAD") == 0) {
        if (!sArmed || robot_modes::anyActive()) {
            sendAck("REJECT,HEAD_LOCK");
            return;
        }
        if (PS4::isManualControlActive()) {
            sManualOverride = true;
            sendAck("REJECT,MANUAL_OVERRIDE");
            return;
        }
        char* xy = strtok_r(nullptr, ",", &save);
        char* z = strtok_r(nullptr, ",", &save);
        if (xy && z) applyHead(atoi(xy), atoi(z));
        return;
    }
    if (strcmp(token, "MOVE") == 0) {
        char* dir = strtok_r(nullptr, ",", &save);
        char* duration = strtok_r(nullptr, ",", &save);
        if (dir && duration && dir[0] != '\0') {
            applyMove(dir[0], static_cast<uint32_t>(atoi(duration)), now);
        }
        return;
    }
}

void pollCommands(uint32_t now) {
    uint8_t byteBudget = 96;
    while (SERIAL_AT.available() > 0 && byteBudget-- > 0) {
        const char c = static_cast<char>(SERIAL_AT.read());
        if (c == '\r') continue;
        if (c == '\n') {
            sRxLine[sRxPos] = '\0';
            if (sRxPos > 0) handleCommand(sRxLine, now);
            sRxPos = 0;
            continue;
        }
        if (sRxPos < sizeof(sRxLine) - 1) {
            sRxLine[sRxPos++] = c;
        } else {
            sRxPos = 0;
        }
    }
}

void sendTelemetry(uint32_t now) {
    int distanceMm = -1;
    if (FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) {
        distanceMm = readDistanceMm(now);
    }

    const int lightL = FEATURE_ENABLED(main::use_analog, USE_ANALOG) ? analog::ext_analog_0 : 0;
    const int micL   = FEATURE_ENABLED(main::use_analog, USE_ANALOG) ? analog::ext_analog_1 : 0;
    const int lightR = FEATURE_ENABLED(main::use_analog, USE_ANALOG) ? analog::ext_analog_2 : 0;
    const int micR   = FEATURE_ENABLED(main::use_analog, USE_ANALOG) ? analog::ext_analog_3 : 0;
    const int gx = FEATURE_ENABLED(main::use_gyro, USE_GYRO) ? static_cast<int>(std::lround(gyroscope::gx * 1000.0f)) : 0;
    const int gy = FEATURE_ENABLED(main::use_gyro, USE_GYRO) ? static_cast<int>(std::lround(gyroscope::gy * 1000.0f)) : 0;
    const int gz = FEATURE_ENABLED(main::use_gyro, USE_GYRO) ? static_cast<int>(std::lround(gyroscope::gz * 1000.0f)) : 0;

    char line[192];
    snprintf(line, sizeof(line),
        "T,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
        static_cast<unsigned long>(now), distanceMm,
        lightL, lightR, micL, micR,
        gx, gy, gz,
        pwm_board::posXY, pwm_board::posZ,
        PS4::isManualControlActive() ? 1 : 0,
        robot_modes::anyActive() ? 1 : 0,
        sArmed ? 1 : 0);
    SERIAL_AT.println(line);

    // Brain v0.4 imitation-learning side channel. This mirrors the already
    // applied PS4 intent; it never changes motor/servo authority on the RA4M1.
    const Ps4ManualSnapshot demo = PS4::manualSnapshot();
    if (demo.drive_active || demo.head_active) {
        char dline[96];
        snprintf(dline, sizeof(dline),
            "D,%lu,%d,%d,%d,%d,%d,%d",
            static_cast<unsigned long>(demo.ms),
            demo.lx, demo.ly, demo.rx, demo.ry,
            demo.drive_active ? 1 : 0, demo.head_active ? 1 : 0);
        SERIAL_AT.println(dline);
    }
}
}

namespace brain_link {
void begin() {
    sArmed = false;
    sManualOverride = false;
    sMoveActive = false;
    sLastHeartbeatMs = millis();
    sLastTelemetryMs = 0;
    SERIAL_AT.println("RA,HELLO,BRAIN,0.6.0");
}

void poll(uint32_t now) {
    pollCommands(now);

    const Ps4ManualSnapshot manual = PS4::manualSnapshot();
    const bool manualNow = manual.drive_active || manual.head_active;
    if (manualNow && sArmed) {
        // Shared control: the PS4 owns the actuators only while there is real
        // manual input. A drive command already replaces motor PWM, so clear
        // the old Brain pulse without STOP. For head-only input, keep any
        // existing Brain move deadline intact so it still expires safely.
        if (manual.drive_active) cancelBrainMotion(false);
        if (!sManualOverride) sendAck("PAUSE,MANUAL");
        sManualOverride = true;
    } else if (!manualNow && sManualOverride) {
        sManualOverride = false;
        if (sArmed) sendAck("RESUME,BRAIN");
    }

    if (sArmed && static_cast<uint32_t>(now - sLastHeartbeatMs) > kHeartbeatTimeoutMs) {
        cancelBrainMotion(true);
        sArmed = false;
        sendAck("DISARM,HEARTBEAT");
    }

    if (sMoveActive && deadlineReached(now, sMoveDeadlineMs)) {
        cancelBrainMotion(true);
    }
}

void tick(uint32_t now) {
    poll(now);

    if (static_cast<uint32_t>(now - sLastTelemetryMs) >= kTelemetryPeriodMs) {
        sLastTelemetryMs = now;
        sendTelemetry(now);
    }
}

bool isArmed() { return sArmed; }

void userStop() {
    cancelBrainMotion(true);
    sArmed = false;
    sManualOverride = false;
    Motor::Car_Stop();
    SERIAL_AT.println("U,MODE,STOP");
    sendAck("USER,STOP");
}

void userStartBrain(bool imitation) {
    if (robot_modes::anyActive()) {
        sendAck("REJECT,ROBOTMODE");
        return;
    }
    setArmed(true, millis());
    SERIAL_AT.println(imitation ? "U,MODE,IMITATION" : "U,MODE,BRAIN");
}

void userSelectRobotMode(const char* mode) {
    cancelBrainMotion(true);
    sArmed = false;
    sManualOverride = false;
    if (mode && mode[0] != '\0') {
        SERIAL_AT.print("U,MODE,");
        SERIAL_AT.println(mode);
    } else {
        SERIAL_AT.println("U,MODE,STOP");
    }
}
}
