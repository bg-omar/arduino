#ifndef WALLZ_EPISODE_CAPTURE_POLICY_H
#define WALLZ_EPISODE_CAPTURE_POLICY_H

#include <cstdint>

class EpisodeCapturePolicy {
public:
    static constexpr uint16_t kDefaultPreMs = 2000;
    static constexpr uint16_t kDefaultPostMs = 3000;
    static constexpr uint8_t kSampleFps = 5;
    static constexpr uint32_t kSamplePeriodMs = 1000u / kSampleFps;
    static constexpr uint32_t kTriggerCooldownMs = 1200;

    bool sampleDue(uint32_t now) {
        if (!haveSample_ || static_cast<uint32_t>(now - lastSampleMs_) >= kSamplePeriodMs) {
            lastSampleMs_ = now;
            haveSample_ = true;
            return true;
        }
        return false;
    }

    bool canTrigger(uint32_t now) const {
        return !active_ && (!haveTrigger_ || static_cast<uint32_t>(now - lastTriggerMs_) >= kTriggerCooldownMs);
    }

    void begin(uint32_t now, uint16_t postMs) {
        active_ = true;
        haveTrigger_ = true;
        lastTriggerMs_ = now;
        endMs_ = now + postMs;
        postFrames_ = 0;
    }

    void notePostFrame() { if (active_ && postFrames_ < 0xFFFFu) ++postFrames_; }

    bool postComplete(uint32_t now) const {
        return active_ && static_cast<int32_t>(now - endMs_) >= 0;
    }

    void finish() { active_ = false; }
    bool active() const { return active_; }
    uint16_t postFrames() const { return postFrames_; }

private:
    uint32_t lastSampleMs_ = 0;
    uint32_t lastTriggerMs_ = 0;
    uint32_t endMs_ = 0;
    uint16_t postFrames_ = 0;
    bool haveSample_ = false;
    bool haveTrigger_ = false;
    bool active_ = false;
};

#endif
