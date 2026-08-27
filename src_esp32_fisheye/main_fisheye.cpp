/*
 * Wall-Z Fisheye Vision + Episodic SD Node v0.7.0
 * Target: AI-Thinker ESP32-CAM + OV2640 fisheye lens + onboard microSD slot.
 *
 * Architecture:
 *   - fast local perception: 160x120 grayscale -> 20x15 motion grid
 *   - compact V/G telemetry -> onboard ESP32-S3 Brain
 *   - local-first SD storage in 1-bit SD_MMC mode
 *   - PSRAM rolling pre-event buffer + bounded post-event episode recording
 *   - Brain returns semantic context / explicit SAVE/EPISODE requests only
 *   - raw camera frames NEVER traverse the UART or RA4M1
 *
 * SD_MMC 1-bit pins on classic ESP32:
 *   GPIO14 CLK, GPIO15 CMD, GPIO2 D0.
 * This frees GPIO4/12/13. Wall-Z v0.5 uses:
 *   GPIO13 TX -> onboard S3 GPIO41 RX
 *   GPIO4  RX <- onboard S3 GPIO42 TX
 * GPIO4 also drives the AI-Thinker flash LED, so the LED may flicker with RX.
 */
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include "esp_camera.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "vision_grid.h"
#include "camera_storage_policy.h"
#include "wallz_camera_storage_protocol.h"
#include "wallz_episode_protocol.h"
#include "episode_capture_policy.h"

namespace {
constexpr char kVersion[] = "0.7.0";
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

constexpr int kBrainRxPin = 4;   // free in SD_MMC 1-bit mode; shares flash LED
constexpr int kBrainTxPin = 13;  // free in SD_MMC 1-bit mode
constexpr uint32_t kBrainBaud = 230400;
constexpr uint32_t kBrainContextFreshMs = 2200;
constexpr uint32_t kStorageStatusPeriodMs = 10000;
constexpr size_t kEpisodeFrameBytes = 160u * 120u;
constexpr uint8_t kEpisodePreSlots = 25; // max 5 s at 5 fps

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
uint32_t lastStorageStatusMs = 0;
int fpsX10 = 0;
uint16_t gridSeq = 0;
char rxLine[128] = {};
uint8_t rxPos = 0;

CameraStoragePolicy storagePolicy;
EpisodeCapturePolicy episodePolicy;
CameraBrainContext brainContext;
CameraStoreRequest pendingStore;
EpisodeRequest pendingEpisode;
bool haveBrainContext = false;
bool havePendingStore = false;
bool havePendingEpisode = false;
bool sdMounted = false;
uint32_t sdFrames = 0;
uint32_t sdEvents = 0;
uint32_t sdErrors = 0;
uint32_t sdEpisodes = 0;
uint32_t sdSession = 0;
char sessionDir[32] = "/wallz/s0000";
char eventsPath[48] = "/wallz/s0000/events.csv";

uint8_t* preFrames = nullptr;
uint32_t preFrameMs[kEpisodePreSlots] = {};
WallZVisionResult preFrameVision[kEpisodePreSlots] = {};
uint8_t preHead = 0;
uint8_t preCount = 0;

struct ActiveEpisode {
    bool active = false;
    uint32_t id = 0;
    uint32_t trigger_ms = 0;
    uint16_t pre_ms = EpisodeCapturePolicy::kDefaultPreMs;
    uint16_t post_ms = EpisodeCapturePolicy::kDefaultPostMs;
    uint16_t frames = 0;
    uint16_t errors = 0;
    char reason[WALLZ_EPISODE_REASON_CAP] = "event";
    char label[WALLZ_EPISODE_LABEL_CAP] = "unknown";
    char dir[56] = {};
    char manifest[72] = {};
} activeEpisode;


bool ensureDir(const char* path);
bool appendEvent(uint32_t now, uint32_t frameId, const char* reason, const char* label,
                 const WallZVisionResult& r, bool brainFresh, const char* framePath);

bool initEpisodeBuffer() {
    if (preFrames) return true;
    const size_t bytes = kEpisodeFrameBytes * kEpisodePreSlots;
    preFrames = static_cast<uint8_t*>(psramFound() ? ps_malloc(bytes) : malloc(bytes));
    if (!preFrames) {
        Serial.printf("[FISHEYE] episode prebuffer unavailable (%u bytes)\n", static_cast<unsigned>(bytes));
        return false;
    }
    memset(preFrames, 0, bytes);
    return true;
}

void ringPushFrame(const camera_fb_t* fb, uint32_t now, const WallZVisionResult& r) {
    if (!preFrames || !fb || fb->format != PIXFORMAT_GRAYSCALE || fb->width != 160 || fb->height != 120) return;
    memcpy(preFrames + static_cast<size_t>(preHead) * kEpisodeFrameBytes, fb->buf, kEpisodeFrameBytes);
    preFrameMs[preHead] = now;
    preFrameVision[preHead] = r;
    preHead = static_cast<uint8_t>((preHead + 1u) % kEpisodePreSlots);
    if (preCount < kEpisodePreSlots) ++preCount;
}

bool writePgmBytes(const char* path, const uint8_t* data, size_t bytes) {
    if (!sdMounted || !path || !data || bytes < kEpisodeFrameBytes) return false;
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) { ++sdErrors; return false; }
    f.print("P5\n160 120\n255\n");
    const size_t written = f.write(data, kEpisodeFrameBytes);
    f.close();
    if (written != kEpisodeFrameBytes) { ++sdErrors; return false; }
    return true;
}

