//
// Created by mr on 11/13/2023.
//

#ifndef MOTOR_H
#define MOTOR_H


class Motor {

public:
    static void Car_front() ;
    static void Car_left();
    static void Car_right() ;
    static void Car_Stop() ;
    static void Car_Back();
    static void Car_creepForward();
    static void Car_creepBack();
    static void Car_creepLeft();
    static void Car_creepRight();
    static void Car_avoidForward();
    static void motor_setup();
};

#endif //MOTOR_H
