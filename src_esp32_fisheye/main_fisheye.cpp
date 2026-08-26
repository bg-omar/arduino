/*
 * Wall-Z Fisheye Vision Node v0.2.0 (Brain v0.3)
 * Target: AI-Thinker ESP32-CAM + OV2640 fisheye lens
 *
 * Fast path: 160x120 grayscale -> 20x15 local motion grid @ ~20 Hz.
 * Learning path: compact 20x15 grayscale snapshots -> onboard ESP32-S3 @ 2 Hz.
 * No image frames and no motor authority are transported over the Brain UART.
 */
#include <Arduino.h>
#include "esp_camera.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "vision_grid.h"

namespace {
constexpr char kVersion[] = "0.2.0";
constexpr int PWDN_GPIO_NUM = 32;
constexpr int RESET_GPIO_NUM = -1;
constexpr int XCLK_GPIO_NUM = 0;
constexpr int SIOD_GPIO_NUM = 26;
constexpr int SIOC_GPIO_NUM = 27;
constexpr int Y9_GPIO_NUM = 35;
constexpr int Y8_GPIO_NUM = 34;
constexpr int Y7_GPIO_NUM = 39;
constexpr int Y6_GPIO_NUM = 36;
constexpr int Y5_GPIO_NUM = 21;
constexpr int Y4_GPIO_NUM = 19;
constexpr int Y3_GPIO_NUM = 18;
constexpr int Y2_GPIO_NUM = 5;
constexpr int VSYNC_GPIO_NUM = 25;
constexpr int HREF_GPIO_NUM = 23;
constexpr int PCLK_GPIO_NUM = 22;

// GPIO13/14 are free because this dedicated vision node does not use its SD slot.
constexpr int kBrainRxPin = 13;
constexpr int kBrainTxPin = 14;
constexpr uint32_t kBrainBaud = 230400;

uint8_t previousGrid[WALLZ_VISION_GRID_CELLS] = {};
uint8_t currentGrid[WALLZ_VISION_GRID_CELLS] = {};
bool havePrevious = false;
uint8_t motionPixelThreshold = 18;
uint8_t targetFps = 20;
uint8_t gridFps = 2;
bool debugEnabled = false;
bool forceSnapshot = false;
uint32_t frameCounter = 0;
uint32_t fpsWindowStartMs = 0;
uint32_t lastFrameStartMs = 0;
uint32_t lastDebugMs = 0;
uint32_t lastGridMs = 0;
int fpsX10 = 0;
uint16_t gridSeq = 0;
char rxLine[64] = {};
uint8_t rxPos = 0;

void sendAck(const char* msg) {
    Serial2.print("VA,");
    Serial2.println(msg);
}

void handleCommand(char* line) {
    if (!line || line[0] != 'C' || line[1] != ',') return;
    char* save = nullptr;
    char* cmd = strtok_r(line + 2, ",", &save);
    if (!cmd) return;

    if (strcmp(cmd, "PING") == 0) {
        Serial2.print("FISHEYE,HELLO,");
        Serial2.println(kVersion);
        return;
    }
    if (strcmp(cmd, "THR") == 0) {
        char* value = strtok_r(nullptr, ",", &save);
        if (value) motionPixelThreshold = static_cast<uint8_t>(wallzVisionClamp(atoi(value), 2, 96));
        char b[32]; snprintf(b, sizeof(b), "THR,%u", motionPixelThreshold); sendAck(b);
        return;
    }
    if (strcmp(cmd, "RATE") == 0) {
        char* value = strtok_r(nullptr, ",", &save);
        if (value) targetFps = static_cast<uint8_t>(wallzVisionClamp(atoi(value), 5, 40));
        char b[32]; snprintf(b, sizeof(b), "RATE,%u", targetFps); sendAck(b);
        return;
    }
    if (strcmp(cmd, "GRID") == 0) {
        char* value = strtok_r(nullptr, ",", &save);
        if (value) gridFps = static_cast<uint8_t>(wallzVisionClamp(atoi(value), 1, 5));
        char b[32]; snprintf(b, sizeof(b), "GRID,%u", gridFps); sendAck(b);
        return;
    }
    if (strcmp(cmd, "SNAPSHOT") == 0) {
        forceSnapshot = true;
        sendAck("SNAPSHOT,QUEUED");
        return;
    }
    if (strcmp(cmd, "DEBUG") == 0) {
        char* value = strtok_r(nullptr, ",", &save);
        debugEnabled = value && atoi(value) != 0;
        sendAck(debugEnabled ? "DEBUG,1" : "DEBUG,0");
        return;
    }
}

void pollCommands() {
    uint8_t budget = 64;
    while (Serial2.available() > 0 && budget-- > 0) {
        const char c = static_cast<char>(Serial2.read());
        if (c == '\r') continue;
        if (c == '\n') {
            rxLine[rxPos] = '\0';
            if (rxPos > 0) handleCommand(rxLine);
            rxPos = 0;
            continue;
        }
        if (rxPos < sizeof(rxLine) - 1) rxLine[rxPos++] = c;
        else rxPos = 0;
    }
}

bool initCamera() {
    camera_config_t c{};
    c.ledc_channel = LEDC_CHANNEL_0;
    c.ledc_timer = LEDC_TIMER_0;
    c.pin_d0 = Y2_GPIO_NUM;
    c.pin_d1 = Y3_GPIO_NUM;
    c.pin_d2 = Y4_GPIO_NUM;
    c.pin_d3 = Y5_GPIO_NUM;
    c.pin_d4 = Y6_GPIO_NUM;
    c.pin_d5 = Y7_GPIO_NUM;
    c.pin_d6 = Y8_GPIO_NUM;
    c.pin_d7 = Y9_GPIO_NUM;
    c.pin_xclk = XCLK_GPIO_NUM;
    c.pin_pclk = PCLK_GPIO_NUM;
    c.pin_vsync = VSYNC_GPIO_NUM;
    c.pin_href = HREF_GPIO_NUM;
    c.pin_sccb_sda = SIOD_GPIO_NUM;
    c.pin_sccb_scl = SIOC_GPIO_NUM;
    c.pin_pwdn = PWDN_GPIO_NUM;
    c.pin_reset = RESET_GPIO_NUM;
    c.xclk_freq_hz = 20000000;
    c.pixel_format = PIXFORMAT_GRAYSCALE;
    c.frame_size = FRAMESIZE_QQVGA;
    c.jpeg_quality = 20;
    c.fb_count = psramFound() ? 2 : 1;
    c.grab_mode = psramFound() ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
    c.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

    const esp_err_t err = esp_camera_init(&c);
    if (err != ESP_OK) {
        Serial.printf("[FISHEYE] camera init failed: 0x%x\n", static_cast<unsigned>(err));
        return false;
    }
    sensor_t* sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_framesize(sensor, FRAMESIZE_QQVGA);
        sensor->set_brightness(sensor, 0);
        sensor->set_contrast(sensor, 0);
        sensor->set_saturation(sensor, 0);
        sensor->set_gain_ctrl(sensor, 1);
        sensor->set_exposure_ctrl(sensor, 1);
        sensor->set_whitebal(sensor, 1);
    }
    return true;
}

