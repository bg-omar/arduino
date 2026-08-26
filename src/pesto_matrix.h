//
// Created by mr on 10/27/2023.
//

#ifndef PESTO_MATRIX_H
#define PESTO_MATRIX_H

#include <Arduino.h>
#include <cstdint>

#include "pesto_emotion.h"

    /********************************************** the function for dot matrix display ****************************/
    // section Pesto Matrix
    /***************************************************************************************************************/
    class Pesto {
    private:
        static int screen;
        static PestoEmotion heldEmotion;
        static uint32_t holdUntilMs;

        static const unsigned char* bitmapFor(PestoEmotion e);
        static bool holdActive(uint32_t nowMs);
        static void displayEmotion(PestoEmotion e);

    public:
        /********************************************** Make DotMatric Images*******************************************/
        // section DotMatrix Images
        /***************************************************************************************************************/

        // Array, used to store the data of the pattern
        static unsigned char STOP01;
        static unsigned char hou;
        static unsigned char op;
        static unsigned char met;
        static unsigned char pesto;
        static unsigned char bleh;

        static unsigned char north;
        static unsigned char east;
        static unsigned char south;
        static unsigned char west;

        static unsigned char front;
        static unsigned char back;
        static unsigned char left;
        static unsigned char right;

        static unsigned char clear;

        constexpr static const byte Heart[8] = { 0b00000, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000};

        static void IIC_start();
        static void IIC_send(unsigned char send_data);
        static void IIC_end();
        static void matrix_display(unsigned char matrix_value[]);
        static void showEmotion(PestoEmotion e, uint32_t nowMs,
                                uint32_t holdMs = PESTO_EMOTION_HOLD_MS);
        static void displayBitmap(const uint8_t colMajor16[16]);
        static void syncFromOledFrame(const uint8_t* oled1024);
        static void pestoMatrix();
        static void setup_pestoMatrix();
    };






#endif //PESTO_MATRIX_H
