//
// Created by mr on 11/13/2023.
//
#include <Arduino.h>
#include "motor.h"
#include "light_drive.h"
#include "main_ra.h"

/********************************************** the function to run motor **************************************/
// section motor function
/***************************************************************************************************************/

void Motor::Car_front(){
    // Serial.println('F');
    digitalWrite(L_ROT,HIGH);
    analogWrite(L_PWM,200);
    digitalWrite(R_ROT,LOW);
    analogWrite(R_PWM,200);
}

void Motor::Car_left(){
    // Serial.println('L');
    digitalWrite(L_ROT,LOW);
    analogWrite(L_PWM,255);
    digitalWrite(R_ROT,LOW);
    analogWrite(R_PWM,255);
}
void Motor::Car_right(){
    //Serial.println('R');
    digitalWrite(L_ROT,HIGH);
    analogWrite(L_PWM,255);
    digitalWrite(R_ROT,HIGH);
    analogWrite(R_PWM,255);
}
void Motor::Car_Stop(){
    //Serial.println("Stop");
    digitalWrite(L_ROT,LOW);
    analogWrite(L_PWM,0);
    digitalWrite(R_ROT,HIGH);
    analogWrite(R_PWM,0);
}

void Motor::Car_Back(){
    //Serial.println('B');
    digitalWrite(L_ROT,LOW);
    analogWrite(L_PWM,200);
    digitalWrite(R_ROT,HIGH);
    analogWrite(R_PWM,200);
}

void Motor::Car_creepForward(){
    digitalWrite(L_ROT,HIGH);
    analogWrite(L_PWM,MOTOR_CREEP_PWM);
    digitalWrite(R_ROT,LOW);
    analogWrite(R_PWM,MOTOR_CREEP_PWM);
}

void Motor::Car_creepBack(){
    digitalWrite(L_ROT,LOW);
    analogWrite(L_PWM,MOTOR_CREEP_PWM);
    digitalWrite(R_ROT,HIGH);
    analogWrite(R_PWM,MOTOR_CREEP_PWM);
}

void Motor::Car_creepLeft(){
    digitalWrite(L_ROT,LOW);
    analogWrite(L_PWM,MOTOR_CREEP_PWM);
    digitalWrite(R_ROT,LOW);
    analogWrite(R_PWM,MOTOR_CREEP_PWM);
}

void Motor::Car_creepRight(){
    digitalWrite(L_ROT,HIGH);
    analogWrite(L_PWM,MOTOR_CREEP_PWM);
    digitalWrite(R_ROT,HIGH);
    analogWrite(R_PWM,MOTOR_CREEP_PWM);
}

void Motor::Car_avoidForward(){
    digitalWrite(L_ROT,HIGH);
    analogWrite(L_PWM,MOTOR_AVOID_PWM);
    digitalWrite(R_ROT,LOW);
    analogWrite(R_PWM,MOTOR_AVOID_PWM);
}

void Motor::motor_setup() {
    pinMode(R_ROT, OUTPUT);     /***** 9 ******/
    pinMode(R_PWM, OUTPUT);      /***** 7 ******/
    pinMode(L_ROT, OUTPUT);     /***** 8 ******/
    pinMode(L_PWM, OUTPUT);      /***** 3 ******/
    digitalWrite(R_ROT, HIGH);
    digitalWrite(L_ROT, HIGH);
//	logger::logln("Motor Pins R: 7, 9,   L: 3, 8  ");
    delay(500);
}
