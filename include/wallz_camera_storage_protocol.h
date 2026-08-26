#ifndef WALLZ_CAMERA_STORAGE_PROTOCOL_H
#define WALLZ_CAMERA_STORAGE_PROTOCOL_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

constexpr int WALLZ_CAMERA_LABEL_CAP = 16;
constexpr int WALLZ_CAMERA_REASON_CAP = 16;

struct CameraBrainContext {
    char label[WALLZ_CAMERA_LABEL_CAP] = "unknown";
    int familiarity = 0;   // 0..1000
    int novelty = 1000;    // 0..1000
    int value_milli = 0;   // -1000..1000
    uint32_t received_ms = 0;
};

struct CameraStoreRequest {
    char reason[WALLZ_CAMERA_REASON_CAP] = {};
    char label[WALLZ_CAMERA_LABEL_CAP] = "unknown";
};

struct CameraStorageStatus {
    bool valid = false;
    bool mounted = false;
    uint32_t total_mb = 0;
    uint32_t used_mb = 0;
    uint32_t frames = 0;
    uint32_t events = 0;
    uint32_t errors = 0;
};

inline int wallzClampStorageInt(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline bool wallzSafeToken(const char* src, char* dst, int cap) {
    if (!src || !dst || cap < 2 || !*src) return false;
    int n = 0;
    for (; *src && n < cap - 1; ++src) {
        const unsigned char c = static_cast<unsigned char>(*src);
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) return false;
        dst[n++] = static_cast<char>(c);
    }
    if (*src != '\0') return false;
    dst[n] = '\0';
    return n > 0;
}

inline bool wallzParseBrainContext(const char* line, CameraBrainContext& out) {
    if (!line || strncmp(line, "C,CTX,", 6) != 0) return false;
    char buf[96];
    strncpy(buf, line + 6, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* save = nullptr;
    char* label = strtok_r(buf, ",", &save);
    char* fam = strtok_r(nullptr, ",", &save);
    char* nov = strtok_r(nullptr, ",", &save);
    char* val = strtok_r(nullptr, ",", &save);
    if (!label || !fam || !nov || !val) return false;
    CameraBrainContext tmp;
    if (!wallzSafeToken(label, tmp.label, sizeof(tmp.label))) return false;
    tmp.familiarity = wallzClampStorageInt(atoi(fam), 0, 1000);
    tmp.novelty = wallzClampStorageInt(atoi(nov), 0, 1000);
    tmp.value_milli = wallzClampStorageInt(atoi(val), -1000, 1000);
    out = tmp;
    return true;
}

inline bool wallzParseStoreRequest(const char* line, CameraStoreRequest& out) {
    if (!line || strncmp(line, "C,SAVE,", 7) != 0) return false;
    char buf[64];
    strncpy(buf, line + 7, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* save = nullptr;
    char* reason = strtok_r(buf, ",", &save);
    char* label = strtok_r(nullptr, ",", &save);
    if (!reason) return false;
    CameraStoreRequest tmp;
    if (!wallzSafeToken(reason, tmp.reason, sizeof(tmp.reason))) return false;
    if (label && *label) {
        if (!wallzSafeToken(label, tmp.label, sizeof(tmp.label))) return false;
    }
    out = tmp;
    return true;
}

inline bool wallzParseStorageStatus(const char* line, CameraStorageStatus& out) {
    if (!line || strncmp(line, "VS,SD,", 6) != 0) return false;
    const char* p = line + 6;
    long v[6] = {};
    for (int i = 0; i < 6; ++i) {
        char* end = nullptr;
        v[i] = strtol(p, &end, 10);
        if (end == p) return false;
        if (i < 5) {
            if (*end != ',') return false;
            p = end + 1;
        } else if (*end != '\0' && *end != '\r' && *end != '\n') {
            return false;
        }
    }
    out.valid = true;
    out.mounted = v[0] != 0;
    out.total_mb = static_cast<uint32_t>(v[1] < 0 ? 0 : v[1]);
    out.used_mb = static_cast<uint32_t>(v[2] < 0 ? 0 : v[2]);
    out.frames = static_cast<uint32_t>(v[3] < 0 ? 0 : v[3]);
    out.events = static_cast<uint32_t>(v[4] < 0 ? 0 : v[4]);
    out.errors = static_cast<uint32_t>(v[5] < 0 ? 0 : v[5]);
    return true;
}

#endif