bool appendEpisodeFrame(const char* phase, uint16_t index, uint32_t frameMs,
                        const WallZVisionResult& r, const char* path) {
    if (!activeEpisode.active) return false;
    File f = SD_MMC.open(activeEpisode.manifest, FILE_APPEND);
    if (!f) { ++sdErrors; ++activeEpisode.errors; return false; }
    const int32_t rel = static_cast<int32_t>(frameMs - activeEpisode.trigger_ms);
    f.printf("%s,%u,%lu,%ld,%d,%d,%d,%d,%d,%s\n",
             phase, static_cast<unsigned>(index), static_cast<unsigned long>(frameMs),
             static_cast<long>(rel), r.motion, r.x, r.y, r.brightness, r.contrast,
             path ? path : "");
    f.close();
    return true;
}

uint16_t availablePreFrames(uint32_t now, uint16_t preMs) {
    if (!preFrames || preCount == 0) return 0;
    uint16_t count = 0;
    const uint8_t oldest = static_cast<uint8_t>((preHead + kEpisodePreSlots - preCount) % kEpisodePreSlots);
    for (uint8_t i = 0; i < preCount; ++i) {
        const uint8_t slot = static_cast<uint8_t>((oldest + i) % kEpisodePreSlots);
        if (static_cast<uint32_t>(now - preFrameMs[slot]) <= preMs) ++count;
    }
    return count;
}

bool flushPreFrames(uint32_t now, uint16_t preMs) {
    if (!preFrames || preCount == 0 || !activeEpisode.active) return true;
    const uint8_t oldest = static_cast<uint8_t>((preHead + kEpisodePreSlots - preCount) % kEpisodePreSlots);
    uint16_t outIndex = 0;
    for (uint8_t i = 0; i < preCount; ++i) {
        const uint8_t slot = static_cast<uint8_t>((oldest + i) % kEpisodePreSlots);
        if (static_cast<uint32_t>(now - preFrameMs[slot]) > preMs) continue;
        char path[88];
        snprintf(path, sizeof(path), "%s/pre_%03u_%010lu.pgm", activeEpisode.dir,
                 static_cast<unsigned>(outIndex), static_cast<unsigned long>(preFrameMs[slot]));
        const uint8_t* data = preFrames + static_cast<size_t>(slot) * kEpisodeFrameBytes;
        if (writePgmBytes(path, data, kEpisodeFrameBytes)) {
            appendEpisodeFrame("pre", outIndex, preFrameMs[slot], preFrameVision[slot], path);
            ++activeEpisode.frames;
            ++sdFrames;
        } else {
            ++activeEpisode.errors;
        }
        ++outIndex;
    }
    return true;
}

void sendEpisodeBegin(uint16_t preFramesWritten, uint16_t postTarget) {
    Serial2.printf("VE,BEGIN,%lu,%lu,%s,%s,%u,%u\n",
                   static_cast<unsigned long>(sdSession),
                   static_cast<unsigned long>(activeEpisode.id),
                   activeEpisode.reason, activeEpisode.label,
                   static_cast<unsigned>(preFramesWritten),
                   static_cast<unsigned>(postTarget));
}

