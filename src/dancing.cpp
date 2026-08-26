//
// Created by mr on 11/18/2023.
//

#include "dancing.h"

#include "Arduino.h"
#include "pwm_board.h"
#include "PS4.h"
#include "deadline.h"


/********************************************** arbitrary sequence *********************************************/
// section Dance
/***************************************************************************************************************/
long  dancing::randomXY, dancing::randomZ;
bool dancing::active = false;

static int dancePose = 0;
static Deadline danceDeadline;

void dancing::start() {
	active = true;
	dancePose = 0;
	danceDeadline.after(millis(), 0);
}

void dancing::stop() {
	active = false;
}

bool dancing::isActive() {
	return active;
}

void dancing::tick() {
	if (!active) {
		return;
	}
	if (PS4::exitLoop()) {
		stop();
		return;
	}

	const uint32_t now = millis();
	if (!danceDeadline.isDue(now)) {
		return;
	}

	pwm_board::RainbowColor();
	if (dancePose % 2 == 0) {
		randomXY = random(1, 180);
		pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(randomXY));
	} else {
		randomZ = random(1, 160);
		pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(randomZ));
	}
	dancePose = (dancePose + 1) % 6;
	danceDeadline.after(now, 500);
}

void dancing::dance() {
	start();
}
