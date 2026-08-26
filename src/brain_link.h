#ifndef WALLZ_BRAIN_LINK_H
#define WALLZ_BRAIN_LINK_H

#include <cstdint>

namespace brain_link {
void begin();
void poll(uint32_t now);
void tick(uint32_t now);
bool isArmed();
}

#endif // WALLZ_BRAIN_LINK_H
