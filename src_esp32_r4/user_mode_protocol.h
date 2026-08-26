#ifndef WALLZ_USER_MODE_PROTOCOL_H
#define WALLZ_USER_MODE_PROTOCOL_H

#include <cstdint>
#include <cstring>

enum class RaUserModeRequest : uint8_t {
    None = 0,
    Stop,
    Brain,
    Imitation,
    SenseReact,
    FreeRoam,
};

inline RaUserModeRequest parseRaUserModeRequest(const char* line) {
    if (!line || std::strncmp(line, "U,MODE,", 7) != 0) return RaUserModeRequest::None;
    const char* mode = line + 7;
    if (std::strcmp(mode, "BRAIN") == 0) return RaUserModeRequest::Brain;
    if (std::strcmp(mode, "IMITATION") == 0) return RaUserModeRequest::Imitation;
    if (std::strcmp(mode, "SENSE") == 0) return RaUserModeRequest::SenseReact;
    if (std::strcmp(mode, "FREE_ROAM") == 0) return RaUserModeRequest::FreeRoam;
    if (std::strcmp(mode, "STOP") == 0) return RaUserModeRequest::Stop;
    return RaUserModeRequest::Stop;
}

#endif // WALLZ_USER_MODE_PROTOCOL_H
