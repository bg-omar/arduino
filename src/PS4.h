//
// Created by mr on 11/13/2023.
//

#ifndef PS4_H
#define PS4_H

#include <cstdint>

struct Ps4ManualSnapshot {
    uint32_t ms = 0;
    int lx = 0;
    int ly = 0;
    int rx = 0;
    int ry = 0;
    bool drive_active = false;
    bool head_active = false;
};

class PS4 {
private:
    static const unsigned int MAX_MESSAGE_LENGTH = 30;
    static int flag;

    int16_t lastY = 0;

public:
    #define DPAD_U   1100
    #define DPAD_R   1200
    #define DPAD_D   1300
    #define DPAD_L   1400
    #define SQUARE   3100
    #define xCROSS   3200
    #define CIRCLE   3300
    #define TRIANG   3400
    #define OPTION   2900
    #define xSHARE   2800
    #define PSHOME   2500
    #define L1       2100
    #define L3       2300
    #define R1       2200
    #define R3       2400
    #define TOUCHPD  2700
    #define MIC      3700
    #define CHARGING 3500
    #define XAUDIO   3600
    #define URIGHT   1500
    #define DRIGHT   1600
    #define D_LEFT   1700
    #define UPLEFT   1800
    static int exitLoop();
    static void joystick(int Xinput, int Yinput);
    static void pollInput();
    static void controller();
    static void pollSerial();
    static void driveWatchdog();
    static bool isManualControlActive();
    static Ps4ManualSnapshot manualSnapshot();

};


#endif //PS4_H
