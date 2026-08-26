//
// Created by mr on 11/19/2023.
//

#include "displayAdafruit.h"
#include <Wire.h>
#include "config.h"
#include "main_ra.h"
#include "logger.h"
#include <cstdint>
#include "deadline.h"
#include "log_buffer.h"
#include "oled_mode.h"
#include "compass.h"
#include "compass_heading.h"
#include "gyroscope.h"
#include "barometer.h"
#include "analog.h"
#include "avoid_objects.h"
#include "iot_iconset_16x16.h"
#include "sensor_format.h"
#include "oled_face_clip.h"
#include "pesto_matrix.h"
#include <Arduino.h>
#include <cstdio>
#include <cstring>
int displaySwitch = 1;
static LogBuffer gLog;
static OledMode sOledMode = OledMode::Log;
static bool sMenuDirty = false;
static uint32_t sPetUntilMs = 0;
static uint32_t sSensorsLastMs = 0;
static OledFaceClipId sFaceClip = OledFaceClipId::IdleBlink;
static uint8_t sFaceFrameIdx = 0;
static uint32_t sFaceLastMs = 0;
static char sFaceLabel[16] = "Idle";
static PestoEmotion sFaceEmotion = PestoEmotion::Idle;
static bool sFaceDirty = true;

Adafruit_SH1106G displayAdafruit::display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM logo16_glcd_bmp[] =
		{ B00000000, B11000000,
		  B00000001, B11000000,
		  B00000001, B11000000,
		  B00000011, B11100000,
		  B11110011, B11100000,
		  B11111110, B11111000,
		  B01111110, B11111111,
		  B00110011, B10011111,
		  B00011111, B11111100,
		  B00001101, B01110000,
		  B00011011, B10100000,
		  B00111111, B11100000,
		  B00111111, B11110000,
		  B01111100, B11110000,
		  B01110000, B01110000,
		  B00000000, B00110000
		};


void displayAdafruit::setupAdafruit(){
	delay(250); // wait for the OLED to power up

	if (!display.begin(SCREEN_ADDRESS, true)){
		main::use_adafruit = false;
		main::Found_Display = false;
		Serial.println("Adafruit Display Failed");
	} else {
		main::Found_Display = true;
		display.clearDisplay();
		display.display();
		display.setFont(&TomThumb);
		sOledMode = OledMode::Log;
		sMenuDirty = false;
		logger::logln("Display ok");

		display.setTextWrap(false);
		display.setTextSize(1);
		display.setTextColor(SH110X_WHITE);
	}
}

void displayAdafruit::setMode(OledMode mode) {
	if (mode == sOledMode) {
		return;
	}
	sOledMode = mode;
	if (mode == OledMode::SenseReact || mode == OledMode::Avoid) {
		sFaceDirty = true;
	}
}

OledMode displayAdafruit::getMode() {
	return sOledMode;
}

void displayAdafruit::markMenuDirty() {
	sMenuDirty = true;
}

bool displayAdafruit::isMenuDirty() {
	return sMenuDirty;
}

void displayAdafruit::clearMenuDirty() {
	sMenuDirty = false;
}

void displayAdafruit::activatePet() {
	setMode(OledMode::Pet);
	sPetUntilMs = millis() + 2000;
	animateScreen();
}

void displayAdafruit::tickPet() {
	if (sOledMode != OledMode::Pet) {
		return;
	}
	animateScreen();
	if (static_cast<int32_t>(millis() - sPetUntilMs) >= 0) {
		setMode(OledMode::Log);
	}
}

