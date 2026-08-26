#ifndef WALLZ_BRAIN_CORE_H
#define WALLZ_BRAIN_CORE_H

#include <cmath>
#include <cstdint>
#include <cstring>

#include "brain_protocol.h"

enum class BrainContext : uint8_t {
    Calm = 0,
    Obstacle = 1,
    LightLeft = 2,
    LightRight = 3,
    SoundEvent = 4,
    MotionEvent = 5,
    Count = 6
};

enum class BrainAction : uint8_t {
    Idle = 0,
    Stop = 1,
    LookLeft = 2,
    LookRight = 3,
    CreepForward = 4,
    TurnLeft = 5,
    TurnRight = 6,
    Count = 7
};

struct BrainMetrics {
    float novelty = 0.0f;
    float curiosity = 0.35f;
    float arousal = 0.15f;
    float confidence = 0.25f;
    float valence = 0.0f;
    uint32_t observations = 0;
    uint32_t rewards = 0;
    uint32_t actions = 0;
};

struct BrainPersist {
    uint32_t magic = 0x57425A31u; // "WBZ1"
    uint16_t version = 1;
    uint16_t reserved = 0;
    float q[static_cast<int>(BrainContext::Count)][static_cast<int>(BrainAction::Count)] = {};
    BrainMetrics metrics{};
};

class BrainCore {
public:
    static constexpr float kAlpha = 0.18f;
    static constexpr float kGamma = 0.82f;

    void reset() {
        memset(q_, 0, sizeof(q_));
        metrics_ = BrainMetrics{};
        havePrev_ = false;
        micBaseline_ = 0.0f;
        lastContext_ = BrainContext::Calm;
        lastAction_ = BrainAction::Idle;
        haveAction_ = false;
    }

    void observe(const BrainTelemetry& t) {
        const float micMean = 0.5f * (std::fabs(static_cast<float>(t.mic_l)) + std::fabs(static_cast<float>(t.mic_r)));
        if (metrics_.observations == 0) micBaseline_ = micMean;
        micBaseline_ = 0.985f * micBaseline_ + 0.015f * micMean;

        float noveltyNow = 0.0f;
        if (havePrev_) {
            float dDist = 0.0f;
            if (t.distance_mm >= 0 && prev_.distance_mm >= 0) {
                dDist = clamp01(std::fabs(static_cast<float>(t.distance_mm - prev_.distance_mm)) / 700.0f);
            }
            const float dLight = clamp01(
                (std::fabs(static_cast<float>(t.light_l - prev_.light_l)) +
                 std::fabs(static_cast<float>(t.light_r - prev_.light_r))) / 5000.0f);
            const float dMic = clamp01(
                (std::fabs(static_cast<float>(t.mic_l - prev_.mic_l)) +
                 std::fabs(static_cast<float>(t.mic_r - prev_.mic_r))) / 6000.0f);
            const float gyroMag = std::sqrt(
                static_cast<float>(t.gyro_x_mrad * t.gyro_x_mrad +
                                   t.gyro_y_mrad * t.gyro_y_mrad +
                                   t.gyro_z_mrad * t.gyro_z_mrad));
            const float motion = clamp01(gyroMag / 2500.0f);
            const float visualFamiliarity = (t.vision_online != 0)
                ? clamp01(static_cast<float>(t.vision_familiarity) / 1000.0f) : 0.0f;
            const float visionNovelty = (t.vision_online != 0)
                ? clamp01((static_cast<float>(t.vision_motion) / 1000.0f)
                          * (1.0f - 0.55f * visualFamiliarity)) : 0.0f;
            noveltyNow = clamp01(0.28f*dDist + 0.16f*dLight + 0.20f*dMic
                               + 0.16f*motion + 0.20f*visionNovelty);
        }

        metrics_.novelty = 0.78f * metrics_.novelty + 0.22f * noveltyNow;
        metrics_.curiosity = clamp01(0.88f * metrics_.curiosity + 0.12f * (0.20f + 0.80f * metrics_.novelty));

        const float soundExcitation = clamp01(std::fabs(micMean - micBaseline_) / (std::fabs(micBaseline_) * 0.30f + 100.0f));
        const float gyroAbs = static_cast<float>(std::abs(t.gyro_x_mrad) + std::abs(t.gyro_y_mrad) + std::abs(t.gyro_z_mrad));
        const float motionExcitation = clamp01(gyroAbs / 3500.0f);
        const float visionExcitation = (t.vision_online != 0)
            ? clamp01(static_cast<float>(t.vision_motion) / 250.0f) : 0.0f;
        metrics_.arousal = clamp01(0.86f * metrics_.arousal
            + 0.14f * (0.45f*soundExcitation + 0.30f*motionExcitation + 0.25f*visionExcitation));
        metrics_.valence *= 0.995f;
        metrics_.observations++;
        metrics_.confidence = clamp01(0.25f + 0.65f * (1.0f - std::exp(-static_cast<float>(metrics_.observations) / 1200.0f)));

        if (haveAction_) {
            float intrinsic = 0.03f * metrics_.novelty;
            if (lastAction_ == BrainAction::CreepForward && t.distance_mm >= 0 && t.distance_mm < 280) intrinsic -= 0.65f;
            if (t.manual != 0) intrinsic -= 0.25f; // Human takeover: action was probably undesirable.
            if (std::fabs(intrinsic) > 0.001f) learn(lastContext_, lastAction_, intrinsic, classify(t));
        }

        prev_ = t;
        havePrev_ = true;
    }

