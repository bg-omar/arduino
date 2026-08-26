//
// Created by mr on 10/27/2023.
//

#ifndef FOLLOW_LIGHT_H
#define FOLLOW_LIGHT_H

#include "config.h"


class Follow_light {

public:
    static int lightSensorL, lightSensorR;
    static void light_track();
    static void start();
    static void stop();
    static void tick();
    static bool isActive();
    static double lightSensor();

    static int exitLoop();

private:
    static bool active;
};

#endif //FOLLOW_LIGHT_H
