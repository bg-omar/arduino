//
// Created by mr on 11/13/2023.
//
// PS4 wire protocol (ESP32 CAM/PS4 host → Serial1 → this RA sketch)
// ---------------------------------------------------------------------------
// Lines are ASCII ints, optional "A+B" pairs (see ps4_parse.h).
//
// Button bases (xxxx) — held/poll in BUTTONS mode uses the base value:
//   1xxx  D-pad (numpad layout)     1100 Up  1200 Right  1300 Down  1400 Left
//         diagonals (optional)      1500 UR  1600 DR     1700 DL    1800 UL
//   2xxx  shoulders / sticks / sys  2100 L1  2200 R1     2300 L3    2400 R3
//                                   2500 PS  2700 Touch  2800 Share 2900 Options
//   3xxx  face + status             3100 Square  3200 Cross  3300 Circle  3400 Triangle
//                                   3500 Charging  3600 Audio  3700 Mic
//                                   3900+n Battery (n = level)
//
// Event suffixes (EVENTS mode on ESP): xx10 = press/down, xx01 = release/up
//   e.g. 1110 D-pad Up down, 1101 D-pad Up up; 2110 L1 down, 2101 L1 up.
//
// Analog ranges (base + signed/unsigned stick or trigger value):
//   4xxx  L2 trigger     4000..4255   (4000 + 0..255)
//   5xxx  R2 trigger     5000..5255
//   6xxx  Left stick X   6000..6255   center ~6127
//   7xxx  Left stick Y   7000..7255   center ~7127   → drive (ps4_drive.h)
//   8xxx  Right stick X  8000..8255   center ~8127
//   9xxx  Right stick Y  9000..9255   center ~9127   → head  (ps4_head.h)
// Dual axis: "6127+7127" or "4000+5000" (see main_ps4.cpp).
//

#include "PS4.h"

#include "Arduino.h"
#include "motor.h"
#include "pwm_board.h"
#include "timers.h"

#include "main_ra.h"
#include "compass.h"
#include "barometer.h"
#include "menu.h"
#include "displayAdafruit.h"
#include "deadline.h"
#include "logger.h"
#include "ps4_parse.h"
#include "ps4_drive.h"
#include "ps4_head.h"
#include "pesto_emotion.h"
#include "pesto_matrix.h"
#include "robot_modes.h"
#include "sense_react.h"
#include "laser_beam.h"
#include "radar_scan.h"

static uint32_t lastButtonMs = 0;
static uint32_t lastDriveMs = 0;
static uint32_t lastHeadMs = 0;
static bool stickDriving = false;
static int lastLX = 0;
static int lastLY = 0;
static int lastRX = 0;
static int lastRY = 0;
static int headTargetXY = 90;
static int headTargetZ = 135;

static PestoEmotion lastDotEmotion = PestoEmotion::Idle;
static uint32_t lastDotEmotionMs = 0;

static void showDotEmotion(PestoEmotion e) {
	if (e == PestoEmotion::Idle) {
		return;
	}
	if (!FEATURE_ENABLED(main::use_dot, USE_DOT)) {
		return;
	}
	const uint32_t now = millis();
	if (e == lastDotEmotion && !elapsed(now, lastDotEmotionMs, 100)) {
		return;
	}
	lastDotEmotion = e;
	lastDotEmotionMs = now;
	Pesto::showEmotion(e, now);
}

static void applyTankOutput(const Ps4TankOutput& tank) {
	if (tank.stop) {
		Motor::Car_Stop();
		stickDriving = false;
		lastLX = 0;
		lastLY = 0;
		return;
	}

	int left = tank.leftVel;
	int right = tank.rightVel;
	if (left < 0) {
		digitalWrite(L_ROT, HIGH);
	} else {
		digitalWrite(L_ROT, LOW);
	}
	if (right < 0) {
		digitalWrite(R_ROT, LOW);
	} else {
		digitalWrite(R_ROT, HIGH);
	}

	analogWrite(L_PWM, ps4PwmFromVel(left));
	analogWrite(R_PWM, ps4PwmFromVel(right));
	stickDriving = true;
	lastDriveMs = millis();
}

int PS4::exitLoop() {
    if (Serial1.available()) {
        static char message[MAX_MESSAGE_LENGTH]; // Create char for serial1 message
        static unsigned int message_pos = 0;
        char inByte = Serial1.read();
        if (inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1)) { // Add the incoming byte to our message
            message[message_pos] = inByte;
            message_pos++;
        } else { // Full message received...
            message[message_pos] = '\0'; // Add null character to string to end string
            int PS4input = atoi(message);
            message_pos = 0;
            if (PS4input == PSHOME){
                return 1;
            }
        }
    }
    return 0;
}

