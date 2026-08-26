//
// Created by mr on 11/13/2023.
//

#include "timers.h"
#include "config.h"
#include "pesto_matrix.h"
#include "compass.h"
#include "gyroscope.h"
#include "displayAdafruit.h"

bool timers::timerTwoActive = false;
bool timers::timerTreeActive = false;
int timers::timerButton = 0;
TimerEvent timers::timerOne;
TimerEvent timers::timerTwo;
TimerEvent timers::timerThree;
TimerEvent timers::timerMouth;


/***************************************************** Functions s**********************************************/

// section Timer Functions

/***************************************************************************************************************/

void timers::initTimers() {
    timers::timerOne.set(timers::timerOnePeriod, timers::dotMatrixTimer);
    timers::timerTwo.set(timers::timerTwoPeriod, timers::sensorTimer);
    timers::timerThree.set(timers::timerThreePeriod, timers::resetTimers);
    timers::timerMouth.set(timers::timerMouthPeriod, timers::mouthTimer);
}

void timers::update(){
    timers::timerOne.update();
    timers::timerTwo.update();
    timers::timerThree.update();
    timers::timerMouth.update();
}
void timers::dotMatrixTimer(){
        Pesto::pestoMatrix();
}

void timers::sensorTimer(){
    #if USE_COMPASS
        if (timerTwoActive && timerButton == L1){
            compass::showCompass();
        }
    #endif

    #if USE_GYRO
        if (timerTwoActive && timerButton == R1){
            gyroscope::gyroFunc();
        }
    #endif
}

void timers::resetTimers(){
    timers::timerTwoActive = false;
    timers::timerTreeActive = false;
}

void timers::mouthTimer(){
    #if DISPLAY_DEMO
        #if USE_ADAFRUIT
             displayAdafruit::displayLoop();
        #endif;
    #endif
}

