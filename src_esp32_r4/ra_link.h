#ifndef WALLZ_RA_LINK_H
#define WALLZ_RA_LINK_H

#include <cstdint>
#include "brain_protocol.h"

namespace ra_link {
void begin();
void poll(uint32_t now);
void heartbeat(uint32_t now);
bool online(uint32_t now, uint32_t maxAgeMs = 1000);
bool hasTelemetry();
const BrainTelemetry& telemetry();
uint32_t telemetryAge(uint32_t now);
const char* lastAck();

void arm(bool enable);
void stop();
void brake();
void head(int xy, int z);
void move(char direction, uint32_t durationMs);
}

#endif // WALLZ_RA_LINK_H
