// Wall-Z Brain v0.6 shared-control arbitration. Arduino-free for native tests.
#ifndef WALLZ_SHARED_CONTROL_H
#define WALLZ_SHARED_CONTROL_H

enum class WallzControlOwner {
    Stopped = 0,
    Brain,
    Ps4Manual,
    RobotMode,
};

inline WallzControlOwner wallzControlOwner(bool brainArmed, bool manualActive, bool robotModeActive) {
    if (manualActive) return WallzControlOwner::Ps4Manual;
    if (robotModeActive) return WallzControlOwner::RobotMode;
    if (brainArmed) return WallzControlOwner::Brain;
    return WallzControlOwner::Stopped;
}

inline bool wallzBrainMayAct(bool brainArmed, bool manualActive, bool robotModeActive) {
    return wallzControlOwner(brainArmed, manualActive, robotModeActive) == WallzControlOwner::Brain;
}

#endif // WALLZ_SHARED_CONTROL_H
