//
// Created by mr on 11/13/2023.
//

#ifndef PWM_BOARD_H
#define PWM_BOARD_H

#include <Adafruit_PWMServoDriver.h>
#include "pesto_emotion.h"

#define PWM_0        0
#define PWM_1        1
#define PWM_2        2
#define PWM_3        3
#define PWM_4        4
#define PWM_5        5
#define PWM_6        6
#define PWM_7        7
#define PWM_8        8
#define PWM_9        9
#define PWM_10      10
#define PWM_11      11
#define PWM_12      12
#define PWM_13      13
#define PWM_14      14
#define PWM_15      15
#define PWM_16      16

#define FREQUENCY             50
#define MIN_PULSE_WIDTH       650
#define MAX_PULSE_WIDTH       2350



class pwm_board {
public:
    uint8_t servonum = 0;
    static void setupPWM();
    static Adafruit_PWMServoDriver pwm;
    static int pulseWidth(int angle);
    static void RGBled(int r_val, int g_val, int b_val);
    static void RainbowColor();

    static void leftLedStrip(int r_val, int g_val, int b_val);

    static void rightLedStrip(int r_val, int g_val, int b_val);
    static void applyEmotionLeds(PestoEmotion e);

    static int posXY;
    static int posZ;
    static int a;
private:
    static int r;
    static int g;
    static int b;

};


#endif //PWM_BOARD_H