void PS4::joystick(int Xinput, int Yinput) {
    int L2, R2;
    // Safety for if only Yinput is received as Xinput
    if ((Xinput >= 9000 && Xinput <= 9255) ||
        (Xinput >= 5000 && Xinput <= 5255) ||
        (Xinput >= 7000 && Xinput <= 7255)
        ) Yinput = Xinput;
    //    Serial.print("Xinput: "); Serial.println(Xinput); Serial.print("Yinput: "); Serial.println(Yinput);

    //---------------------------------------------- RIGHT THUMBSTICK (absolute aim + slew)
    if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD) &&
        (ps4HasRightStickAxis(Xinput) || ps4HasRightStickAxis(Yinput))) {
		if (ps4UpdateRightStick(Xinput, Yinput, lastRX, lastRY)) {
			showDotEmotion(pestoEmotionFromHead(lastRX, lastRY));
			const Ps4HeadTargets aim = ps4HeadTargetsFromStick(lastRX, lastRY);
			if (!aim.active) {
				return;
			}
			headTargetXY = aim.xy;
			headTargetZ = aim.z;
			const uint32_t now = millis();
			const uint32_t dt = (lastHeadMs == 0) ? 16u : static_cast<uint32_t>(now - lastHeadMs);
			lastHeadMs = now;
			const int step = ps4HeadMaxDelta(dt);
			const int nextXY = ps4HeadSlew(pwm_board::posXY, headTargetXY, step);
			const int nextZ = ps4HeadSlew(pwm_board::posZ, headTargetZ, step);
			if (nextXY != pwm_board::posXY || nextZ != pwm_board::posZ) {
				pwm_board::posXY = nextXY;
				pwm_board::posZ = nextZ;
				pwm_board::pwm.setPWM(PWM_1, 0, pwm_board::pulseWidth(pwm_board::posZ));
				pwm_board::pwm.setPWM(PWM_0, 0, pwm_board::pulseWidth(pwm_board::posXY));
			}
		}
		return;
	}

    if (ps4HasLeftStickAxis(Xinput) || ps4HasLeftStickAxis(Yinput)) {
        if (ps4UpdateLeftStick(Xinput, Yinput, lastLX, lastLY)) {
            applyTankOutput(ps4TankFromStick(lastLX, lastLY));
            showDotEmotion(pestoEmotionFromDrive(lastLX, lastLY));
        }
        return;
    }

    if (ps4IsL2(Xinput) || ps4IsR2(Yinput) || ps4IsL2(Yinput) || ps4IsR2(Xinput)) {
        L2 = ps4IsL2(Xinput) ? (Xinput - 4000) : (ps4IsL2(Yinput) ? (Yinput - 4000) : 0);
        R2 = ps4IsR2(Yinput) ? (Yinput - 5000) : (ps4IsR2(Xinput) ? (Xinput - 5000) : 0);
        L2 = ps4ApplyDeadzone(L2);
        R2 = ps4ApplyDeadzone(R2);
        Ps4TankOutput tank;
        tank.leftVel = R2 - L2;
        tank.rightVel = R2 - L2;
        tank.stop = (L2 == 0 && R2 == 0);
        applyTankOutput(tank);
        // Treat throttle mix as forward/back axis (no strafe).
        showDotEmotion(pestoEmotionFromDrive(0, R2 - L2));
        return;
    }

    if (ps4UpdateLeftStick(Xinput, Yinput, lastLX, lastLY)) {
        applyTankOutput(ps4TankFromStick(lastLX, lastLY));
        showDotEmotion(pestoEmotionFromDrive(lastLX, lastLY));
    }
}

void PS4::driveWatchdog() {
    if (!stickDriving) {
        return;
    }
    if (Serial1.available() > 0) {
        return;
    }
    if (ps4DriveTimedOut(millis(), lastDriveMs)) {
        Motor::Car_Stop();
        stickDriving = false;
        lastLX = 0;
        lastLY = 0;
    }
}

bool PS4::isManualControlActive() {
    if (stickDriving) {
        return true;
    }
    const uint32_t now = millis();
    if (lastHeadMs != 0 && (now - lastHeadMs) < 500) {
        if (ps4HeadStickActive(lastRX, lastRY)) {
            return true;
        }
    }
    if (ps4HasLeftStickAxis(lastLX) || ps4HasLeftStickAxis(lastLY)) {
        if (lastDriveMs != 0 && (now - lastDriveMs) < 500) {
            return true;
        }
    }
    return false;
}