void displayAdafruit::drawSensorsPage() {
	if (!oledMayDrawSensors(sOledMode, FEATURE_ENABLED(main::use_oled_sensors, USE_OLED_SENSORS))) {
		return;
	}

	char line[40];
	display.clearDisplay();
	display.setFont(&TomThumb);
	display.setTextSize(1);
	display.setTextColor(SH110X_WHITE);

	// 16px icon rows — pack fields so H / A+G / B+D / M+Lt fit on 128x64.
	auto drawRow = [&](int yTop, const unsigned char* icon, const char* text) {
		if (yTop > 48) {
			return;
		}
		display.drawBitmap(0, yTop, icon, 16, 16, SH110X_WHITE);
		display.setCursor(18, yTop + 11);
		display.print(text);
	};

	int y = 0;
	if (FEATURE_ENABLED(main::use_compass, USE_COMPASS) && compass::mag != nullptr) {
		const double h = compass::readCompass();
		formatHeadingLine(line, sizeof(line), h,
			cardinalToString(cardinalFromDegrees(static_cast<float>(h))));
		drawRow(y, signal3_icon16x16, line);
		y += 16;
	}

	if (FEATURE_ENABLED(main::use_gyro, USE_GYRO) && y <= 48) {
		gyroscope::gyroRead();
		formatAccelGyroCompact(line, sizeof(line),
			gyroscope::ax, gyroscope::ay, gyroscope::az,
			gyroscope::gx, gyroscope::gy, gyroscope::gz);
		drawRow(y, siren_icon16x16, line);
		y += 16;
	}

	const bool showBaro = FEATURE_ENABLED(main::use_barometer, USE_BAROMETER)
		&& barometer::bmp != nullptr;
	const bool showDist = FEATURE_ENABLED(main::use_distance, USE_DISTANCE);
	if ((showBaro || showDist) && y <= 48) {
		float tempC = 0.0f;
		bool haveBaro = false;
		if (showBaro && barometer::bmp->takeForcedMeasurement()) {
			tempC = barometer::bmp->readTemperature();
			haveBaro = true;
		}
		double distCm = -1.0;
		if (showDist) {
			distCm = avoid_objects::checkDistanceLong(29000);
		}
		display.drawBitmap(0, y, haveBaro ? temperature_icon16x16 : humidity2_icon16x16,
			16, 16, SH110X_WHITE);
		int x = 18;
		if (haveBaro) {
			formatBaroTempCompact(line, sizeof(line), tempC);
			display.setCursor(x, y + 11);
			display.print(line);
			x += 30;
		}
		if (showDist) {
			formatDistMeters(line, sizeof(line), distCm);
			display.setCursor(x, y + 11);
			display.print(line);
			const int barW = distDisplayBarPx(distCm);
			const int barX = x + 34;
			display.drawRect(barX, y + 6, 50, 6, SH110X_WHITE);
			if (barW > 0) {
				display.fillRect(barX, y + 6, barW, 6, SH110X_WHITE);
			}
		}
		y += 16;
	}

	const bool showMic = FEATURE_ENABLED(main::use_audio, USE_AUDIO)
		|| FEATURE_ENABLED(main::use_analog, USE_ANALOG);
	const bool showLight = FEATURE_ENABLED(main::use_light, USE_LIGHT)
		|| FEATURE_ENABLED(main::use_analog, USE_ANALOG);
	if ((showMic || showLight) && y <= 56) {
		const int micL = static_cast<int>(analog::ext_analog_1);
		const int micR = static_cast<int>(analog::ext_analog_3);
		const int lightL = static_cast<int>(analog::ext_analog_0);
		const int lightR = static_cast<int>(analog::ext_analog_2);

		auto drawSensorBarStrip = [&](int yTop, const char* label, int lVal, int rVal) {
			display.setCursor(2, yTop + 6);
			display.print(label);
			const int barY = yTop + 2;
			const int lw = sensorBarPx(lVal, SENSOR_ANALOG_MAX, 36);
			const int rw = sensorBarPx(rVal, SENSOR_ANALOG_MAX, 36);
			display.drawRect(22, barY, 36, 5, SH110X_WHITE);
			if (lw > 0) {
				display.fillRect(22, barY, lw, 5, SH110X_WHITE);
			}
			display.drawRect(62, barY, 36, 5, SH110X_WHITE);
			if (rw > 0) {
				display.fillRect(62, barY, rw, 5, SH110X_WHITE);
			}
			display.setCursor(102, yTop + 6);
			display.print("LR");
		};

		if (showMic && showLight) {
			drawSensorBarStrip(y, "mic", micL, micR);
			drawSensorBarStrip(y + 8, "lt", lightL, lightR);
		} else if (showMic) {
			display.drawBitmap(0, y, speak_icon16x16, 16, 16, SH110X_WHITE);
			drawSensorBarStrip(y, "mic", micL, micR);
		} else {
			display.drawBitmap(0, y, sun_icon16x16, 16, 16, SH110X_WHITE);
			drawSensorBarStrip(y, "lt", lightL, lightR);
		}
	}

	display.display();
	sSensorsLastMs = millis();
}

