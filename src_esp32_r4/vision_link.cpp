#include "vision_link.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

namespace {
constexpr int kVisionRxPin = 41;
constexpr int kVisionTxPin = 42;
constexpr uint32_t kVisionBaud = 230400;

VisionTelemetry sTelemetry{};
bool sHaveTelemetry = false;
uint32_t sLastTelemetryLocalMs = 0;
VisionGridSnapshot sGrid{};
bool sHaveGrid = false;
uint32_t sLastGridLocalMs = 0;
char sLine[704] = {};
uint16_t sLinePos = 0;
char sLastMessage[64] = "none";

void sendLine(const char* line) { Serial2.println(line); }

void handleLine(char* line, uint32_t now) {
    VisionTelemetry t;
    if (parseVisionTelemetry(line, t)) {
        sTelemetry = t;
        sHaveTelemetry = true;
        sLastTelemetryLocalMs = now;
        return;
    }
    VisionGridSnapshot g;
    if (parseVisionGridSnapshot(line, g)) {
        sGrid = g;
        sHaveGrid = true;
        sLastGridLocalMs = now;
        return;
    }
    if (strncmp(line, "FISHEYE,", 8) == 0 || strncmp(line, "VA,", 3) == 0) {
        strncpy(sLastMessage, line, sizeof(sLastMessage) - 1);
        sLastMessage[sizeof(sLastMessage) - 1] = '\0';
    }
}
}

namespace vision_link {
void begin() {
    Serial2.setRxBufferSize(2048);
    Serial2.setTxBufferSize(512);
    Serial2.begin(kVisionBaud, SERIAL_8N1, kVisionRxPin, kVisionTxPin);
    sHaveTelemetry = false;
    sHaveGrid = false;
    sLinePos = 0;
    sendLine("C,PING");
    sendLine("C,GRID,2");
}

void poll(uint32_t now) {
    uint16_t budget = 768;
    while (Serial2.available() > 0 && budget-- > 0) {
        const char c = static_cast<char>(Serial2.read());
        if (c == '\r') continue;
        if (c == '\n') {
            sLine[sLinePos] = '\0';
            if (sLinePos > 0) handleLine(sLine, now);
            sLinePos = 0;
            continue;
        }
        if (sLinePos < sizeof(sLine) - 1) sLine[sLinePos++] = c;
        else sLinePos = 0;
    }
}

bool online(uint32_t now, uint32_t maxAgeMs) {
    return sHaveTelemetry && static_cast<uint32_t>(now - sLastTelemetryLocalMs) <= maxAgeMs;
}
bool hasTelemetry() { return sHaveTelemetry; }
const VisionTelemetry& telemetry() { return sTelemetry; }
uint32_t telemetryAge(uint32_t now) {
    return sHaveTelemetry ? static_cast<uint32_t>(now - sLastTelemetryLocalMs) : 0xFFFFFFFFu;
}
const char* lastMessage() { return sLastMessage; }

bool gridOnline(uint32_t now, uint32_t maxAgeMs) {
    return sHaveGrid && static_cast<uint32_t>(now - sLastGridLocalMs) <= maxAgeMs;
}
bool hasGrid() { return sHaveGrid; }
const VisionGridSnapshot& grid() { return sGrid; }
uint32_t gridAge(uint32_t now) {
    return sHaveGrid ? static_cast<uint32_t>(now - sLastGridLocalMs) : 0xFFFFFFFFu;
}

void ping() { sendLine("C,PING"); }
void setThreshold(int value) {
    if (value < 2) value = 2;
    if (value > 96) value = 96;
    char b[32]; snprintf(b, sizeof(b), "C,THR,%d", value); sendLine(b);
}
void setRate(int fps) {
    if (fps < 5) fps = 5;
    if (fps > 40) fps = 40;
    char b[32]; snprintf(b, sizeof(b), "C,RATE,%d", fps); sendLine(b);
}
void setGridRate(int fps) {
    if (fps < 1) fps = 1;
    if (fps > 5) fps = 5;
    char b[32]; snprintf(b, sizeof(b), "C,GRID,%d", fps); sendLine(b);
}
void requestSnapshot() { sendLine("C,SNAPSHOT"); }
void setDebug(bool on) { sendLine(on ? "C,DEBUG,1" : "C,DEBUG,0"); }
}
