//
// Created by mr on 11/18/2023.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_DANCING_H
#define ARDUINO_R4_UNO_WALL_Z_DANCING_H

#include "config.h"

class dancing {
public:
    static long  randomXY, randomZ;
    static void dance();
    static void start();
    static void stop();
    static void tick();
    static bool isActive();

    static int exitLoop();

private:
    static bool active;
};


#endif //ARDUINO_R4_UNO_WALL_Z_DANCING_H
