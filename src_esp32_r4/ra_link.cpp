#include "ra_link.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <esp_uno_r4.h>

namespace {
BrainTelemetry sTelemetry{};
bool sHaveTelemetry = false;
uint32_t sLastTelemetryLocalMs = 0;
uint32_t sLastHeartbeatMs = 0;
char sLine[192] = {};
uint16_t sLinePos = 0;
char sLastAck[64] = "none";

void sendLine(const char* line) {
    SERIAL_AT.println(line);
}

void handleLine(char* line, uint32_t now) {
    BrainTelemetry t;
    if (parseBrainTelemetry(line, t)) {
        sTelemetry = t;
        sHaveTelemetry = true;
        sLastTelemetryLocalMs = now;
        return;
    }
    if (strncmp(line, "A,", 2) == 0 || strncmp(line, "RA,", 3) == 0) {
        strncpy(sLastAck, line, sizeof(sLastAck)-1);
        sLastAck[sizeof(sLastAck)-1] = '\0';
    }
}
}

namespace ra_link {
void begin() {
    // esp_uno_r4_setup() already configured SERIAL_AT = ESP32-S3 Serial1
    // at 115200 baud on the onboard RA4M1 link (GPIO 6/5).
    sHaveTelemetry = false;
    sLastHeartbeatMs = 0;
    sendLine("B,HB");
}

void poll(uint32_t now) {
    uint16_t budget = 256;
    while (SERIAL_AT.available() > 0 && budget-- > 0) {
        const char c = static_cast<char>(SERIAL_AT.read());
        if (c == '\r') continue;
        if (c == '\n') {
            sLine[sLinePos] = '\0';
            if (sLinePos > 0) handleLine(sLine, now);
            sLinePos = 0;
            continue;
        }
        if (sLinePos < sizeof(sLine)-1) sLine[sLinePos++] = c;
        else sLinePos = 0;
    }
}

void heartbeat(uint32_t now) {
    if (static_cast<uint32_t>(now - sLastHeartbeatMs) >= 250) {
        sLastHeartbeatMs = now;
        sendLine("B,HB");
    }
}

bool online(uint32_t now, uint32_t maxAgeMs) {
    return sHaveTelemetry && static_cast<uint32_t>(now - sLastTelemetryLocalMs) <= maxAgeMs;
}

bool hasTelemetry() { return sHaveTelemetry; }
const BrainTelemetry& telemetry() { return sTelemetry; }
uint32_t telemetryAge(uint32_t now) { return sHaveTelemetry ? static_cast<uint32_t>(now - sLastTelemetryLocalMs) : 0xFFFFFFFFu; }
const char* lastAck() { return sLastAck; }

void arm(bool enable) {
    sendLine(enable ? "B,ARM,1" : "B,ARM,0");
}
void stop() { sendLine("B,STOP"); }
void brake() { sendLine("B,BRAKE"); }
void head(int xy, int z) {
    char b[48];
    snprintf(b, sizeof(b), "B,HEAD,%d,%d", xy, z);
    sendLine(b);
}
void move(char direction, uint32_t durationMs) {
    char b[48];
    snprintf(b, sizeof(b), "B,MOVE,%c,%lu", direction, static_cast<unsigned long>(durationMs));
    sendLine(b);
}
}
