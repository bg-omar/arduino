#ifndef WALLZ_CAMERA_STORAGE_POLICY_H
#define WALLZ_CAMERA_STORAGE_POLICY_H

#include <cstdint>

enum class CameraAutoSaveReason : uint8_t {
    None = 0,
    MotionOffline,
    UnknownMotion,
    NovelMotion
};

struct CameraStoragePolicyInput {
    uint32_t now_ms = 0;
    int motion = 0;
    bool brain_context_fresh = false;
    int familiarity = 0;
    int novelty = 1000;
};

class CameraStoragePolicy {
public:
    CameraAutoSaveReason evaluate(const CameraStoragePolicyInput& in) {
        if (!enabled_) return CameraAutoSaveReason::None;
        if (static_cast<uint32_t>(in.now_ms - lastAutoSaveMs_) < autoCooldownMs_) return CameraAutoSaveReason::None;
        CameraAutoSaveReason reason = CameraAutoSaveReason::None;
        if (!in.brain_context_fresh && in.motion >= motionThreshold_) {
            reason = CameraAutoSaveReason::MotionOffline;
        } else if (in.brain_context_fresh && in.motion >= unknownMotionThreshold_ && in.familiarity < unknownThreshold_) {
            reason = CameraAutoSaveReason::UnknownMotion;
        } else if (in.brain_context_fresh && in.motion >= novelMotionThreshold_ && in.novelty >= noveltyThreshold_) {
            reason = CameraAutoSaveReason::NovelMotion;
        }
        if (reason != CameraAutoSaveReason::None) lastAutoSaveMs_ = in.now_ms;
        return reason;
    }

    bool acceptExplicit(uint32_t now_ms) {
        if (!enabled_) return false;
        if (static_cast<uint32_t>(now_ms - lastExplicitSaveMs_) < explicitCooldownMs_) return false;
        lastExplicitSaveMs_ = now_ms;
        return true;
    }

    void setEnabled(bool on) { enabled_ = on; }
    bool enabled() const { return enabled_; }
    void setMotionThreshold(int v) { motionThreshold_ = clamp(v, 20, 900); }
    void setUnknownThreshold(int v) { unknownThreshold_ = clamp(v, 100, 990); }
    void setAutoCooldownMs(uint32_t v) { autoCooldownMs_ = v < 500 ? 500 : (v > 60000 ? 60000 : v); }

    static const char* name(CameraAutoSaveReason r) {
        switch (r) {
            case CameraAutoSaveReason::MotionOffline: return "motion_offline";
            case CameraAutoSaveReason::UnknownMotion: return "unknown";
            case CameraAutoSaveReason::NovelMotion: return "novel";
            default: return "none";
        }
    }

private:
    static int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
    bool enabled_ = true;
    int motionThreshold_ = 220;
    int unknownMotionThreshold_ = 90;
    int novelMotionThreshold_ = 90;
    int unknownThreshold_ = 760;
    int noveltyThreshold_ = 560;
    uint32_t autoCooldownMs_ = 5000;
    uint32_t explicitCooldownMs_ = 350;
    uint32_t lastAutoSaveMs_ = 0;
    uint32_t lastExplicitSaveMs_ = 0;
};

#endif
