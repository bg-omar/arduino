#ifndef WALLZ_IMITATION_MEMORY_H
#define WALLZ_IMITATION_MEMORY_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "brain_protocol.h"
#include "manual_demo.h"

constexpr int WALLZ_IMITATION_FEATURES = 8;
constexpr int WALLZ_IMITATION_MAX_SAMPLES = 96;
constexpr int WALLZ_IMITATION_K = 5;
constexpr int WALLZ_IMITATION_EXEC_THRESHOLD = 680; // confidence 0..1000
constexpr int WALLZ_IMITATION_DEDUP_DISTANCE = 80;

enum class ImitationAction : uint8_t {
    Idle = 0,
    Forward = 1,
    Backward = 2,
    TurnLeft = 3,
    TurnRight = 4,
    LookLeft = 5,
    LookRight = 6,
    LookUp = 7,
    LookDown = 8,
    Count = 9
};

struct ImitationFeatures {
    int8_t v[WALLZ_IMITATION_FEATURES] = {};
    uint16_t concept_tag = 0; // stable hash of recognized label; 0 = unknown
};

struct ImitationSample {
    ImitationFeatures features{};
    uint8_t action = static_cast<uint8_t>(ImitationAction::Idle);
    uint8_t reserved = 0;
};

struct ImitationPrediction {
    ImitationAction action = ImitationAction::Idle;
    int confidence = 0; // 0..1000
    int nearest_distance = 0;
    int neighbors = 0;
};

struct ImitationPersist {
    uint32_t magic = 0x57494D34u; // "WIM4"
    uint16_t version = 1;
    uint16_t count = 0;
    uint16_t next = 0;
    uint16_t reserved = 0;
    uint32_t accepted = 0;
    uint32_t rejected_duplicate = 0;
    ImitationSample samples[WALLZ_IMITATION_MAX_SAMPLES] = {};
};

class ImitationMemory {
public:
    void reset() {
        count_ = 0;
        next_ = 0;
        accepted_ = 0;
        rejectedDuplicate_ = 0;
        for (auto& sample : samples_) sample = ImitationSample{};
    }

    static ImitationAction actionFromManual(const ManualDemonstration& d) {
        constexpr int kDriveDeadzone = 20;
        constexpr int kHeadDeadzone = 20;
        if (d.drive_active != 0) {
            const int ax = std::abs(d.lx);
            const int ay = std::abs(d.ly);
            if (ax < kDriveDeadzone && ay < kDriveDeadzone) return ImitationAction::Idle;
            // Prefer forward/back when Y is the dominant intent. Diagonal driving
            // remains represented as the stronger discrete component in v0.4.
            if (ay * 10 >= ax * 7) {
                return d.ly >= 0 ? ImitationAction::Forward : ImitationAction::Backward;
            }
            return d.lx >= 0 ? ImitationAction::TurnRight : ImitationAction::TurnLeft;
        }
        if (d.head_active != 0) {
            const int ax = std::abs(d.rx);
            const int ay = std::abs(d.ry);
            if (ax < kHeadDeadzone && ay < kHeadDeadzone) return ImitationAction::Idle;
            if (ax >= ay) return d.rx < 0 ? ImitationAction::LookLeft : ImitationAction::LookRight;
            // R-stick Y is inverted by ps4HeadTargetZ(): negative is look-up.
            return d.ry < 0 ? ImitationAction::LookUp : ImitationAction::LookDown;
        }
        return ImitationAction::Idle;
    }