void sendEpisodeDone() {
    Serial2.printf("VE,DONE,%lu,%lu,%s,%s,%u,%u\n",
                   static_cast<unsigned long>(sdSession),
                   static_cast<unsigned long>(activeEpisode.id),
                   activeEpisode.reason, activeEpisode.label,
                   static_cast<unsigned>(activeEpisode.frames),
                   static_cast<unsigned>(activeEpisode.errors));
}

void finishEpisode(uint32_t now) {
    if (!activeEpisode.active) return;
    File f = SD_MMC.open(activeEpisode.manifest, FILE_APPEND);
    if (f) {
        f.printf("end,0,%lu,%ld,0,0,0,0,0,\n",
                 static_cast<unsigned long>(now),
                 static_cast<long>(static_cast<int32_t>(now - activeEpisode.trigger_ms)));
        f.close();
    } else {
        ++sdErrors;
        ++activeEpisode.errors;
    }
    sendEpisodeDone();
    ++sdEpisodes;
    activeEpisode.active = false;
    episodePolicy.finish();
}

bool beginEpisode(const camera_fb_t* fb, uint32_t now, const WallZVisionResult& r,
                  const EpisodeRequest& req) {
    if (!sdMounted || !storagePolicy.enabled()) return false;
    if (activeEpisode.active || !episodePolicy.canTrigger(now)) {
        Serial2.printf("VE,BUSY,%lu\n", static_cast<unsigned long>(activeEpisode.id));
        return false;
    }

    activeEpisode = ActiveEpisode{};
    activeEpisode.active = true;
    activeEpisode.id = sdEpisodes + 1u;
    activeEpisode.trigger_ms = now;
    activeEpisode.pre_ms = req.pre_ms;
    activeEpisode.post_ms = req.post_ms;
    strncpy(activeEpisode.reason, req.reason, sizeof(activeEpisode.reason) - 1);
    strncpy(activeEpisode.label, req.label, sizeof(activeEpisode.label) - 1);
    snprintf(activeEpisode.dir, sizeof(activeEpisode.dir), "%s/ep%06lu", sessionDir,
             static_cast<unsigned long>(activeEpisode.id));
    if (!ensureDir(activeEpisode.dir)) {
        ++sdErrors;
        activeEpisode.active = false;
        Serial2.println("VE,ERROR");
        return false;
    }
    snprintf(activeEpisode.manifest, sizeof(activeEpisode.manifest), "%s/frames.csv", activeEpisode.dir);
    File mf = SD_MMC.open(activeEpisode.manifest, FILE_WRITE);
    if (!mf) {
        ++sdErrors;
        activeEpisode.active = false;
        Serial2.println("VE,ERROR");
        return false;
    }
    const bool brainFresh = haveBrainContext && static_cast<uint32_t>(now - brainContext.received_ms) <= kBrainContextFreshMs;
    mf.println("phase,index,ms,relative_ms,motion,x,y,brightness,contrast,file");
    mf.printf("# reason=%s,label=%s,trigger_ms=%lu,pre_ms=%u,post_ms=%u,familiarity=%d,novelty=%d,value=%d,brain_fresh=%d\n",
              activeEpisode.reason, activeEpisode.label, static_cast<unsigned long>(now),
              static_cast<unsigned>(activeEpisode.pre_ms), static_cast<unsigned>(activeEpisode.post_ms),
              brainFresh ? brainContext.familiarity : 0,
              brainFresh ? brainContext.novelty : 1000,
              brainFresh ? brainContext.value_milli : 0,
              brainFresh ? 1 : 0);
    mf.close();

    const uint16_t preAvailable = availablePreFrames(now, activeEpisode.pre_ms);
    flushPreFrames(now, activeEpisode.pre_ms);

    char eventPath[88];
    snprintf(eventPath, sizeof(eventPath), "%s/event_%010lu.pgm", activeEpisode.dir, static_cast<unsigned long>(now));
    if (fb && fb->format == PIXFORMAT_GRAYSCALE && fb->buf &&
        writePgmBytes(eventPath, fb->buf, fb->len)) {
        appendEpisodeFrame("event", 0, now, r, eventPath);
        ++activeEpisode.frames;
        ++sdFrames;
    } else {
        ++activeEpisode.errors;
    }

    episodePolicy.begin(now, activeEpisode.post_ms);
    const uint16_t postTarget = static_cast<uint16_t>((activeEpisode.post_ms + EpisodeCapturePolicy::kSamplePeriodMs - 1u) /
                                                      EpisodeCapturePolicy::kSamplePeriodMs);
    sendEpisodeBegin(preAvailable, postTarget);
    appendEvent(now, sdFrames, activeEpisode.reason, activeEpisode.label, r, brainFresh, activeEpisode.dir);
    return true;
}

