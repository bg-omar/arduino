#ifndef WALLZ_VISUAL_MEMORY_H
#define WALLZ_VISUAL_MEMORY_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include "vision_grid_protocol.h"

constexpr int WALLZ_VISUAL_MAX_CONCEPTS = 8;
constexpr int WALLZ_VISUAL_LABEL_CAP = 16;
constexpr int WALLZ_VISUAL_RECOGNIZE_THRESHOLD = 820;

struct VisualConcept {
    char label[WALLZ_VISUAL_LABEL_CAP] = {};
    uint16_t samples = 0;
    int16_t value_milli = 0;
    int8_t prototype[WALLZ_SNAPSHOT_GRID_CELLS] = {};
};

struct VisualMemoryPersist {
    uint32_t magic = 0x57565A33u; // "WVZ3"
    uint16_t version = 1;
    uint16_t reserved = 0;
    VisualConcept concepts[WALLZ_VISUAL_MAX_CONCEPTS] = {};
};

struct VisualRecognition {
    int index = -1;
    int score = 0;
    int value_milli = 0;
    const char* label = "unknown";
};

class VisualMemory {
public:
    void reset() {
        for (auto& c : concepts_) c = VisualConcept{};
        current_ = VisualRecognition{};
        haveGrid_ = false;
    }

    static void normalize(const VisionGridSnapshot& s, int8_t out[WALLZ_SNAPSHOT_GRID_CELLS]) {
        long sum = 0;
        int minv = 255;
        int maxv = 0;
        for (int i = 0; i < WALLZ_SNAPSHOT_GRID_CELLS; ++i) {
            const int v = s.grid[i];
            sum += v;
            if (v < minv) minv = v;
            if (v > maxv) maxv = v;
        }
        const int mean = static_cast<int>(sum / WALLZ_SNAPSHOT_GRID_CELLS);
        int scale = maxv - minv;
        if (scale < 32) scale = 32;
        for (int i = 0; i < WALLZ_SNAPSHOT_GRID_CELLS; ++i) {
            int v = (static_cast<int>(s.grid[i]) - mean) * 96 / scale;
            if (v < -127) v = -127;
            if (v > 127) v = 127;
            out[i] = static_cast<int8_t>(v);
        }
    }

