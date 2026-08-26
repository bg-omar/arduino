#ifndef WALLZ_VISION_LINK_H
#define WALLZ_VISION_LINK_H

#include <cstdint>
#include "vision_protocol.h"
#include "vision_grid_protocol.h"

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

void ping();
void setThreshold(int value);
void setRate(int fps);
void setGridRate(int fps);
void requestSnapshot();
void setDebug(bool on);
}

#endif