void displayAdafruit::setSenseReactFace(OledFaceClipId clip, const char* label, PestoEmotion emotion) {
	bool changed = (clip != sFaceClip) || (emotion != sFaceEmotion);
	if (label != nullptr && strcmp(label, sFaceLabel) != 0) {
		changed = true;
	}
	if (!changed) {
		return;
	}
	sFaceClip = clip;
	sFaceEmotion = emotion;
	if (label != nullptr) {
		snprintf(sFaceLabel, sizeof(sFaceLabel), "%s", label);
	}
	sFaceDirty = true;
}

void displayAdafruit::setAvoidFace(const char* label) {
	if (label != nullptr && strcmp(label, sFaceLabel) == 0
		&& sFaceClip == OledFaceClipId::IdleBlink) {
		return;
	}
	sFaceClip = OledFaceClipId::IdleBlink;
	if (label != nullptr) {
		snprintf(sFaceLabel, sizeof(sFaceLabel), "%s", label);
	}
	sFaceDirty = true;
}

void displayAdafruit::drawSenseReactPage() {
	const uint32_t now = millis();
	const uint8_t nextFrame = oledFaceAdvanceFrame(sFaceClip, sFaceFrameIdx, now, sFaceLastMs);
	const bool frameAdvanced = (nextFrame != sFaceFrameIdx) || sFaceLastMs == 0;
	if (!frameAdvanced && !sFaceDirty) {
		return;
	}
	if (frameAdvanced) {
		sFaceFrameIdx = nextFrame;
		sFaceLastMs = now;
	}
	sFaceDirty = false;

	display.clearDisplay();
	const uint8_t displayFrame = oledFaceDisplayFrame(sFaceClip, sFaceFrameIdx, sFaceEmotion);
	const uint8_t* frame = oledFaceFrameData(sFaceClip, displayFrame);
	if (frame != nullptr) {
		display.drawBitmap(0, 0, frame, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
	}
	display.fillRect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, SH110X_BLACK);
	display.setFont(&TomThumb);
	display.setTextSize(1);
	display.setTextColor(SH110X_WHITE);
	display.setCursor(SCREEN_WIDTH - 40, SCREEN_HEIGHT - 9);
	display.print(sFaceLabel);
	display.display();
	if (frame != nullptr && FEATURE_ENABLED(main::use_dot, USE_DOT)) {
		Pesto::syncFromOledFrame(frame);
	}
}

void displayAdafruit::displayLoop(){
	if (sOledMode == OledMode::Pet) {
		tickPet();
		return;
	}
	if (sOledMode == OledMode::Sensors) {
		const uint32_t now = millis();
		if (elapsed(now, sSensorsLastMs, 250)) {
			drawSensorsPage();
		}
		return;
	}
	if (sOledMode == OledMode::AutonomyMenu) {
		// autonomy_menu draws only on input changes; keep its frame untouched.
		return;
	}
	if (sOledMode == OledMode::SenseReact || sOledMode == OledMode::Avoid) {
		drawSenseReactPage();
		return;
	}
}

void displayAdafruit::updateDisplay() {
	if (!oledModeAllowsDraw(sOledMode, OledMode::Log)) {
		return;
	}
	display.clearDisplay();
	display.setFont(&TomThumb);
	display.setCursor(0, 6);
	const int count = gLog.packedCount();
	for (int i = 0; i < count; ++i) {
		display.println(gLog.packedLine(i));
	}
	display.display();
}
/************************************************ Display Adafruit  *************************************************/
// section Display Adafruit
/***************************************************************************************************************/
void displayAdafruit::text(const char * text) {
	gLog.append(text);
}

void displayAdafruit::textln(const char * text) {
	gLog.appendln(text);
	updateDisplay();
}

void displayAdafruit::Int(int inter) {
	display.print(inter);
	display.display();
}

void displayAdafruit::Intln(int inter) {
	display.println(inter);
	display.display();
}

void displayAdafruit::Doubble(double dubby) {
	// text display tests
	display.print(dubby);
	display.display();
}

void displayAdafruit::Doubbleln(double dubby) {
	// text display tests
	display.println(dubby);
	display.display();
}

void displayAdafruit::Float(float floaty) {
	// text display tests
	display.print(floaty);
	display.display();
}

void displayAdafruit::Floatln(float floaty) {
	// text display tests
	display.println(floaty);
	display.display();
}

void displayAdafruit::hex(unsigned char hexa) {
	display.print("0x"); display.print(hexa, HEX);
	display.display();
}

