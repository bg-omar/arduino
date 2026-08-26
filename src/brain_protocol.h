#ifndef WALLZ_BRAIN_PROTOCOL_H
#define WALLZ_BRAIN_PROTOCOL_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

// Wall-Z Brain v0.2 (v0.1-compatible) ASCII protocol over the internal RA4M1 <-> ESP32-S3 UART.
// RA -> S3 telemetry:
// T,ms,distance_mm,light_l,light_r,mic_l,mic_r,gx_mrad,gy_mrad,gz_mrad,head_xy,head_z,manual,robot_mode,brain_armed
//
// S3 -> RA commands:
// B,HB
// B,ARM,0|1
// B,STOP
// B,BRAKE
// B,HEAD,xy,z
// B,MOVE,F|B|L|R,duration_ms

constexpr int BRAIN_FORWARD_CLEARANCE_MM = 350;
constexpr int BRAIN_SONAR_MAX_CM = 500;
constexpr uint32_t BRAIN_SONAR_TIMEOUT_US =
	static_cast<uint32_t>(BRAIN_SONAR_MAX_CM * 58);

// No-echo (distance_mm < 0) means nothing within BRAIN_SONAR_MAX_CM: treat as clear.
inline bool brainForwardIsSafe(int distance_mm, int clearance_mm = BRAIN_FORWARD_CLEARANCE_MM) {
	if (distance_mm < 0) return true;
	return distance_mm >= clearance_mm;
}

struct BrainTelemetry {
    uint32_t ms = 0;
    int distance_mm = -1;
    int light_l = 0;
    int light_r = 0;
    int mic_l = 0;
    int mic_r = 0;
    int gyro_x_mrad = 0;
    int gyro_y_mrad = 0;
    int gyro_z_mrad = 0;
    int head_xy = 90;
    int head_z = 135;
    int manual = 0;
    int robot_mode = 0;
    int brain_armed = 0;

    // Fused by the onboard ESP32-S3 from the dedicated fisheye node.
    // Not part of the RA telemetry line; parseBrainTelemetry() leaves defaults.
    int vision_online = 0;
    int vision_motion = 0;
    int vision_x = 0;
    int vision_y = 0;
    int vision_brightness = 0;
    int vision_contrast = 0;
    int vision_familiarity = 0; // S3-only visual memory similarity 0..1000
    int vision_value = 0;       // S3-only concept association -1000..1000
};

inline bool brainParseInt(const char*& p, long& out) {
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

inline bool parseBrainTelemetry(const char* line, BrainTelemetry& out) {
    if (line == nullptr || line[0] != 'T' || line[1] != ',') return false;
    const char* p = line + 2;
    long v[14] = {};
    for (int i = 0; i < 14; ++i) {
        if (!brainParseInt(p, v[i])) return false;
    }
    out.ms = static_cast<uint32_t>(v[0]);
    out.distance_mm = static_cast<int>(v[1]);
    out.light_l = static_cast<int>(v[2]);
    out.light_r = static_cast<int>(v[3]);
    out.mic_l = static_cast<int>(v[4]);
    out.mic_r = static_cast<int>(v[5]);
    out.gyro_x_mrad = static_cast<int>(v[6]);
    out.gyro_y_mrad = static_cast<int>(v[7]);
    out.gyro_z_mrad = static_cast<int>(v[8]);
    out.head_xy = static_cast<int>(v[9]);
    out.head_z = static_cast<int>(v[10]);
    out.manual = static_cast<int>(v[11]);
    out.robot_mode = static_cast<int>(v[12]);
    out.brain_armed = static_cast<int>(v[13]);
    return true;
}

#endif // WALLZ_BRAIN_PROTOCOL_H
