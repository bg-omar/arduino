//
// Created by mr on 11/13/2023.
//

#include <Arduino.h>
#include "pwm_board.h"
#include "PS4.h"


/***************************************************** Servo PWM Angle s**********************************************/
// section Servo PWM Angle
/***************************************************************************************************************/

Adafruit_PWMServoDriver pwm_board::pwm = Adafruit_PWMServoDriver();

int pwm_board::r = 255;
int pwm_board::g = 0;
int pwm_board::b = 0;
int pwm_board::a = 16;


void pwm_board::setupPWM(){
    pwm_board::pwm.begin();
    pwm_board::pwm.setPWMFreq(FREQUENCY);  // Analog servos run at ~50 Hz updates
    pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(posXY));
    pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(posZ));
}


int pwm_board::pulseWidth(int angle){  //  pwm.setPWM(PWM_0, 0, pulseWidth(0));
    int pulse_wide, analog_value;
    pulse_wide   = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
    analog_value = int(float(pulse_wide) / 1000000 * FREQUENCY * 4096);
    return analog_value;
}

/***************************************************** Servo PWM Angle s**********************************************/
// section RGBled
/***************************************************************************************************************/

void pwm_board::RGBled(int r_val, int g_val, int b_val) {
    pwm.setPWM(PWM_12, 0, (a*b_val<4080) ? a*b_val : 4080);
    pwm.setPWM(PWM_13, 0, (a*g_val<4080) ? a*g_val : 4080);
    pwm.setPWM(PWM_14, 0, (a*r_val<4080) ? a*r_val : 4080);
}

void pwm_board::leftLedStrip(int r_val, int g_val, int b_val) {
    r_val = 255 - r_val;
    g_val = 255 - g_val;
    b_val = 255 - b_val;
    static int lastR = -1;
    static int lastG = -1;
    static int lastB = -1;
    if (r_val == lastR && g_val == lastG && b_val == lastB) {
        return;
    }
    lastR = r_val;
    lastG = g_val;
    lastB = b_val;
    pwm.setPWM(PWM_8, 0, (a*b_val<4080) ? a*b_val : 4080);
    pwm.setPWM(PWM_9, 0, (a*g_val<4080) ? a*g_val : 4080);
    pwm.setPWM(PWM_10, 0, (a*r_val<4080) ? a*r_val : 4080);
}

void pwm_board::rightLedStrip(int r_val, int g_val, int b_val) {
    r_val = 255 - r_val;
    g_val = 255 - g_val;
    b_val = 255 - b_val;
    static int lastR = -1;
    static int lastG = -1;
    static int lastB = -1;
    if (r_val == lastR && g_val == lastG && b_val == lastB) {
        return;
    }
    lastR = r_val;
    lastG = g_val;
    lastB = b_val;
    pwm.setPWM(PWM_5, 0, (a*b_val<4080) ? a*b_val : 4080);
    pwm.setPWM(PWM_6, 0, (a*g_val<4080) ? a*g_val : 4080);
    pwm.setPWM(PWM_7, 0, (a*r_val<4080) ? a*r_val : 4080);
}


void  pwm_board::RainbowColor() {
    if (pwm_board::r > 0 && pwm_board::b == 0) {
        pwm_board::r--;
        pwm_board::g++;
    }
    if (pwm_board::g > 0 && pwm_board::r == 0) {
        pwm_board::g--;
        pwm_board::b++;
    }
    if (pwm_board::b > 0 && pwm_board::g == 0) {
        pwm_board::r++;
        pwm_board::b--;
    }
    pwm_board::rightLedStrip(pwm_board::r,pwm_board::g,pwm_board::b);
    pwm_board::leftLedStrip(pwm_board::r,pwm_board::g,pwm_board::b);
}

void pwm_board::applyEmotionLeds(PestoEmotion e) {
	int lr = 70, lg = 0, lb = 70;
	int rr = 70, rg = 0, rb = 70;
	switch (e) {
		case PestoEmotion::Happy:
			lr = rr = 40; lg = rg = 220; lb = rb = 20;
			break;
		case PestoEmotion::Sad:
			lr = rr = 20; lg = rg = 40; lb = rb = 220;
			break;
		case PestoEmotion::Alert:
			lr = rr = 255; lg = rg = 0; lb = rb = 0;
			break;
		case PestoEmotion::Curious:
			lr = rr = 0; lg = rg = 200; lb = rb = 200;
			break;
		case PestoEmotion::Heart:
			lr = rr = 255; lg = rg = 40; lb = rb = 120;
			break;
		case PestoEmotion::Wink:
			lr = rr = 40; lg = rg = 80; lb = rb = 255;
			break;
		case PestoEmotion::LookLeft:
			lr = 220; lg = 180; lb = 40;
			rr = 40; rg = 40; rb = 40;
			break;
		case PestoEmotion::LookRight:
			lr = 40; lg = 40; lb = 40;
			rr = 220; rg = 180; rb = 40;
			break;
		case PestoEmotion::LookUp:
			lr = rr = 180; lg = rg = 180; lb = rb = 80;
			break;
		case PestoEmotion::LookDown:
			lr = rr = 100; lg = rg = 60; lb = rb = 20;
			break;
		case PestoEmotion::Idle:
		default:
			break;
	}
	leftLedStrip(lr, lg, lb);
	rightLedStrip(rr, rg, rb);
}