void displayAdafruit::hexln(unsigned char hexa) {
	display.print("0x"); display.println(hexa, HEX);
	display.display();
}

void displayAdafruit::bitmap(unsigned char bmp []) {
	display.drawBitmap(30, 16, bmp, 16, 16, 1);
	display.display();
}


//u8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//// GLOBAL VARIABLES
const int framesPerSecond = 3;
int displayAdafruit::incoming;

// *** PICK THE ENVIRONMENT YOUR CREATURE LIVES IN ***
// 1 = Desert, 2 = Forest, 3 = Water
int displayAdafruit::environment = 1;
int displayAdafruit::petStatus = 1; // 0=HAPPY, 1=SAD
////

////////// HERE ARE WHERE THE INSTRUCTIONS FOR ANIMATION FRAMES GO
//NOTE: screen dimensions: 128x64
//NOTE: use the u8g2 library to write instructions for drawing images (for example using the shape and line functions)
//NOTE: only add your desired drawing functions, buffering/clearing/timing is handled for you :)
// https://github.com/olikraus/u8g2/wiki/u8g2reference#drawbox
//
void displayAdafruit::happyFrame1() {  //THE FIRST FRAME OF THE 'HAPPY' ANIMATION
	display.drawTriangle(46,4,46,16,55,16, SH110X_WHITE); // left ear
	display.drawTriangle(81,4,73,16,82,16, SH110X_WHITE); // right ear
	display.drawRect(46,15,36,33, SH110X_WHITE); //face
	drawEllipse(56.5, 25.5, 2, 2, SH110X_WHITE); // left eye
	drawEllipse(71.5, 25.5, 2, 2, SH110X_WHITE); // right eye
	display.drawTriangle(61.5,34, 67.5,34, 64.5, 36.67, SH110X_WHITE); // nose
	display.drawLine(64.5,36.5,64.5,40.5, SH110X_WHITE); // middle nose
	display.drawLine(59.5,35.5,52.5,33.5, SH110X_WHITE); // left top wisk
	display.drawLine(59.5,37.5,52.5,37.5, SH110X_WHITE); //left bottom wisk
	display.drawLine(69.5,35.5,76.5,33.5, SH110X_WHITE); // right top wisk
	display.drawLine(69.5,37.5,76.5,37.5, SH110X_WHITE); // right bottom wisk
	display.drawLine(59.5,40.5,62,43, SH110X_WHITE); // mouth 1
	display.drawLine(64.5,40.5,62,43, SH110X_WHITE); // mouth 2
	display.drawLine(64.5,40.5,67,43, SH110X_WHITE); // mouth 3
	display.drawLine(69.5,40.5,67,43, SH110X_WHITE); // mouth 4
}
void displayAdafruit::happyFrame2() {  //THE SECOND FRAME OF THE 'HAPPY' ANIMATION
	display.drawTriangle(46,4,46,16,55,16, SH110X_WHITE); // left ear
	display.drawTriangle(81,4,73,16,82,16, SH110X_WHITE); // right ear
	display.drawRect(46,15,36,33, SH110X_WHITE); //face
	drawEllipse(56.5, 25.5, 2, 2, SH110X_WHITE); // left eye
	drawEllipse(71.5, 25.5, 2, 2, SH110X_WHITE); // right eye
	display.drawTriangle(61.5,33, 67.5,34, 64.5, 36.67, SH110X_WHITE); // nose
	display.drawLine(64.5,35.5,64.5,39.5, SH110X_WHITE); // middle nose
	display.drawLine(59.5,35.5,52.5,31.5, SH110X_WHITE); // left top wisk
	display.drawLine(59.5,37.5,52.5,36.5, SH110X_WHITE); //left bottom wisk
	display.drawLine(69.5,35.5,76.5,31.5, SH110X_WHITE); // right top wisk
	display.drawLine(76.5,36.5,69.5,37.5, SH110X_WHITE); // right bottom wisk
	display.drawLine(59.5,39.5,62,42, SH110X_WHITE); // mouth 1
	display.drawLine(64.5,39.5,62,42, SH110X_WHITE); // mouth 2
	display.drawLine(64.5,39.5,67,42, SH110X_WHITE); // mouth 3
	display.drawLine(69.5,39.5,67,42, SH110X_WHITE); // mouth 4
}
void displayAdafruit::happyFrame3() {  //THE THIRD FRAME OF THE 'HAPPY' ANIMATION
	display.drawTriangle(46,4,46,16,55,16, SH110X_WHITE); // left ear
	display.drawTriangle(81,4,73,16,82,16, SH110X_WHITE); // right ear
	display.drawRect(46,15,36,33, SH110X_WHITE); //face
	drawEllipse(56.5, 25.5, 2, 2, SH110X_WHITE); // left eye
	drawEllipse(71.5, 25.5, 2, 2, SH110X_WHITE); // right eye
	display.drawTriangle(61.5,34, 67.5,34, 64.5, 36.67, SH110X_WHITE); // nose
	display.drawLine(64.5,36.5,64.5,40.5, SH110X_WHITE); // middle nose
	display.drawLine(59.5,35.5,52.5,33.5, SH110X_WHITE); // left top wisk
	display.drawLine(59.5,37.5,52.5,37.5, SH110X_WHITE); //left bottom wisk
	display.drawLine(69.5,35.5,76.5,33.5, SH110X_WHITE); // right top wisk
	display.drawLine(69.5,37.5,76.5,37.5, SH110X_WHITE); // right bottom wisk
	display.drawLine(59.5,40.5,62,43, SH110X_WHITE); // mouth 1
	display.drawLine(64.5,40.5,62,43, SH110X_WHITE); // mouth 2
	display.drawLine(64.5,40.5,67,43, SH110X_WHITE); // mouth 3
	display.drawLine(69.5,40.5,67,43, SH110X_WHITE); // mouth 4
}

