//
// Created by mr on 11/18/2023.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_AVOID_OBJECTS_H
#define ARDUINO_R4_UNO_WALL_Z_AVOID_OBJECTS_H

#include <cstdint>

#include "config.h"

class avoid_objects {
public:
    static long random2;
    static void avoid();
    static void start();
    static void startHosted();
    static void stop();
    static void tick();
    static bool isActive();
    static double checkDistance();
    static double checkDistanceLong(uint32_t timeoutUs = 29000);

    static double distanceF;
    static double distanceR, distanceL;
    static uint32_t distanceReadMs;

    static bool cachedDistanceFresh(uint32_t now, uint32_t maxAgeMs = 80);

    static int exitLoop();

private:
    static bool active;
};


#endif //ARDUINO_R4_UNO_WALL_Z_AVOID_OBJECTS_H
