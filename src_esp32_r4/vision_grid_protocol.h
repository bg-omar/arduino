#ifndef WALLZ_VISION_GRID_PROTOCOL_H
#define WALLZ_VISION_GRID_PROTOCOL_H

#include <cstdint>
#include <cstdlib>

constexpr int WALLZ_SNAPSHOT_GRID_W = 20;
constexpr int WALLZ_SNAPSHOT_GRID_H = 15;
constexpr int WALLZ_SNAPSHOT_GRID_CELLS = WALLZ_SNAPSHOT_GRID_W * WALLZ_SNAPSHOT_GRID_H;
constexpr int WALLZ_SNAPSHOT_GRID_HEX_CHARS = WALLZ_SNAPSHOT_GRID_CELLS * 2;

// Fisheye -> Brain S3 snapshot protocol:
// G,ms,seq,mean,contrast,<600 hex chars>
struct VisionGridSnapshot {
    uint32_t ms = 0;
    uint16_t seq = 0;
    uint8_t mean = 0;
    uint8_t contrast = 0;
    uint8_t grid[WALLZ_SNAPSHOT_GRID_CELLS] = {};
};

inline int wallzHexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

inline bool parseVisionGridSnapshot(const char* line, VisionGridSnapshot& out) {
    if (!line || line[0] != 'G' || line[1] != ',') return false;
    const char* p = line + 2;
    char* end = nullptr;

    const long ms = strtol(p, &end, 10);
    if (end == p || *end != ',') return false;
    p = end + 1;
    const long seq = strtol(p, &end, 10);
    if (end == p || *end != ',') return false;
    p = end + 1;
    const long mean = strtol(p, &end, 10);
    if (end == p || *end != ',') return false;
    p = end + 1;
    const long contrast = strtol(p, &end, 10);
    if (end == p || *end != ',') return false;
    p = end + 1;

    if (ms < 0 || seq < 0 || seq > 65535 || mean < 0 || mean > 255 || contrast < 0 || contrast > 255) return false;
    for (int i = 0; i < WALLZ_SNAPSHOT_GRID_CELLS; ++i) {
        const int hi = wallzHexNibble(p[2 * i]);
        const int lo = wallzHexNibble(p[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.grid[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    const char tail = p[WALLZ_SNAPSHOT_GRID_HEX_CHARS];
    if (!(tail == '\0' || tail == '\r' || tail == '\n')) return false;

    out.ms = static_cast<uint32_t>(ms);
    out.seq = static_cast<uint16_t>(seq);
    out.mean = static_cast<uint8_t>(mean);
    out.contrast = static_cast<uint8_t>(contrast);
    return true;
}

#endif
