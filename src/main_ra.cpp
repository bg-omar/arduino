/*
UNO BLE -->     DC:54:75:C3:D9:ED   -
PC USB Dongel   00:1F:E2:C8:82:BA
ESP 1           66:CB:3E:E9:02:8A
ESP small cam   3C:E9:0E:88:65:16
PS4 Controller: A4:AE:11:E1:8B:B3 (SONYWA) GooglyEyes
PS5 Controller: 88:03:4C:B5:00:66
USB-Shield BT   00:1a:7d:da:71:13
				00:1a:7d:da:71:13
*/


/***************************************************************************************************************/
// section include
/***************************************************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>



#include <wiring_private.h>
#include <cstring>
#include <cstdint>
#include <cmath>

#include <Adafruit_Sensor.h>
#include "Arduino_LED_Matrix.h"

#include "PS4.h"
#include "secrets.h"
#include "main_ra.h"
#include "menu.h"

#include "logger.h"
#include "barometer.h"
#include "animation.h"
#include "pwm_board.h"
#include "timers.h"
#include "compass.h"
#include "gyroscope.h"

#include "dancing.h"
#include "displayAdafruit.h"
#include "pesto_matrix.h"
#include "follow_light.h"
#include "motor.h"
#include "MicStereo.h"
#include "avoid_objects.h"
#include "I2Cscanner.h"

#include "analog.h"
#include "SD_card.h"
#include "deadline.h"
#include "sense_react.h"
#include "robot_modes.h"
#include "laser_beam.h"
#include "radar_scan.h"
#include "brain_link.h"

bool main::Found_Display = false;
bool main::Found_Gyro = false;
bool main::Found_Compass = false;
bool main::Found_Mics = false;
bool main::Found_PwmBoard = false;
bool main::Found_Switch = false;
bool main::Found_Sonar = false;

bool main::use_adafruit = false;
bool main::use_u8g2 = false;
bool main::small = false;
bool main::display_demo = false;
bool main::use_round = false;
bool main::use_menu = false;
bool main::log_debug = false;
bool main::use_ps4 = false;
bool main::use_sd_card = false;
bool main::use_gyro = false;
bool main::use_compass = false;
bool main::use_barometer = false;
bool main::use_distance = false;
bool main::use_i2c_scanner = false;
bool main::use_pwm_board = false;
bool main::use_dot = false;
bool main::use_audio = false;
bool main::use_switch = false;
bool main::use_analog = false;
bool main::use_light = false;
bool main::use_robot = false;
bool main::use_timers = true;
bool main::use_matrix = true;
bool main::use_matrix_preview = false;
bool main::read_esp32 = false;
bool main::use_lcd = false;
bool main::use_oled_sensors = false;

//int timers::timerButton;

int pwm_board::posXY = 90;  // set horizontal servo position
int pwm_board::posZ = 135;   // set vertical servo position

int Switch_8_State, Switch_9_State;

// Define an array to hold pixel data for a single frame (4 pixels)
uint32_t frame[] = {0, 0, 0, 0xFFFF};
ArduinoLEDMatrix matrix;


/********************************************** Setup booting the arduino **************************************/
// section Setup
/***************************************************************************************************************/
void setup(){
    Wire.begin();
	Serial.begin(9600);// Initialize the hardware serial port for debugging
	displayAdafruit::setupAdafruit();

	logger::logln("Wall-Z boot");

	delay(1);
	Serial1.begin(115200);
	delay(1);
	SERIAL_AT.begin(115200);
	#if USE_BRAIN_LINK
	brain_link::begin();
	#endif
	logger::logln("UART 9600/115200 + Brain link");
	delay(1);

	Motor::motor_setup();

	SD_card::initSD();

	if(main::use_sd_card) {
		if(main::use_i2c_scanner)	I2Cscanner::scan();
		delay(500);
		if(main::use_analog) {
			analog::analogSetup();

			if(main::use_audio) {
				MicStereo::MicSetup();
				delay(500);
			}
		}

		if(main::use_switch) {
			pinMode(8, INPUT_PULLUP);     /***** 8 ******/
			pinMode(9, INPUT_PULLUP);     /***** 9 ******/

			delay(50);
			Switch_8_State = digitalRead(8);
			Switch_9_State = digitalRead(9);

			if (Switch_8_State == LOW) { logger::log(" SWITCH_8 is on"); } else { logger::log(" SWITCH_8 is off"); }
			if (Switch_9_State == LOW) { logger::logln(" SWITCH_9 is on"); } else { logger::logln(" SWITCH_9 is off"); }

			delay(500);
		}

		if(main::use_pwm_board) { pwm_board::setupPWM();   logger::logln("PWM ok");}
		delay(500);
		if(main::use_matrix_preview) {
			matrix.begin();
		} else if(main::use_matrix) {
			matrix.loadSequence(animation);
			matrix.begin();
			matrix.autoscroll(300);
			matrix.play(true);
			delay(500);
			logger::logln(" R4 matrix ");
		}


		if(main::use_distance) {
			pinMode(Trig_PIN, OUTPUT);    /***** 6 ******/
			pinMode(Echo_PIN, INPUT);     /***** 7 ******/
			logger::logln("Sonar ok");
			delay(500);
		}

		if(main::use_dot) {
			Pesto::setup_pestoMatrix();
			delay(500);
		}

		if(main::use_gyro) {
			gyroscope::gyroSetup();
			delay(500);
		}

		if(main::use_compass) {
			compass::compassSetup();
			delay(500);
		}

		if(main::use_barometer) {
			barometer::baroSetup();
			delay(500);
		}

		if(main::use_timers) {
			timers::initTimers();
			delay(500);

        }
		logger::logln("Setup SD done");
	} else {
		logger::logln("Setup from config.h");
		#if USE_ROUND
			displayMenu::menuSetup();
		#endif

		#if USE_I2C_SCANNER
			I2Cscanner::scan();
			delay(500);
		#endif

		#if USE_ANALOG
			analog::analogSetup();
		#endif

		#if USE_AUDIO
			MicStereo::MicSetup();
			delay(500);
		#endif

		#if USE_SWITCH
			pinMode(SWITCH_8, INPUT_PULLUP);     /***** 8 ******/
			pinMode(SWITCH_9, INPUT_PULLUP);     /***** 9 ******/

			delay(50);
			Switch_8_State = digitalRead(SWITCH_8);
			Switch_9_State = digitalRead(SWITCH_9);

			if (Switch_8_State == LOW) {  logger::log(" SWITCH_8 is on");} else {logger::log(" SWITCH_8 is off");}
			if (Switch_9_State == LOW) {  logger::logln(" SWITCH_9 is on");} else {logger::logln(" SWITCH_9 is off");}

			delay(500);
		#endif

		#if USE_PWM_BOARD
			pwm_board::setupPWM();
			delay(500);
		#endif


		#if USE_MATRIX_PREVIEW
			matrix.begin();
		#elif USE_MATRIX
			matrix.loadSequence(animation);
			matrix.begin();
			matrix.autoscroll(300);
			matrix.play(true);
			delay(500);
			logger::log(" R4 matrix ");
		#endif

		#if USE_DISTANCE
			pinMode(Trig_PIN, OUTPUT);    /***** 6 ******/
			pinMode(Echo_PIN, INPUT);     /***** 7 ******/
			logger::log(" Sonar ");
			delay(500);
		#endif

		#if USE_DOT
			Pesto::setup_pestoMatrix();
			delay(500);
		#endif

		#if USE_COMPASS
			compass::compassSetup();
			delay(500);
		#endif

		#if USE_BAROMETER
			barometer::baroSetup();
			delay(500);
		#endif

		#if USE_GYRO
			gyroscope::gyroSetup();
			delay(500);
		#endif

		#if USE_TIMERS
			timers::initTimers();
			delay(500);

		//        general_timer::setup_General_Timer();
		//        delay(500);
		#endif

		logger::logln("Setup from config.h Complete");
	}
	logger::logln("Starting loop");
	logger::endBootLog();
}