    BrainContext classify(const BrainTelemetry& t) const {
        if (t.distance_mm >= 0 && t.distance_mm < 400) return BrainContext::Obstacle;

        // Dedicated fisheye node can raise a visual attention event without
        // taking any motor authority away from the RA4M1 safety layer.
        if (t.vision_online != 0 && t.vision_motion >= 45) return BrainContext::MotionEvent;

        const int lightDiff = t.light_l - t.light_r;
        const int lightScale = (std::abs(t.light_l) + std::abs(t.light_r)) / 2;
        const int lightThreshold = (lightScale / 12 > 80) ? lightScale / 12 : 80;
        if (lightDiff > lightThreshold) return BrainContext::LightLeft;
        if (lightDiff < -lightThreshold) return BrainContext::LightRight;

        const float micMean = 0.5f * (std::fabs(static_cast<float>(t.mic_l)) + std::fabs(static_cast<float>(t.mic_r)));
        if (std::fabs(micMean - micBaseline_) > (std::fabs(micBaseline_) * 0.35f + 120.0f)) return BrainContext::SoundEvent;

        const int gyroAbs = std::abs(t.gyro_x_mrad) + std::abs(t.gyro_y_mrad) + std::abs(t.gyro_z_mrad);
        if (gyroAbs > 1800) return BrainContext::MotionEvent;
        return BrainContext::Calm;
    }

    BrainAction suggest(const BrainTelemetry& t) const {
        if (t.manual || t.robot_mode) return BrainAction::Idle;
        const BrainContext ctx = classify(t);
        if (ctx == BrainContext::Obstacle) return BrainAction::Stop;

        const int ci = static_cast<int>(ctx);
        BrainAction best = BrainAction::Idle;
        float bestQ = q_[ci][0];
        bool anyLearned = std::fabs(bestQ) > 1e-5f;
        for (int a = 1; a < static_cast<int>(BrainAction::Count); ++a) {
            const float qv = q_[ci][a];
            if (std::fabs(qv) > 1e-5f) anyLearned = true;
            if (qv > bestQ) {
                bestQ = qv;
                best = static_cast<BrainAction>(a);
            }
        }
        if (anyLearned) return best;

        switch (ctx) {
            case BrainContext::LightLeft: return BrainAction::LookLeft;
            case BrainContext::LightRight: return BrainAction::LookRight;
            case BrainContext::SoundEvent:
                return (t.mic_l >= t.mic_r) ? BrainAction::LookLeft : BrainAction::LookRight;
            case BrainContext::MotionEvent:
                if (t.vision_online != 0 && t.vision_motion >= 45) {
                    if (t.vision_x < -120) return BrainAction::LookLeft;
                    if (t.vision_x > 120) return BrainAction::LookRight;
                    return BrainAction::Idle;
                }
                return BrainAction::Stop;
            case BrainContext::Calm:
            default:
                return metrics_.curiosity > 0.62f ? BrainAction::CreepForward : BrainAction::Idle;
        }
    }