void queueEpisode(const EpisodeRequest& req) {
    pendingEpisode = req;
    havePendingEpisode = true;
}

void sendAck(const char* msg) {
    Serial2.print("VA,");
    Serial2.println(msg);
}

bool ensureDir(const char* path) {
    if (SD_MMC.exists(path)) return true;
    return SD_MMC.mkdir(path);
}

uint32_t nextSessionId() {
    uint32_t value = 0;
    File r = SD_MMC.open("/wallz/session.txt", FILE_READ);
    if (r) {
        String s = r.readStringUntil('\n');
        value = static_cast<uint32_t>(strtoul(s.c_str(), nullptr, 10));
        r.close();
    }
    ++value;
    File w = SD_MMC.open("/wallz/session.txt", FILE_WRITE);
    if (w) {
        w.seek(0);
        w.printf("%lu\n", static_cast<unsigned long>(value));
        w.close();
    }
    return value;
}

bool initStorage() {
    // AI-Thinker onboard slot. true = 1-bit SD_MMC: GPIO14/15/2 only.
    if (!SD_MMC.begin("/sdcard", true)) {
        ++sdErrors;
        sdMounted = false;
        return false;
    }
    if (SD_MMC.cardType() == CARD_NONE) {
        ++sdErrors;
        sdMounted = false;
        return false;
    }
    if (!ensureDir("/wallz")) {
        ++sdErrors;
        sdMounted = false;
        return false;
    }
    sdSession = nextSessionId();
    snprintf(sessionDir, sizeof(sessionDir), "/wallz/s%04lu", static_cast<unsigned long>(sdSession % 10000u));
    if (!ensureDir(sessionDir)) {
        ++sdErrors;
        sdMounted = false;
        return false;
    }
    snprintf(eventsPath, sizeof(eventsPath), "%s/events.csv", sessionDir);
    const bool newEvents = !SD_MMC.exists(eventsPath);
    File e = SD_MMC.open(eventsPath, FILE_APPEND);
    if (!e) {
        ++sdErrors;
        sdMounted = false;
        return false;
    }
    if (newEvents) {
        e.println("ms,frame,reason,label,motion,x,y,brightness,contrast,familiarity,novelty,value,brain_fresh,file");
    }
    e.close();
    sdMounted = true;
    return true;
}

void sendStorageStatus() {
    uint32_t totalMb = 0;
    uint32_t usedMb = 0;
    if (sdMounted) {
        totalMb = static_cast<uint32_t>(SD_MMC.totalBytes() / (1024ULL * 1024ULL));
        usedMb = static_cast<uint32_t>(SD_MMC.usedBytes() / (1024ULL * 1024ULL));
    }
    Serial2.printf("VS,SD,%d,%lu,%lu,%lu,%lu,%lu\n",
                   sdMounted ? 1 : 0,
                   static_cast<unsigned long>(totalMb),
                   static_cast<unsigned long>(usedMb),
                   static_cast<unsigned long>(sdFrames),
                   static_cast<unsigned long>(sdEvents),
                   static_cast<unsigned long>(sdErrors));
}

bool appendEvent(uint32_t now, uint32_t frameId, const char* reason, const char* label,
                 const WallZVisionResult& r, bool brainFresh, const char* framePath) {
    if (!sdMounted) return false;
    File e = SD_MMC.open(eventsPath, FILE_APPEND);
    if (!e) { ++sdErrors; return false; }
    e.printf("%lu,%lu,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\n",
             static_cast<unsigned long>(now), static_cast<unsigned long>(frameId),
             reason ? reason : "unknown", label ? label : "unknown",
             r.motion, r.x, r.y, r.brightness, r.contrast,
             brainFresh ? brainContext.familiarity : 0,
             brainFresh ? brainContext.novelty : 1000,
             brainFresh ? brainContext.value_milli : 0,
             brainFresh ? 1 : 0,
             framePath ? framePath : "");
    e.close();
    ++sdEvents;
    return true;
}

