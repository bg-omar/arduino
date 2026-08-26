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
CameraStorageStatus sStorage{};
bool sHaveStorage = false;
uint32_t sLastStorageLocalMs = 0;
char sLine[704] = {};
uint16_t sLinePos = 0;
char sLastMessage[64] = "none";

void sendLine(const char* line) { Serial2.println(line); }

const char* safeToken(const char* s, const char* fallback) {
    if (!s || !*s) return fallback;
    for (const char* p=s; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) return fallback;
    }
    return s;
}

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

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
    CameraStorageStatus st;
    if (wallzParseStorageStatus(line, st)) {
        sStorage = st;
        sHaveStorage = true;
        sLastStorageLocalMs = now;
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
    Serial2.setTxBufferSize(1024);
    Serial2.begin(kVisionBaud, SERIAL_8N1, kVisionRxPin, kVisionTxPin);
    sHaveTelemetry = false;
    sHaveGrid = false;
    sHaveStorage = false;
    sLinePos = 0;
    sendLine("C,PING");
    sendLine("C,GRID,2");
    sendLine("C,SDSTATUS");
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

bool hasStorageStatus() { return sHaveStorage; }
const CameraStorageStatus& storageStatus() { return sStorage; }
uint32_t storageStatusAge(uint32_t now) {
    return sHaveStorage ? static_cast<uint32_t>(now - sLastStorageLocalMs) : 0xFFFFFFFFu;
}

void ping() { sendLine("C,PING"); }
void setThreshold(int value) {
    value = clampInt(value, 2, 96);
    char b[32]; snprintf(b, sizeof(b), "C,THR,%d", value); sendLine(b);
}
void setRate(int fps) {
    fps = clampInt(fps, 5, 40);
    char b[32]; snprintf(b, sizeof(b), "C,RATE,%d", fps); sendLine(b);
}
void setGridRate(int fps) {
    fps = clampInt(fps, 1, 5);
    char b[32]; snprintf(b, sizeof(b), "C,GRID,%d", fps); sendLine(b);
}
void requestSnapshot() { sendLine("C,SNAPSHOT"); }
void setDebug(bool on) { sendLine(on ? "C,DEBUG,1" : "C,DEBUG,0"); }

void sendContext(const char* label, int familiarity, int novelty, int valueMilli) {
    char b[96];
    snprintf(b, sizeof(b), "C,CTX,%s,%d,%d,%d",
             safeToken(label, "unknown"),
             clampInt(familiarity, 0, 1000),
             clampInt(novelty, 0, 1000),
             clampInt(valueMilli, -1000, 1000));
    sendLine(b);
}

void requestStore(const char* reason, const char* label) {
    char b[80];
    snprintf(b, sizeof(b), "C,SAVE,%s,%s", safeToken(reason, "manual"), safeToken(label, "unknown"));
    sendLine(b);
}

void setStorageEnabled(bool on) { sendLine(on ? "C,SD,1" : "C,SD,0"); }
void requestStorageStatus() { sendLine("C,SDSTATUS"); }
}
