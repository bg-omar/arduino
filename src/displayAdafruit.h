//
// Created by mr on 11/19/2023.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_DISPLAYADAFRUIT_H
#define ARDUINO_R4_UNO_WALL_Z_DISPLAYADAFRUIT_H

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/TomThumb.h>
#include "oled_mode.h"
#include "oled_face_clip.h"


#define i2c_Address 0x3C

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define PIXEL_BLACK 0   ///< Draw 'off' pixels
#define PIXEL_WHITE 1   ///< Draw 'on' pixels
#define PIXEL_INVERSE 2 ///< Invert pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS i2c_Address

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16


class displayAdafruit {
public:
	static void text(const char * text);
	static void Int(int inter);
	static void Float(float floaty);
	static void hex(unsigned char hexa);

	static void textln(const char * text);
	static void Intln(int inter);
	static void Floatln(float floaty);
	static void hexln(unsigned char hexa);
	static void bitmap(unsigned char bmp []);
	static void Doubble(double dubby);
	static void Doubbleln(double dubby);

	static void displayLoop();
	static void setupAdafruit();

	static void setMode(OledMode mode);
	static OledMode getMode();
	static void markMenuDirty();
	static bool isMenuDirty();
	static void clearMenuDirty();
	static void activatePet();
	static void tickPet();
	static void drawSensorsPage();
	static void setSenseReactFace(OledFaceClipId clip, const char* label,
			PestoEmotion emotion = PestoEmotion::Idle);
	static void setAvoidFace(const char* label);
	static void drawSenseReactPage();

	static Adafruit_SH1106G display;
	static int t;



	static void animateScreen();
	static void animate();
	static int incoming;
	static int environment;
	static int petStatus;
	static void happyFrame1();
	static void happyFrame2();
	static void happyFrame3();
	static void sadFrame1();
	static void sadFrame2();
	static void sadFrame3();

private:
	static void drawEllipse(int x0, int y0, int a, int b, uint8_t color);
	static uint8_t draw_state;


	static void updateDisplay();
};



#endif //ARDUINO_R4_UNO_WALL_Z_DISPLAYADAFRUIT_H