bool savePgmFrame(const camera_fb_t* fb, uint32_t now, const char* reason, const char* label,
                  const WallZVisionResult& r, bool brainFresh) {
    if (!sdMounted || !fb || fb->format != PIXFORMAT_GRAYSCALE || !fb->buf) return false;
    const uint32_t frameId = ++sdFrames;
    char path[72];
    snprintf(path, sizeof(path), "%s/f%06lu_%010lu.pgm", sessionDir,
             static_cast<unsigned long>(frameId), static_cast<unsigned long>(now));
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) { ++sdErrors; --sdFrames; appendEvent(now, frameId, "write_error", label, r, brainFresh, ""); return false; }
    f.printf("P5\n%u %u\n255\n", static_cast<unsigned>(fb->width), static_cast<unsigned>(fb->height));
    const size_t expected = fb->width * fb->height;
    const size_t bytes = fb->len < expected ? fb->len : expected;
    const size_t written = f.write(fb->buf, bytes);
    f.close();
    if (written != bytes) {
        ++sdErrors;
        appendEvent(now, frameId, "short_write", label, r, brainFresh, path);
        return false;
    }
    appendEvent(now, frameId, reason, label, r, brainFresh, path);
    return true;
}

void queueExplicitStore(const CameraStoreRequest& req) {
    pendingStore = req;
    havePendingStore = true;
}

