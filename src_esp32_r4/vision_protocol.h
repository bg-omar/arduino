#ifndef WALLZ_VISION_PROTOCOL_H
#define WALLZ_VISION_PROTOCOL_H

#include <cstdint>
#include <cstdlib>

// Wall-Z Fisheye Vision v0.1 ASCII protocol.
// Fisheye -> Brain S3:
//   V,ms,motion,x,y,brightness,contrast,fps_x10,flags
// where motion=0..1000, x/y=-1000..1000, brightness/contrast=0..255.
//
// Brain S3 -> Fisheye (optional full-duplex control):
//   C,PING
//   C,THR,<0..255>
//   C,RATE,<5..40>
//   C,DEBUG,<0|1>

struct VisionTelemetry {
    uint32_t ms = 0;
    int motion = 0;
    int x = 0;
    int y = 0;
    int brightness = 0;
    int contrast = 0;
    int fps_x10 = 0;
    uint32_t flags = 0;
};

inline bool visionParseLong(const char*& p, long& out) {
    if (!p || *p == '\0') return false;
    char* end = nullptr;
    out = strtol(p, &end, 10);
    if (end == p) return false;
    if (*end == ',') { p = end + 1; return true; }
    if (*end == '\0' || *end == '\r' || *end == '\n') { p = end; return true; }
    return false;
}

inline bool parseVisionTelemetry(const char* line, VisionTelemetry& out) {
    if (!line || line[0] != 'V' || line[1] != ',') return false;
    const char* p = line + 2;
    long v[8] = {};
    for (int i = 0; i < 8; ++i) {
        if (!visionParseLong(p, v[i])) return false;
    }
    out.ms = static_cast<uint32_t>(v[0]);
    out.motion = static_cast<int>(v[1]);
    out.x = static_cast<int>(v[2]);
    out.y = static_cast<int>(v[3]);
    out.brightness = static_cast<int>(v[4]);
    out.contrast = static_cast<int>(v[5]);
    out.fps_x10 = static_cast<int>(v[6]);
    out.flags = static_cast<uint32_t>(v[7]);
    return true;
}

#endif