void displayAdafruit::sadFrame1() {  //THE FIRST FRAME OF THE 'SAD' ANIMATION
	display.drawTriangle(46,4,46,16,55,16, SH110X_WHITE); // left ear
	display.drawTriangle(81,4,73,16,82,16, SH110X_WHITE); // right ear
	display.drawRect(46,15,36,33, SH110X_WHITE); //face
	drawEllipse(56.5, 25.5, 2, 2, SH110X_WHITE); // left eye
	drawEllipse(71.5, 25.5, 2, 2, SH110X_WHITE); // right eye
	display.drawTriangle(61.5,34, 67.5,34, 64.5, 36.67, SH110X_WHITE); // nose
	display.drawLine(64.5,36.5,64.5,40.5, SH110X_WHITE); // middle nose
	display.drawLine(59.5,35.5,52.5,33.5, SH110X_WHITE); // left top wisk
	display.drawLine(59.5,37.5,52.5,37.5, SH110X_WHITE); //left bottom wisk
	display.drawLine(69.5,35.5,76.5,33.5, SH110X_WHITE); // right top wisk
	display.drawLine(69.5,37.5,76.5,37.5, SH110X_WHITE); // right bottom wisk
	display.drawLine(59.5,40.5,62,43, SH110X_WHITE); // mouth 1
	display.drawLine(64.5,40.5,62,43, SH110X_WHITE); // mouth 2
	display.drawLine(64.5,40.5,67,43, SH110X_WHITE); // mouth 3
	display.drawLine(69.5,40.5,67,43, SH110X_WHITE); // mouth 4
}
void displayAdafruit::sadFrame2() {  //THE SECOND FRAME OF THE 'SAD' ANIMATION
	display.drawTriangle(46,4,46,16,55,16, SH110X_WHITE); // left ear
	display.drawTriangle(81,4,73,16,82,16, SH110X_WHITE); // right ear
	display.drawRect(46,15,36,33, SH110X_WHITE); //face
	display.drawLine(59.5,22.5,52.5,24.5, SH110X_WHITE); // low brow left
	display.drawLine(69.5,22.5,76.5,24.5, SH110X_WHITE); // low brow right
	drawEllipse(56.5, 25.5, 1, 1, SH110X_WHITE); // left eye
	drawEllipse(71.5, 25.5, 1, 1, SH110X_WHITE); // right eye
	display.drawTriangle(61.5,34, 67.5,34, 64.5, 36.67, SH110X_WHITE); // nose
	display.drawLine(64.5,36.5,64.5,40.5, SH110X_WHITE); // middle nose
	display.drawLine(59.5,35.5,52.5,35.5, SH110X_WHITE); // left top wisk
	display.drawLine(59.5,37.5,52.5,39.5, SH110X_WHITE); //left bottom wisk
	display.drawLine(69.5,35.5,76.5,35.5, SH110X_WHITE); // right top wisk
	display.drawLine(69.5,37.5,76.5,39.5, SH110X_WHITE); // right bottom wisk
	display.drawLine(58.5,41.5,62,43, SH110X_WHITE); // mouth 1
	display.drawLine(64.5,40.5,62,43, SH110X_WHITE); // mouth 2
	display.drawLine(64.5,40.5,67,43, SH110X_WHITE); // mouth 3
	display.drawLine(70.5,41.5,67,43, SH110X_WHITE); // mouth 4
}
void displayAdafruit::sadFrame3() {  //THE THIRD FRAME OF THE 'SAD' ANIMATION
	display.drawTriangle(46,4,46,16,55,16, SH110X_WHITE); // left ear
	display.drawTriangle(81,4,73,16,82,16, SH110X_WHITE); // right ear
	display.drawRect(46,15,36,33, SH110X_WHITE); //face
	drawEllipse(56.5, 25.5, 2, 2, SH110X_WHITE); // left eye
	drawEllipse(71.5, 25.5, 2, 2, SH110X_WHITE); // right eye
	display.drawTriangle(61.5,34, 67.5,34, 64.5, 36.67, SH110X_WHITE); // nose
	display.drawLine(64.5,36.5,64.5,40.5, SH110X_WHITE); // middle nose
	display.drawLine(59.5,35.5,52.5,33.5, SH110X_WHITE); // left top wisk
	display.drawLine(59.5,37.5,52.5,37.5, SH110X_WHITE); //left bottom wisk
	display.drawLine(69.5,35.5,76.5,33.5, SH110X_WHITE); // right top wisk
	display.drawLine(69.5,37.5,76.5,37.5, SH110X_WHITE); // right bottom wisk
	display.drawLine(59.5,40.5,62,43, SH110X_WHITE); // mouth 1
	display.drawLine(64.5,40.5,62,43, SH110X_WHITE); // mouth 2
	display.drawLine(64.5,40.5,67,43, SH110X_WHITE); // mouth 3
	display.drawLine(69.5,40.5,67,43, SH110X_WHITE); // mouth 4
}
//////////END OF ANIMATION FRAME INSTRUCTIONS


