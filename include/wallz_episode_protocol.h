#ifndef WALLZ_EPISODE_PROTOCOL_H
#define WALLZ_EPISODE_PROTOCOL_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

constexpr int WALLZ_EPISODE_REASON_CAP = 20;
constexpr int WALLZ_EPISODE_LABEL_CAP = 16;

struct EpisodeRequest {
    char reason[WALLZ_EPISODE_REASON_CAP] = "event";
    char label[WALLZ_EPISODE_LABEL_CAP] = "unknown";
    uint16_t pre_ms = 2000;
    uint16_t post_ms = 3000;
};

struct EpisodeEvent {
    enum class Type : uint8_t { None = 0, Begin, Done, Busy, Error };
    Type type = Type::None;
    uint32_t session = 0;
    uint32_t episode = 0;
    char reason[WALLZ_EPISODE_REASON_CAP] = "event";
    char label[WALLZ_EPISODE_LABEL_CAP] = "unknown";
    uint16_t pre_frames = 0;
    uint16_t post_frames = 0;
    uint16_t total_frames = 0;
    uint16_t errors = 0;
};

inline int wallzEpisodeClamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline bool wallzEpisodeSafeToken(const char* src, char* dst, int cap) {
    if (!src || !dst || cap < 2 || !*src) return false;
    int n = 0;
    for (; *src && n < cap - 1; ++src) {
        const unsigned char c = static_cast<unsigned char>(*src);
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) return false;
        dst[n++] = static_cast<char>(c);
    }
    if (*src != '\0') return false;
    dst[n] = '\0';
    return n > 0;
}

inline bool wallzParseEpisodeRequest(const char* line, EpisodeRequest& out) {
    if (!line || strncmp(line, "C,EP,", 5) != 0) return false;
    char buf[96];
    strncpy(buf, line + 5, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* save = nullptr;
    char* reason = strtok_r(buf, ",", &save);
    char* label = strtok_r(nullptr, ",", &save);
    char* pre = strtok_r(nullptr, ",", &save);
    char* post = strtok_r(nullptr, ",", &save);
    if (!reason || !label) return false;
    EpisodeRequest tmp;
    if (!wallzEpisodeSafeToken(reason, tmp.reason, sizeof(tmp.reason))) return false;
    if (!wallzEpisodeSafeToken(label, tmp.label, sizeof(tmp.label))) return false;
    if (pre) tmp.pre_ms = static_cast<uint16_t>(wallzEpisodeClamp(atoi(pre), 0, 5000));
    if (post) tmp.post_ms = static_cast<uint16_t>(wallzEpisodeClamp(atoi(post), 250, 10000));
    out = tmp;
    return true;
}

inline bool wallzParseEpisodeEvent(const char* line, EpisodeEvent& out) {
    if (!line || strncmp(line, "VE,", 3) != 0) return false;
    char buf[160];
    strncpy(buf, line + 3, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* save = nullptr;
    char* type = strtok_r(buf, ",", &save);
    if (!type) return false;

    EpisodeEvent tmp;
    if (strcmp(type, "BUSY") == 0) {
        tmp.type = EpisodeEvent::Type::Busy;
        char* ep = strtok_r(nullptr, ",", &save);
        if (ep) tmp.episode = static_cast<uint32_t>(strtoul(ep, nullptr, 10));
        out = tmp;
        return true;
    }
    if (strcmp(type, "ERROR") == 0) {
        tmp.type = EpisodeEvent::Type::Error;
        out = tmp;
        return true;
    }

    char* session = strtok_r(nullptr, ",", &save);
    char* episode = strtok_r(nullptr, ",", &save);
    char* reason = strtok_r(nullptr, ",", &save);
    char* label = strtok_r(nullptr, ",", &save);
    if (!session || !episode || !reason || !label) return false;
    tmp.session = static_cast<uint32_t>(strtoul(session, nullptr, 10));
    tmp.episode = static_cast<uint32_t>(strtoul(episode, nullptr, 10));
    if (!wallzEpisodeSafeToken(reason, tmp.reason, sizeof(tmp.reason))) return false;
    if (!wallzEpisodeSafeToken(label, tmp.label, sizeof(tmp.label))) return false;

    if (strcmp(type, "BEGIN") == 0) {
        tmp.type = EpisodeEvent::Type::Begin;
        char* pre = strtok_r(nullptr, ",", &save);
        char* post = strtok_r(nullptr, ",", &save);
        if (pre) tmp.pre_frames = static_cast<uint16_t>(wallzEpisodeClamp(atoi(pre), 0, 1000));
        if (post) tmp.post_frames = static_cast<uint16_t>(wallzEpisodeClamp(atoi(post), 0, 1000));
    } else if (strcmp(type, "DONE") == 0) {
        tmp.type = EpisodeEvent::Type::Done;
        char* total = strtok_r(nullptr, ",", &save);
        char* errors = strtok_r(nullptr, ",", &save);
        if (total) tmp.total_frames = static_cast<uint16_t>(wallzEpisodeClamp(atoi(total), 0, 2000));
        if (errors) tmp.errors = static_cast<uint16_t>(wallzEpisodeClamp(atoi(errors), 0, 65535));
    } else {
        return false;
    }
    out = tmp;
    return true;
}

#endif
