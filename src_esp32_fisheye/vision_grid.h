#ifndef WALLZ_FISHEYE_VISION_GRID_H
#define WALLZ_FISHEYE_VISION_GRID_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

constexpr int WALLZ_VISION_FRAME_W = 160;
constexpr int WALLZ_VISION_FRAME_H = 120;
constexpr int WALLZ_VISION_GRID_W = 20;
constexpr int WALLZ_VISION_GRID_H = 15;
constexpr int WALLZ_VISION_GRID_CELLS = WALLZ_VISION_GRID_W * WALLZ_VISION_GRID_H;
constexpr int WALLZ_VISION_BLOCK_W = WALLZ_VISION_FRAME_W / WALLZ_VISION_GRID_W;
constexpr int WALLZ_VISION_BLOCK_H = WALLZ_VISION_FRAME_H / WALLZ_VISION_GRID_H;

struct WallZVisionResult {
    int motion = 0;      // 0..1000
    int x = 0;           // -1000..1000
    int y = 0;           // -1000..1000
    int brightness = 0;  // 0..255
    int contrast = 0;    // 0..255
    uint32_t flags = 0;
};

inline int wallzVisionClamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline uint8_t wallzVisionSampleBlock(const uint8_t* p, int stride, int gx, int gy) {
    const int x0 = gx * WALLZ_VISION_BLOCK_W;
    const int y0 = gy * WALLZ_VISION_BLOCK_H;
    const int x1 = x0 + WALLZ_VISION_BLOCK_W / 3;
    const int x2 = x0 + (2 * WALLZ_VISION_BLOCK_W) / 3;
    const int y1 = y0 + WALLZ_VISION_BLOCK_H / 3;
    const int y2 = y0 + (2 * WALLZ_VISION_BLOCK_H) / 3;
    const unsigned sum = p[y1 * stride + x1] + p[y1 * stride + x2]
                       + p[y2 * stride + x1] + p[y2 * stride + x2];
    return static_cast<uint8_t>(sum / 4u);
}

inline WallZVisionResult wallzAnalyzeGrayFrame(
    const uint8_t* pixels,
    int width,
    int height,
    uint8_t previous[WALLZ_VISION_GRID_CELLS],
    bool& havePrevious,
    uint8_t motionPixelThreshold,
    uint8_t* currentOut = nullptr) {

    WallZVisionResult r;
    if (!pixels || width != WALLZ_VISION_FRAME_W || height != WALLZ_VISION_FRAME_H) {
        r.flags |= 0x08u;
        return r;
    }

    uint8_t current[WALLZ_VISION_GRID_CELLS];
    uint32_t brightnessSum = 0;
    int minV = 255;
    int maxV = 0;
    uint32_t diffSum = 0;
    uint32_t activeWeight = 0;
    int64_t weightedX = 0;
    int64_t weightedY = 0;

    for (int gy = 0; gy < WALLZ_VISION_GRID_H; ++gy) {
        for (int gx = 0; gx < WALLZ_VISION_GRID_W; ++gx) {
            const int i = gy * WALLZ_VISION_GRID_W + gx;
            const uint8_t v = wallzVisionSampleBlock(pixels, width, gx, gy);
            current[i] = v;
            brightnessSum += v;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;

            if (havePrevious) {
                const int d = std::abs(static_cast<int>(v) - static_cast<int>(previous[i]));
                diffSum += static_cast<uint32_t>(d);
                if (d >= motionPixelThreshold) {
                    const int px = ((2 * gx + 1) * 1000 / WALLZ_VISION_GRID_W) - 1000;
                    const int py = ((2 * gy + 1) * 1000 / WALLZ_VISION_GRID_H) - 1000;
                    const uint32_t w = static_cast<uint32_t>(d - motionPixelThreshold + 1);
                    activeWeight += w;
                    weightedX += static_cast<int64_t>(px) * w;
                    weightedY += static_cast<int64_t>(py) * w;
                }
            }
        }
    }

    if (currentOut) memcpy(currentOut, current, WALLZ_VISION_GRID_CELLS);
    memcpy(previous, current, WALLZ_VISION_GRID_CELLS);
    r.brightness = static_cast<int>(brightnessSum / WALLZ_VISION_GRID_CELLS);
    r.contrast = maxV - minV;

    if (havePrevious) {
        r.motion = wallzVisionClamp(
            static_cast<int>((diffSum * 1000u) / (WALLZ_VISION_GRID_CELLS * 255u)), 0, 1000);
        if (activeWeight > 0) {
            r.x = wallzVisionClamp(static_cast<int>(weightedX / static_cast<int64_t>(activeWeight)), -1000, 1000);
            r.y = wallzVisionClamp(static_cast<int>(weightedY / static_cast<int64_t>(activeWeight)), -1000, 1000);
        }
        if (r.motion >= 45) r.flags |= 0x01u;
    }
    havePrevious = true;
    return r;
}

#endif