    void markAction(BrainContext context, BrainAction action) {
        lastContext_ = context;
        lastAction_ = action;
        haveAction_ = action != BrainAction::Idle;
        if (haveAction_) metrics_.actions++;
    }

    void reward(float rewardValue, BrainContext nextContext) {
        if (!haveAction_) return;
        if (rewardValue > 1.0f) rewardValue = 1.0f;
        if (rewardValue < -1.0f) rewardValue = -1.0f;
        learn(lastContext_, lastAction_, rewardValue, nextContext);
        metrics_.valence = clampSigned(0.75f * metrics_.valence + 0.25f * rewardValue);
        metrics_.rewards++;
    }

    const BrainMetrics& metrics() const { return metrics_; }
    float q(BrainContext c, BrainAction a) const { return q_[static_cast<int>(c)][static_cast<int>(a)]; }

    BrainPersist persist() const {
        BrainPersist p;
        memcpy(p.q, q_, sizeof(q_));
        p.metrics = metrics_;
        return p;
    }

    bool restore(const BrainPersist& p) {
        if (p.magic != 0x57425A31u || p.version != 1) return false;
        memcpy(q_, p.q, sizeof(q_));
        metrics_ = p.metrics;
        havePrev_ = false;
        haveAction_ = false;
        micBaseline_ = 0.0f;
        return true;
    }

    static const char* contextName(BrainContext c) {
        switch (c) {
            case BrainContext::Obstacle: return "obstacle";
            case BrainContext::LightLeft: return "light-left";
            case BrainContext::LightRight: return "light-right";
            case BrainContext::SoundEvent: return "sound-event";
            case BrainContext::MotionEvent: return "motion-event";
            case BrainContext::Calm:
            default: return "calm";
        }
    }

    static const char* actionName(BrainAction a) {
        switch (a) {
            case BrainAction::Stop: return "stop";
            case BrainAction::LookLeft: return "look-left";
            case BrainAction::LookRight: return "look-right";
            case BrainAction::CreepForward: return "creep-forward";
            case BrainAction::TurnLeft: return "turn-left";
            case BrainAction::TurnRight: return "turn-right";
            case BrainAction::Idle:
            default: return "idle";
        }
    }

private:
    static float clamp01(float x) {
        if (x < 0.0f) return 0.0f;
        if (x > 1.0f) return 1.0f;
        return x;
    }
    static float clampSigned(float x) {
        if (x < -1.0f) return -1.0f;
        if (x > 1.0f) return 1.0f;
        return x;
    }
    void learn(BrainContext c, BrainAction a, float r, BrainContext next) {
        const int ci = static_cast<int>(c);
        const int ai = static_cast<int>(a);
        const int ni = static_cast<int>(next);
        float maxNext = q_[ni][0];
        for (int j = 1; j < static_cast<int>(BrainAction::Count); ++j) {
            if (q_[ni][j] > maxNext) maxNext = q_[ni][j];
        }
        const float target = r + kGamma * maxNext;
        q_[ci][ai] += kAlpha * (target - q_[ci][ai]);
    }

    float q_[static_cast<int>(BrainContext::Count)][static_cast<int>(BrainAction::Count)] = {};
    BrainMetrics metrics_{};
    BrainTelemetry prev_{};
    bool havePrev_ = false;
    float micBaseline_ = 0.0f;
    BrainContext lastContext_ = BrainContext::Calm;
    BrainAction lastAction_ = BrainAction::Idle;
    bool haveAction_ = false;
};

#endif // WALLZ_BRAIN_CORE_H
