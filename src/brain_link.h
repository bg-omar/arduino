#ifndef WALLZ_BRAIN_LINK_H
#define WALLZ_BRAIN_LINK_H

#include <cstdint>

namespace brain_link {
void begin();
void poll(uint32_t now);
void tick(uint32_t now);
bool isArmed();

// v0.6 local PS4/menu authority requests. PS4 activity only pauses Brain; it
// does not disarm. OPTIONS/userStop is the explicit autonomy stop.
void userStop();
void userStartBrain(bool imitation);
void userSelectRobotMode(const char* mode);
}

#endif // WALLZ_BRAIN_LINK_H