//// YOU DO NOT HAVE TO MODIFY THE REST OF THE CODE:
void displayAdafruit::animateScreen() {
	if (!oledModeAllowsDraw(sOledMode, OledMode::Pet)) {
		return;
	}
	static uint32_t lastFrameMs = 0;
	static int animFrame = 0;
	const uint32_t now = millis();
	if (!elapsed(now, lastFrameMs, 1000 / framesPerSecond)) {
		return;
	}

	display.clearDisplay();
	display.setFont(nullptr);
	if (displayAdafruit::petStatus == 0) {
		switch (animFrame) {
			case 0: displayAdafruit::happyFrame1(); break;
			case 1: displayAdafruit::happyFrame2(); break;
			case 2: displayAdafruit::happyFrame3(); break;
			default: displayAdafruit::happyFrame2(); break;
		}
	} else {
		switch (animFrame) {
			case 0: displayAdafruit::sadFrame1(); break;
			case 1: displayAdafruit::sadFrame2(); break;
			case 2: displayAdafruit::sadFrame3(); break;
			default: displayAdafruit::sadFrame2(); break;
		}
	}
	display.display();
	animFrame = (animFrame + 1) % 4;
}

void displayAdafruit::drawEllipse(int x0, int y0, int a, int b, uint8_t color) {
	int x = 0, y = b;
	int a2 = a * a, b2 = b * b;
	int crit1 = -(a2/4 + a%2 + b2);
	int crit2 = -(b2/4 + b%2 + a2);
	int crit3 = -(b2/4 + b%2);
	int t = -a2*y;
	int dxt = 2*b2*x, dyt = -2*a2*y;
	int d2xt = 2*b2, d2yt = 2*a2;
	while (y >= 0 && x <= a) {
		display.drawLine(x0 + x, y0 + y, x0 - x, y0 + y, color);
		display.drawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);
		if (2*t + x*d2yt < crit1 || 2*t + y*d2xt < crit3) {
			x++;
			dxt += d2xt;
			t += dxt;
		}
		else if (2*t - y*d2xt > crit2) {
			y--;
			dyt += d2yt;
			t += dyt;
		}
		else {
			x++;
			y--;
			dxt += d2xt;
			dyt += d2yt;
			t += dxt + dyt;
		}
	}
}