void PS4::pollSerial() {
    uint8_t msgBudget = 8;
    while (Serial1.available() > 0 && msgBudget > 0) {
        static char message[MAX_MESSAGE_LENGTH]; // Create char for serial1 message
        static unsigned int message_pos = 0;

        char inByte = Serial1.read();
        if (inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1)) { // Add the incoming byte to our message
            message[message_pos] = inByte;
            message_pos++;
        } else { // Full message received...
            message[message_pos] = '\0'; // Add null character to string to end string
            msgBudget--;

            int values[2] = {0, 0};
            uint8_t count = 0;
            parsePs4Message(message, values, &count);

            #if LOG_VERBOSE
                if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
                    logger::logln(message);
                }
            #endif

            if (count >= 1 && values[0] >= 4000) { //for double input X-Y
                PS4::joystick(values[0], count >= 2 ? values[1] : values[0]);
            } else if (count >= 1) {
				const uint32_t now = millis();
				const bool petHappy = (displayAdafruit::petStatus == 0);
				const bool inMenu = menu::isOpen();
                switch (values[0]) {
                    case SQUARE:
						if (!elapsed(now, lastButtonMs, 250)) break;
						showDotEmotion(pestoEmotionFromButton(SQUARE, petHappy));
						if (inMenu) {
							menu::save();
						} else {
							menu::toggleSensorsPage();
						}
						break;
                    case TRIANG:
						if (!elapsed(now, lastButtonMs, 250)) break;
						displayAdafruit::activatePet();
						showDotEmotion(pestoEmotionFromButton(TRIANG, petHappy));
                        break;
                    case xCROSS:
						if (!elapsed(now, lastButtonMs, 250)) break;
						if (inMenu) {
							menu::select();
						}
						showDotEmotion(pestoEmotionFromButton(xCROSS, petHappy));
						break;
                    case CIRCLE:
						if (inMenu) {
							if (!elapsed(now, lastButtonMs, 250)) break;
							menu::undo();
							showDotEmotion(pestoEmotionFromButton(CIRCLE, petHappy));
						} else {
							displayAdafruit::petStatus = !displayAdafruit::petStatus;
							showDotEmotion(pestoEmotionFromButton(
								CIRCLE, displayAdafruit::petStatus == 0));
						}
                        break;


					case DPAD_U:
						if (!elapsed(now, lastButtonMs, 250)) break;
						showDotEmotion(pestoEmotionFromButton(DPAD_U, petHappy));
						if (inMenu) {
							menu::up();
						}
						break;
                    case DPAD_R:
						showDotEmotion(pestoEmotionFromButton(DPAD_R, petHappy));
						if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
							if (pwm_board::posXY > 10) pwm_board::posXY -= 10;
						}
						break;
					case DPAD_D:
						if (!elapsed(now, lastButtonMs, 250)) break;
						showDotEmotion(pestoEmotionFromButton(DPAD_D, petHappy));
						if (inMenu) {
							menu::down();
						}
					break;
                    case DPAD_L:
						showDotEmotion(pestoEmotionFromButton(DPAD_L, petHappy));
						if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
							if (pwm_board::posXY < 170) pwm_board::posXY += 10;
						}
					break;

                    case 3101:
                    case 3401:
                    case 3201:
                    case 3301:
                        //Motor::Car_Stop();
                        break;

                    case xSHARE:
						laser_beam::onPress(now);
						break;
                    case OPTION:
						if (!elapsed(now, lastButtonMs, 250)) break;
						if (sense_react::isActive()) {
							sense_react::stop();
							showDotEmotion(PestoEmotion::Wink);
						} else {
							robot_modes::stopAll();
							sense_react::start();
							showDotEmotion(PestoEmotion::Curious);
						}
						break;
                    case PSHOME: if (main::use_barometer) barometer::baroMeter();break;
                    case L1:
						if (!elapsed(now, lastButtonMs, 100)) break;
						#if USE_TIMERS
						if (main::use_sd_card) {
							if (main::use_timers) {
								timers::timerTwoActive = !timers::timerTwoActive;
								timers::timerTreeActive = false;
								timers::timerButton = L1;
							}
						} else {
                          timers::timerTwoActive = !timers::timerTwoActive;
                          timers::timerTreeActive = false;
                          timers::timerButton = L1;
						}
                        #endif
						// Skip wink while compass owns the matrix (L1 sensor timer on).
						showDotEmotion(pestoEmotionFromButton(L1, petHappy));
                        break;
                    case TOUCHPD:
						if (!elapsed(now, lastButtonMs, 250)) break;
						if (FEATURE_ENABLED(main::use_menu, USE_MENU)) {
							menu::toggle();
						}
						break;
                    case R1:
						if (!elapsed(now, lastButtonMs, 100)) break;
						showDotEmotion(pestoEmotionFromButton(R1, petHappy));
						#if USE_TIMERS
						if (main::use_sd_card) {
							if (main::use_timers) {
								timers::timerTwoActive = !timers::timerTwoActive;
								timers::timerTreeActive = false;
								timers::timerButton = R1;
							}
						} else {
                          timers::timerTwoActive = !timers::timerTwoActive;
                          timers::timerTreeActive = false;
                          timers::timerButton = R1;
						}
                        #endif
                        break;
                    case L3:
					case 2310:
						if (radar_scan::isActive()) {
							radar_scan::stop();
						} else {
							radar_scan::start();
						}
						showDotEmotion(PestoEmotion::Curious);
						break;
					case 2301:
						break;
                    case R3:
					case 2410:
					case 2401:
						// Reserved / unused
						break;
                        //                    CHARGE  3500
                        //                    XAUDIO  3600
                        //                    MIC     3700
                        //                    PS4_Battery        3900 + Battery

                    default:
                        break;
                }
            }
            message_pos = 0; //Reset next message
        }

    }
}

void PS4::pollInput() {
	pollSerial();
	driveWatchdog();
	laser_beam::tick(millis());
}

void PS4::controller() {
	pollInput();
}