/*********************************** Loop **********************************/
// section Loop
/***************************************************************************/

void loop(){
	const uint32_t now = millis();

	auto pollPs4 = []() {
		if (FEATURE_ENABLED(main::use_ps4, USE_PS4)) {
			PS4::pollInput();
		}
	};

	pollPs4();

	#if USE_BRAIN_LINK
	brain_link::poll(now);
	#endif

	#if USE_ROBOT
	auto tickRobotModes = [&]() {
		if (main::use_sd_card && !main::use_robot) {
			return;
		}
		if (radar_scan::isActive()) {
			radar_scan::tick();
			pollPs4();
			return;
		}
		if (avoid_objects::isActive()) {
			avoid_objects::tick();
		}
		pollPs4();
		if (Follow_light::isActive()) {
			Follow_light::tick();
		}
		pollPs4();
		if (dancing::isActive()) {
			dancing::tick();
		}
		pollPs4();
		if (sense_react::isActive()) {
			sense_react::tick();
		}
	};
	if (main::use_sd_card) {
		if (main::use_robot) {
			tickRobotModes();
		}
	} else {
		tickRobotModes();
	}
	#endif

	displayAdafruit::displayLoop();

	if (main::use_menu) {
		menu::loopMenu();
	}

	pollPs4();

	if (FEATURE_ENABLED(main::use_switch, USE_SWITCH)) {
		Switch_8_State = digitalRead(SWITCH_8);
		Switch_9_State = digitalRead(SWITCH_9);
	}

	static uint32_t lastAnalogMs = 0;
	if (FEATURE_ENABLED(main::use_analog, USE_ANALOG)) {
		if (elapsed(now, lastAnalogMs, 20)) {
			analog::analogLoop();
		}
	}

	static uint32_t lastGyroMs = 0;
	if (FEATURE_ENABLED(main::use_gyro, USE_GYRO) || main::Found_Gyro) {
		if (elapsed(now, lastGyroMs, 50)) {
			gyroscope::gyroDetectMovement();
		}
	}

	if (main::use_sd_card) {
		if (main::use_audio) {
			MicStereo::MicLoop();
		}
	} else {
		#if USE_AUDIO
			MicStereo::MicLoop();
		#endif
	}

	if (FEATURE_ENABLED(main::use_distance, USE_DISTANCE)) {
		static uint32_t lastSonarMs = 0;
		if (elapsed(now, lastSonarMs, 60)) {
			int distance = std::lround(avoid_objects::checkDistance());

			#if LOG_VERBOSE
				if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
					logger::log("Distance: ");
					logger::logIntln(distance);
				}
			#endif

			int lazer_brightness = map(distance, 0, 1000, 0, 4000);
			lazer_brightness = laser_beam::pwmForDistance(lazer_brightness);
			if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
				pwm_board::pwm.setPWM(LAZER_PIN, 0, lazer_brightness);
				if (!robot_modes::anyActive()) {
					if (distance >= 0 && distance < 35) {
						pwm_board::leftLedStrip(255, 0, 0);
						pwm_board::rightLedStrip(255, 0, 0);
					} else {
						pwm_board::leftLedStrip(70, 0, 70);
						pwm_board::rightLedStrip(70, 0, 70);
					}
				}
			}
		}
	} else if (FEATURE_ENABLED(main::use_pwm_board, USE_PWM_BOARD)) {
		static uint32_t lastIdleLedMs = 0;
		if (!robot_modes::anyActive() && elapsed(now, lastIdleLedMs, 200)) {
			lastIdleLedMs = now;
			pwm_board::leftLedStrip(0, 0, 70);
			pwm_board::rightLedStrip(0, 0, 70);
		}
	}

	pollPs4();

	const bool pestoOledSync = sense_react::isActive() || avoid_objects::isActive();
	if (main::use_sd_card) {
		if (main::use_dot && !main::use_timers && !pestoOledSync) {
			Pesto::pestoMatrix();
		}
	} else {
		#if USE_DOT && !USE_TIMERS
			if (!pestoOledSync) {
				Pesto::pestoMatrix();
			}
		#endif
	}

	if (main::use_sd_card) {
		if (main::use_timers) {
			timers::update();
		}
	} else {
		#if USE_TIMERS
			timers::update();
		#endif
	}

	#if !USE_BRAIN_LINK
	if (main::use_sd_card) {
		if (main::read_esp32) {
			if (SERIAL_AT.available()) {
				#if LOG_VERBOSE
					if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
						logger::log("ESP32 says: ");
					}
				#endif
				while (SERIAL_AT.available()) {
					Serial.write(SERIAL_AT.read());
				}
			}
		}
	} else {
		#if READ_ESP32
			if (SERIAL_AT.available()) {
				#if LOG_VERBOSE
					if (FEATURE_ENABLED(main::log_debug, LOG_DEBUG)) {
						logger::log("ESP32 says: ");
					}
				#endif
				while (SERIAL_AT.available()) {
					Serial.write(SERIAL_AT.read());
				}
			}
		#endif
	}

	#endif

	#if USE_MATRIX_PREVIEW
	if (main::use_sd_card) {
		if (main::use_matrix_preview) {
			if(Serial.available() >= 12){
				frame[0] = Serial.read() | Serial.read() << 8 | Serial.read() << 16 | Serial.read() << 24;
				frame[1] = Serial.read() | Serial.read() << 8 | Serial.read() << 16 | Serial.read() << 24;
				frame[2] = Serial.read() | Serial.read() << 8 | Serial.read() << 16 | Serial.read() << 24;
				matrix.loadFrame(frame);
			}
		}
	} else {
		if(Serial.available() >= 12){
			frame[0] = Serial.read() | Serial.read() << 8 | Serial.read() << 16 | Serial.read() << 24;
			frame[1] = Serial.read() | Serial.read() << 8 | Serial.read() << 16 | Serial.read() << 24;
			frame[2] = Serial.read() | Serial.read() << 8 | Serial.read() << 16 | Serial.read() << 24;
			matrix.loadFrame(frame);
		}
	}
	#endif

	pollPs4();
	#if USE_BRAIN_LINK
	brain_link::tick(millis());
	#endif
}



