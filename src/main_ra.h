//
// Created by mr on 11/13/2023.
//

#ifndef MAIN_RA_H
#define MAIN_RA_H


#include "config.h"

// SD present → runtime use_* flags. No SD → compile-time USE_* from config.h.
#define FEATURE_ENABLED(use_flag, USE_MACRO) (main::use_sd_card ? (use_flag) : (USE_MACRO))

class main {

public:
    static bool Found_Display;
    static bool Found_Gyro;
    static bool Found_Compass;
    static bool Found_Mics;
    static bool Found_PwmBoard;
    static bool Found_Switch;
    static bool Found_Sonar;

	static bool use_adafruit;
	static bool use_u8g2;
	static bool small;
	static bool display_demo;
	static bool use_round;
	static bool use_menu;
	static bool log_debug;
	static bool use_ps4;
	static bool use_sd_card;
	static bool use_gyro;
	static bool use_compass;
	static bool use_barometer;
	static bool use_distance;
	static bool use_i2c_scanner;
	static bool use_pwm_board;
	static bool use_dot;
	static bool use_audio;
	static bool use_switch;
	static bool use_analog;
	static bool use_light;
	static bool use_robot;
	static bool use_timers;
	static bool use_matrix;
	static bool use_matrix_preview;
	static bool read_esp32;
	static bool use_lcd;
	static bool use_oled_sensors;



private:
	static void log_helper(const char* text, bool newline);


};



#endif //MAIN_RA_H