void updateFps(uint32_t now) {
    ++frameCounter;
    if (fpsWindowStartMs == 0) fpsWindowStartMs = now;
    const uint32_t elapsed = now - fpsWindowStartMs;
    if (elapsed >= 1000) {
        fpsX10 = static_cast<int>((frameCounter * 10000u) / elapsed);
        frameCounter = 0;
        fpsWindowStartMs = now;
    }
}

void sendVision(uint32_t now, const WallZVisionResult& r) {
    Serial2.printf("V,%lu,%d,%d,%d,%d,%d,%d,%lu\n",
        static_cast<unsigned long>(now), r.motion, r.x, r.y,
        r.brightness, r.contrast, fpsX10, static_cast<unsigned long>(r.flags));
}

void sendGrid(uint32_t now, const WallZVisionResult& r) {
    Serial2.printf("G,%lu,%u,%d,%d,", static_cast<unsigned long>(now),
                   static_cast<unsigned>(++gridSeq), r.brightness, r.contrast);
    static const char hex[] = "0123456789ABCDEF";
    char chunk[64];
    int pos = 0;
    for (int i = 0; i < WALLZ_VISION_GRID_CELLS; ++i) {
        const uint8_t v = currentGrid[i];
        chunk[pos++] = hex[v >> 4];
        chunk[pos++] = hex[v & 0x0F];
        if (pos == static_cast<int>(sizeof(chunk))) {
            Serial2.write(reinterpret_cast<const uint8_t*>(chunk), sizeof(chunk));
            pos = 0;
        }
    }
    if (pos > 0) Serial2.write(reinterpret_cast<const uint8_t*>(chunk), pos);
    Serial2.print('\n');
    lastGridMs = now;
    forceSnapshot = false;
}
}

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial2.begin(kBrainBaud, SERIAL_8N1, kBrainRxPin, kBrainTxPin);
    Serial.printf("[FISHEYE] Wall-Z Fisheye Vision v%s\n", kVersion);
    if (!initCamera()) {
        Serial2.printf("FISHEYE,ERROR,CAMERA,%s\n", kVersion);
        return;
    }
    Serial2.printf("FISHEYE,HELLO,%s\n", kVersion);
    Serial.printf("[FISHEYE] camera ready, PSRAM=%s\n", psramFound() ? "yes" : "no");
}

void loop() {
    pollCommands();
    const uint32_t now = millis();
    const uint32_t periodMs = 1000u / targetFps;
    if (static_cast<uint32_t>(now - lastFrameStartMs) < periodMs) {
        delay(1);
        return;
    }
    lastFrameStartMs = now;

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial2.printf("V,%lu,0,0,0,0,0,%d,8\n", static_cast<unsigned long>(now), fpsX10);
        delay(1);
        return;
    }

    WallZVisionResult result = wallzAnalyzeGrayFrame(
        fb->buf, fb->width, fb->height, previousGrid, havePrevious,
        motionPixelThreshold, currentGrid);
    if (psramFound()) result.flags |= 0x02u;
    result.flags |= 0x04u;
    esp_camera_fb_return(fb);

    updateFps(now);
    sendVision(now, result);
    const uint32_t gridPeriodMs = 1000u / gridFps;
    if (forceSnapshot || static_cast<uint32_t>(now - lastGridMs) >= gridPeriodMs) {
        sendGrid(now, result);
    }

    if (debugEnabled && static_cast<uint32_t>(now - lastDebugMs) >= 1000) {
        lastDebugMs = now;
        Serial.printf("[FISHEYE] fps=%.1f motion=%d x=%d y=%d grid=%uHz\n",
                      fpsX10 / 10.0f, result.motion, result.x, result.y, gridFps);
    }
    delay(1);
}