    static ImitationFeatures featuresFrom(const BrainTelemetry& t, uint16_t conceptTag) {
        ImitationFeatures f;
        // Distance encodes 0..5000 mm approximately as -127..127. No echo within
        // the configured 5 m range is treated as maximum-clear distance.
        const int dist = t.distance_mm < 0 ? 5000 : clampInt(t.distance_mm, 0, 5000);
        f.v[0] = qSigned((dist - 2500), 2500);
        f.v[1] = qBalance(t.light_l, t.light_r);
        f.v[2] = qBalance(t.mic_l, t.mic_r);
        const int gyro = clampInt(std::abs(t.gyro_x_mrad) + std::abs(t.gyro_y_mrad) + std::abs(t.gyro_z_mrad), 0, 4500);
        f.v[3] = qUnsigned(gyro, 4500);
        f.v[4] = qSigned(t.head_xy - 90, 80);
        f.v[5] = qSigned(t.vision_online ? t.vision_x : 0, 1000);
        f.v[6] = qUnsigned(t.vision_online ? t.vision_motion : 0, 1000);
        f.v[7] = qUnsigned(t.vision_online ? t.vision_familiarity : 0, 1000);
        f.concept_tag = conceptTag;
        return f;
    }

    bool learn(const ManualDemonstration& d, const BrainTelemetry& t, uint16_t conceptTag) {
        const ImitationAction action = actionFromManual(d);
        if (action == ImitationAction::Idle) return false;
        const ImitationFeatures f = featuresFrom(t, conceptTag);

        // Avoid filling the fixed memory with dozens of almost-identical samples
        // while a stick is held in one position.
        int nearestSame = 100000;
        for (int i = 0; i < count_; ++i) {
            if (samples_[i].action != static_cast<uint8_t>(action)) continue;
            const int d0 = featureDistance(f, samples_[i].features);
            if (d0 < nearestSame) nearestSame = d0;
        }
        if (nearestSame < WALLZ_IMITATION_DEDUP_DISTANCE) {
            ++rejectedDuplicate_;
            return false;
        }

        ImitationSample s;
        s.features = f;
        s.action = static_cast<uint8_t>(action);
        samples_[next_] = s;
        next_ = static_cast<uint16_t>((next_ + 1) % WALLZ_IMITATION_MAX_SAMPLES);
        if (count_ < WALLZ_IMITATION_MAX_SAMPLES) ++count_;
        ++accepted_;
        return true;
    }

    ImitationPrediction predict(const BrainTelemetry& t, uint16_t conceptTag) const {
        ImitationPrediction out;
        if (count_ <= 0) return out;
        const ImitationFeatures query = featuresFrom(t, conceptTag);

        struct Neighbor { int distance; uint8_t action; };
        Neighbor best[WALLZ_IMITATION_K];
        for (int k = 0; k < WALLZ_IMITATION_K; ++k) best[k] = Neighbor{1000000, 0};

        for (int i = 0; i < count_; ++i) {
            const int d = featureDistance(query, samples_[i].features);
            for (int k = 0; k < WALLZ_IMITATION_K; ++k) {
                if (d < best[k].distance) {
                    for (int m = WALLZ_IMITATION_K - 1; m > k; --m) best[m] = best[m - 1];
                    best[k] = Neighbor{d, samples_[i].action};
                    break;
                }
            }
        }

        int votes[static_cast<int>(ImitationAction::Count)] = {};
        int totalVote = 0;
        int used = 0;
        for (int k = 0; k < WALLZ_IMITATION_K && k < count_; ++k) {
            if (best[k].distance >= 1000000) continue;
            const int weight = clampInt(1400 - best[k].distance, 1, 1400);
            const int ai = best[k].action < static_cast<uint8_t>(ImitationAction::Count) ? best[k].action : 0;
            votes[ai] += weight;
            totalVote += weight;
            ++used;
        }
        if (used == 0 || totalVote <= 0) return out;

        int bestAction = 0;
        int bestVote = votes[0];
        for (int a = 1; a < static_cast<int>(ImitationAction::Count); ++a) {
            if (votes[a] > bestVote) { bestVote = votes[a]; bestAction = a; }
        }

        const int nearest = best[0].distance;
        const int similarity = clampInt(1000 - (nearest * 1000) / 3600, 0, 1000);
        const int consensus = (bestVote * 1000) / totalVote;
        const int sampleFactor = clampInt((count_ * 1000) / 3, 0, 1000); // require ~3 examples for high confidence
        out.action = static_cast<ImitationAction>(bestAction);
        out.confidence = (similarity * consensus / 1000) * sampleFactor / 1000;
        out.nearest_distance = nearest;
        out.neighbors = used;
        return out;
    }

