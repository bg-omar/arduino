#ifndef WALLZ_MANUAL_DEMO_H
#define WALLZ_MANUAL_DEMO_H

#include <cstdint>
#include <cstdlib>

// Wall-Z Brain v0.4 imitation-learning side channel.
// RA4M1 -> onboard ESP32-S3 while PS4 manual control is active:
// D,ms,lx,ly,rx,ry,drive_active,head_active
//
// Drive intent is reconstructed from the applied tank command (sticks are roughly
// -128..127; L2/R2 throttle mixing can reach about +/-255). This line is
// observation-only; it never grants the S3 direct access to PS4 or motors.
struct ManualDemonstration {
    uint32_t ms = 0;
    int lx = 0;
    int ly = 0;
    int rx = 0;
    int ry = 0;
    int drive_active = 0;
    int head_active = 0;
};

inline bool manualDemoParseInt(const char*& p, long& out) {
    if (p == nullptr || *p == '\0') return false;
    char* end = nullptr;
    out = strtol(p, &end, 10);
    if (end == p) return false;
    if (*end == ',') {
        p = end + 1;
        return true;
    }
    if (*end == '\0' || *end == '\r' || *end == '\n') {
        p = end;
        return true;
    }
    return false;
}

inline bool parseManualDemonstration(const char* line, ManualDemonstration& out) {
    if (line == nullptr || line[0] != 'D' || line[1] != ',') return false;
    const char* p = line + 2;
    long v[7] = {};
    for (int i = 0; i < 7; ++i) {
        if (!manualDemoParseInt(p, v[i])) return false;
    }
    out.ms = static_cast<uint32_t>(v[0]);
    out.lx = static_cast<int>(v[1]);
    out.ly = static_cast<int>(v[2]);
    out.rx = static_cast<int>(v[3]);
    out.ry = static_cast<int>(v[4]);
    out.drive_active = static_cast<int>(v[5]);
    out.head_active = static_cast<int>(v[6]);
    return true;
}

#endif // WALLZ_MANUAL_DEMO_H