void handleCommand(char* line) {
    if (!line) return;

    CameraBrainContext ctx;
    if (wallzParseBrainContext(line, ctx)) {
        ctx.received_ms = millis();
        brainContext = ctx;
        haveBrainContext = true;
        return;
    }
    CameraStoreRequest req;
    if (wallzParseStoreRequest(line, req)) {
        queueExplicitStore(req);
        sendAck("SAVE,QUEUED");
        return;
    }
    EpisodeRequest ep;
    if (wallzParseEpisodeRequest(line, ep)) {
        queueEpisode(ep);
        sendAck("EP,QUEUED");
        return;
    }

    if (line[0] != 'C' || line[1] != ',') return;
    char* save = nullptr;
    char* cmd = strtok_r(line + 2, ",", &save);
    if (!cmd) return;

    if (strcmp(cmd, "PING") == 0) {
        Serial2.print("FISHEYE,HELLO,");
        Serial2.println(kVersion);
        sendStorageStatus();
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
    if (strcmp(cmd, "SD") == 0) {
        char* value = strtok_r(nullptr, ",", &save);
        if (value) storagePolicy.setEnabled(atoi(value) != 0);
        sendAck(storagePolicy.enabled() ? "SD,1" : "SD,0");
        return;
    }
    if (strcmp(cmd, "SDSTATUS") == 0) {
        sendStorageStatus();
        return;
    }
}

void pollCommands() {
    uint16_t budget = 160;
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

void maybeStoreFrame(const camera_fb_t* fb, uint32_t now, const WallZVisionResult& result) {
    if (!sdMounted || !storagePolicy.enabled()) return;
    const bool brainFresh = haveBrainContext && static_cast<uint32_t>(now - brainContext.received_ms) <= kBrainContextFreshMs;

    // Active episode always gets serviced first. A new trigger may wait one-deep.
    if (activeEpisode.active) {
        if (episodePolicy.sampleDue(now)) {
            const uint16_t index = episodePolicy.postFrames();
            char path[88];
            snprintf(path, sizeof(path), "%s/post_%03u_%010lu.pgm", activeEpisode.dir,
                     static_cast<unsigned>(index), static_cast<unsigned long>(now));
            if (fb && fb->format == PIXFORMAT_GRAYSCALE && fb->buf &&
                writePgmBytes(path, fb->buf, fb->len)) {
                appendEpisodeFrame("post", index, now, result, path);
                ++activeEpisode.frames;
                ++sdFrames;
            } else {
                ++activeEpisode.errors;
            }
            episodePolicy.notePostFrame();
        }
        if (episodePolicy.postComplete(now)) finishEpisode(now);
        if (activeEpisode.active) return;
    }

    // Brain-triggered episode (teach/reward/mode/obstacle/manual) has priority.
    if (havePendingEpisode) {
        const bool started = beginEpisode(fb, now, result, pendingEpisode);
        havePendingEpisode = false;
        if (started) return;
    }

    // Backward-compatible single-frame request.
    if (havePendingStore) {
        if (storagePolicy.acceptExplicit(now)) {
            const char* label = pendingStore.label[0] ? pendingStore.label : (brainFresh ? brainContext.label : "unknown");
            savePgmFrame(fb, now, pendingStore.reason, label, result, brainFresh);
            havePendingStore = false;
            return;
        }
    }

    // Local autonomous event selection becomes an episode trigger in v0.7.
    CameraStoragePolicyInput in;
    in.now_ms = now;
    in.motion = result.motion;
    in.brain_context_fresh = brainFresh;
    in.familiarity = brainFresh ? brainContext.familiarity : 0;
    in.novelty = brainFresh ? brainContext.novelty : 1000;
    const CameraAutoSaveReason autoReason = storagePolicy.evaluate(in);
    if (autoReason != CameraAutoSaveReason::None && episodePolicy.canTrigger(now)) {
        EpisodeRequest ep;
        strncpy(ep.reason, CameraStoragePolicy::name(autoReason), sizeof(ep.reason) - 1);
        strncpy(ep.label, brainFresh ? brainContext.label : "unknown", sizeof(ep.label) - 1);
        ep.pre_ms = EpisodeCapturePolicy::kDefaultPreMs;
        ep.post_ms = EpisodeCapturePolicy::kDefaultPostMs;
        if (beginEpisode(fb, now, result, ep)) return;
    }

    // Maintain rolling visual history only while idle.
    if (episodePolicy.sampleDue(now)) ringPushFrame(fb, now, result);
}
}

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.printf("[FISHEYE] Wall-Z Fisheye Vision + Episodic SD v%s\n", kVersion);

    const bool storageOk = initStorage();
    const bool episodeBufferOk = initEpisodeBuffer();
    Serial.printf("[FISHEYE] SD 1-bit: %s, session=%lu, prebuffer=%s (%u frames @ %u fps)\n",
                  storageOk ? "ready" : "unavailable", static_cast<unsigned long>(sdSession),
                  episodeBufferOk ? "ready" : "unavailable",
                  static_cast<unsigned>(kEpisodePreSlots),
                  static_cast<unsigned>(EpisodeCapturePolicy::kSampleFps));

    // Start Brain UART after SD_MMC has claimed GPIO14/15/2.
    Serial2.setRxBufferSize(512);
    Serial2.setTxBufferSize(1024);
    Serial2.begin(kBrainBaud, SERIAL_8N1, kBrainRxPin, kBrainTxPin);

    if (!initCamera()) {
        Serial2.printf("FISHEYE,ERROR,CAMERA,%s\n", kVersion);
        sendStorageStatus();
        return;
    }
    Serial2.printf("FISHEYE,HELLO,%s\n", kVersion);
    sendStorageStatus();
    Serial.printf("[FISHEYE] camera ready, PSRAM=%s, UART RX=%d TX=%d\n", psramFound() ? "yes" : "no", kBrainRxPin, kBrainTxPin);
}

void loop() {
    pollCommands();
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastStorageStatusMs) >= kStorageStatusPeriodMs) {
        lastStorageStatusMs = now;
        sendStorageStatus();
    }

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
    if (sdMounted) result.flags |= 0x10u;

    updateFps(now);
    sendVision(now, result);
    const uint32_t gridPeriodMs = 1000u / gridFps;
    if (forceSnapshot || static_cast<uint32_t>(now - lastGridMs) >= gridPeriodMs) {
        sendGrid(now, result);
    }

    // Store while the framebuffer is still valid. SD writes are event-driven,
    // never in the hard RA4M1 motor/safety loop.
    maybeStoreFrame(fb, now, result);
    esp_camera_fb_return(fb);

    if (debugEnabled && static_cast<uint32_t>(now - lastDebugMs) >= 1000) {
        lastDebugMs = now;
        Serial.printf("[FISHEYE] fps=%.1f motion=%d x=%d y=%d grid=%uHz sd=%d frames=%lu episodes=%lu ep_active=%d\n",
                      fpsX10 / 10.0f, result.motion, result.x, result.y, gridFps,
                      sdMounted ? 1 : 0, static_cast<unsigned long>(sdFrames),
                      static_cast<unsigned long>(sdEpisodes), activeEpisode.active ? 1 : 0);
    }
    delay(1);
}