    int count() const { return count_; }
    uint32_t accepted() const { return accepted_; }
    uint32_t rejectedDuplicate() const { return rejectedDuplicate_; }
    const ImitationSample& sampleAt(int i) const { return samples_[i]; }

    ImitationPersist persist() const {
        ImitationPersist p;
        p.count = static_cast<uint16_t>(count_);
        p.next = next_;
        p.accepted = accepted_;
        p.rejected_duplicate = rejectedDuplicate_;
        memcpy(p.samples, samples_, sizeof(samples_));
        return p;
    }

    bool restore(const ImitationPersist& p) {
        if (p.magic != 0x57494D34u || p.version != 1) return false;
        if (p.count > WALLZ_IMITATION_MAX_SAMPLES || p.next >= WALLZ_IMITATION_MAX_SAMPLES) return false;
        count_ = p.count;
        next_ = p.next;
        accepted_ = p.accepted;
        rejectedDuplicate_ = p.rejected_duplicate;
        memcpy(samples_, p.samples, sizeof(samples_));
        return true;
    }

    static uint16_t tagFromLabel(const char* label) {
        if (label == nullptr || label[0] == '\0') return 0;
        uint32_t h = 2166136261u;
        for (const unsigned char* p = reinterpret_cast<const unsigned char*>(label); *p; ++p) {
            h ^= static_cast<uint32_t>(*p);
            h *= 16777619u;
        }
        uint16_t tag = static_cast<uint16_t>((h ^ (h >> 16)) & 0xFFFFu);
        return tag == 0 ? 1 : tag;
    }

    static const char* actionName(ImitationAction a) {
        switch (a) {
            case ImitationAction::Forward: return "forward";
            case ImitationAction::Backward: return "backward";
            case ImitationAction::TurnLeft: return "turn-left";
            case ImitationAction::TurnRight: return "turn-right";
            case ImitationAction::LookLeft: return "look-left";
            case ImitationAction::LookRight: return "look-right";
            case ImitationAction::LookUp: return "look-up";
            case ImitationAction::LookDown: return "look-down";
            case ImitationAction::Idle:
            default: return "idle";
        }
    }

private:
    static int clampInt(int v, int lo, int hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
    static int8_t qSigned(int v, int scale) {
        if (scale <= 0) return 0;
        const int q = clampInt((v * 127) / scale, -127, 127);
        return static_cast<int8_t>(q);
    }
    static int8_t qUnsigned(int v, int maxv) {
        if (maxv <= 0) return 0;
        const int q = clampInt((v * 127) / maxv, 0, 127);
        return static_cast<int8_t>(q);
    }
    static int8_t qBalance(int a, int b) {
        const int denom = std::abs(a) + std::abs(b) + 1;
        return static_cast<int8_t>(clampInt(((a - b) * 127) / denom, -127, 127));
    }
    static int featureDistance(const ImitationFeatures& a, const ImitationFeatures& b) {
        static constexpr int w[WALLZ_IMITATION_FEATURES] = {2, 2, 1, 1, 1, 2, 2, 2};
        int d = 0;
        for (int i = 0; i < WALLZ_IMITATION_FEATURES; ++i) {
            d += w[i] * std::abs(static_cast<int>(a.v[i]) - static_cast<int>(b.v[i]));
        }
        if (a.concept_tag != 0 && b.concept_tag != 0) {
            if (a.concept_tag != b.concept_tag) d += 700;
            else d = d > 80 ? d - 80 : 0;
        } else if ((a.concept_tag != 0) != (b.concept_tag != 0)) {
            d += 120;
        }
        return d;
    }

    ImitationSample samples_[WALLZ_IMITATION_MAX_SAMPLES] = {};
    int count_ = 0;
    uint16_t next_ = 0;
    uint32_t accepted_ = 0;
    uint32_t rejectedDuplicate_ = 0;
};

#endif // WALLZ_IMITATION_MEMORY_H
