#ifndef WALLZ_VISION_LINK_H
#define WALLZ_VISION_LINK_H

#include <cstdint>
#include "vision_protocol.h"
#include "vision_grid_protocol.h"
#include "wallz_camera_storage_protocol.h"
#include "wallz_episode_protocol.h"

namespace vision_link {
void begin();
void poll(uint32_t now);

bool online(uint32_t now, uint32_t maxAgeMs = 1000);
bool hasTelemetry();
const VisionTelemetry& telemetry();
uint32_t telemetryAge(uint32_t now);
const char* lastMessage();

bool gridOnline(uint32_t now, uint32_t maxAgeMs = 1500);
bool hasGrid();
const VisionGridSnapshot& grid();
uint32_t gridAge(uint32_t now);

bool hasStorageStatus();
const CameraStorageStatus& storageStatus();
uint32_t storageStatusAge(uint32_t now);

void ping();
void setThreshold(int value);
void setRate(int fps);
void setGridRate(int fps);
void requestSnapshot();
void setDebug(bool on);

// v0.5 semantic feedback: the S3 sends only meaning/context back to the camera.
// Raw frames remain on the fisheye SD and never traverse the Brain/RA path.
void sendContext(const char* label, int familiarity, int novelty, int valueMilli);
void requestStore(const char* reason, const char* label = "unknown");
void requestEpisode(const char* reason, const char* label = "unknown",
                    uint16_t preMs = 2000, uint16_t postMs = 3000);
bool takeEpisodeEvent(EpisodeEvent& out);
void setStorageEnabled(bool on);
void requestStorageStatus();
}

#endif