    static int similarity(const int8_t a[WALLZ_SNAPSHOT_GRID_CELLS], const int8_t b[WALLZ_SNAPSHOT_GRID_CELLS]) {
        float best = -1.0f;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int64_t dot = 0;
                uint64_t aa = 0;
                uint64_t bb = 0;
                for (int y = 0; y < WALLZ_SNAPSHOT_GRID_H; ++y) {
                    const int yy = y + dy;
                    if (yy < 0 || yy >= WALLZ_SNAPSHOT_GRID_H) continue;
                    for (int x = 0; x < WALLZ_SNAPSHOT_GRID_W; ++x) {
                        const int xx = x + dx;
                        if (xx < 0 || xx >= WALLZ_SNAPSHOT_GRID_W) continue;
                        const int av = static_cast<int>(a[y * WALLZ_SNAPSHOT_GRID_W + x]);
                        const int bv = static_cast<int>(b[yy * WALLZ_SNAPSHOT_GRID_W + xx]);
                        dot += static_cast<int64_t>(av) * bv;
                        aa += static_cast<uint64_t>(av * av);
                        bb += static_cast<uint64_t>(bv * bv);
                    }
                }
                if (aa == 0 || bb == 0) continue;
                const float denom = std::sqrt(static_cast<float>(aa) * static_cast<float>(bb));
                const float corr = static_cast<float>(dot) / denom;
                if (corr > best) best = corr;
            }
        }
        if (best < -1.0f) best = -1.0f;
        if (best > 1.0f) best = 1.0f;
        int score = static_cast<int>((best + 1.0f) * 500.0f + 0.5f);
        if (score < 0) score = 0;
        if (score > 1000) score = 1000;
        return score;
    }

    VisualRecognition observe(const VisionGridSnapshot& s) {
        normalize(s, currentGrid_);
        haveGrid_ = true;
        VisualRecognition best;
        for (int i = 0; i < WALLZ_VISUAL_MAX_CONCEPTS; ++i) {
            if (concepts_[i].samples == 0 || concepts_[i].label[0] == '\0') continue;
            const int score = similarity(currentGrid_, concepts_[i].prototype);
            if (score > best.score) {
                best.index = i;
                best.score = score;
                best.value_milli = concepts_[i].value_milli;
                best.label = concepts_[i].label;
            }
        }
        if (best.score < WALLZ_VISUAL_RECOGNIZE_THRESHOLD) best = VisualRecognition{};
        current_ = best;
        return current_;
    }

    bool teach(const char* label) {
        if (!haveGrid_ || !validLabel(label)) return false;
        int idx = findLabel(label);
        if (idx < 0) idx = findEmpty();
        if (idx < 0) return false;
        VisualConcept& c = concepts_[idx];
        if (c.samples == 0) {
            copyLabel(c.label, label);
            memcpy(c.prototype, currentGrid_, sizeof(c.prototype));
            c.samples = 1;
        } else {
            const int oldWeight = c.samples < 31 ? c.samples : 31;
            for (int i = 0; i < WALLZ_SNAPSHOT_GRID_CELLS; ++i) {
                const int blended = (static_cast<int>(c.prototype[i]) * oldWeight + static_cast<int>(currentGrid_[i])) / (oldWeight + 1);
                c.prototype[i] = static_cast<int8_t>(blended);
            }
            if (c.samples < 65535) ++c.samples;
        }
        current_.index = idx;
        current_.score = 1000;
        current_.value_milli = c.value_milli;
        current_.label = c.label;
        return true;
    }

    bool forget(const char* label) {
        const int idx = findLabel(label);
        if (idx < 0) return false;
        concepts_[idx] = VisualConcept{};
        if (current_.index == idx) current_ = VisualRecognition{};
        return true;
    }

    void rewardCurrent(float reward) {
        if (current_.index < 0 || current_.index >= WALLZ_VISUAL_MAX_CONCEPTS) return;
        if (reward > 1.0f) reward = 1.0f;
        if (reward < -1.0f) reward = -1.0f;
        VisualConcept& c = concepts_[current_.index];
        const int target = static_cast<int>(reward * 1000.0f);
        int next = (c.value_milli * 4 + target) / 5;
        if (next < -1000) next = -1000;
        if (next > 1000) next = 1000;
        c.value_milli = static_cast<int16_t>(next);
        current_.value_milli = c.value_milli;
    }

    int conceptCount() const {
        int n = 0;
        for (const auto& c : concepts_) if (c.samples > 0 && c.label[0]) ++n;
        return n;
    }
    const VisualConcept& conceptAt(int i) const { return concepts_[i]; }
    const VisualRecognition& current() const { return current_; }
    bool haveGrid() const { return haveGrid_; }

    VisualMemoryPersist persist() const {
        VisualMemoryPersist p;
        for (int i = 0; i < WALLZ_VISUAL_MAX_CONCEPTS; ++i) p.concepts[i] = concepts_[i];
        return p;
    }
    bool restore(const VisualMemoryPersist& p) {
        if (p.magic != 0x57565A33u || p.version != 1) return false;
        for (int i = 0; i < WALLZ_VISUAL_MAX_CONCEPTS; ++i) concepts_[i] = p.concepts[i];
        current_ = VisualRecognition{};
        haveGrid_ = false;
        return true;
    }

private:
    static bool validLabel(const char* s) {
        if (!s || !*s) return false;
        int n = 0;
        for (; *s; ++s) {
            ++n;
            if (n >= WALLZ_VISUAL_LABEL_CAP) return false;
            const unsigned char c = static_cast<unsigned char>(*s);
            const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') || c == '-' || c == '_';
            if (!ok) return false;
        }
        return true;
    }
    static void copyLabel(char* dest, const char* src) {
        strncpy(dest, src, WALLZ_VISUAL_LABEL_CAP - 1);
        dest[WALLZ_VISUAL_LABEL_CAP - 1] = '\0';
    }
    int findLabel(const char* label) const {
        for (int i = 0; i < WALLZ_VISUAL_MAX_CONCEPTS; ++i)
            if (concepts_[i].samples && strcmp(concepts_[i].label, label) == 0) return i;
        return -1;
    }
    int findEmpty() const {
        for (int i = 0; i < WALLZ_VISUAL_MAX_CONCEPTS; ++i)
            if (concepts_[i].samples == 0) return i;
        return -1;
    }

    VisualConcept concepts_[WALLZ_VISUAL_MAX_CONCEPTS] = {};
    VisualRecognition current_{};
    int8_t currentGrid_[WALLZ_SNAPSHOT_GRID_CELLS] = {};
    bool haveGrid_ = false;
};

#endif
