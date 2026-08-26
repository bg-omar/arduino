//
// Created by mr on 7/8/2024.
//
#include "Arduino.h"
#include "config.h"
#include "main_ra.h"
#include "displayAdafruit.h"
#include "logger.h"
#include "oled_mode.h"

static bool sOledBootLog = true;

static void logToDisplay(const char* text, bool newline) {
	if (!main::Found_Display || !sOledBootLog) {
		return;
	}
	if (!oledModeAllowsDraw(displayAdafruit::getMode(), OledMode::Log)) {
		return;
	}
	if (newline) {
		displayAdafruit::textln(text);
	} else {
		displayAdafruit::text(text);
	}
}

void logger::endBootLog() {
	sOledBootLog = false;
	displayAdafruit::setMode(OledMode::None);
}

bool logger::isOledEnabled() {
	return sOledBootLog;
}

void logger::log(const char* text) {
	Serial.print(text);
	logToDisplay(text, false);
}

void logger::logln(const char* text) {
	Serial.println(text);
	logToDisplay(text, true);
}

void logger::logDoubble(double floaty) {
	Serial.print(floaty);
	char buf[16];
	dtostrf(floaty, 0, 2, buf);
	logToDisplay(buf, false);
}

void logger::logDoubbleln(double floaty) {
	Serial.println(floaty);
	char buf[16];
	dtostrf(floaty, 0, 2, buf);
	logToDisplay(buf, true);
}

void logger::logFloat(float floaty) {
	Serial.print(floaty);
	char buf[16];
	dtostrf(floaty, 0, 2, buf);
	logToDisplay(buf, false);
}

void logger::logFloatln(float floaty) {
	Serial.println(floaty);
	char buf[16];
	dtostrf(floaty, 0, 2, buf);
	logToDisplay(buf, true);
}

void logger::logInt(int inty) {
	Serial.print(inty);
	char buf[12];
	snprintf(buf, sizeof(buf), "%d", inty);
	logToDisplay(buf, false);
}

void logger::logIntln(int inty) {
	Serial.println(inty);
	char buf[12];
	snprintf(buf, sizeof(buf), "%d", inty);
	logToDisplay(buf, true);
}

void logger::logHex(unsigned char hexy, int i) {
	Serial.print(hexy, i);
	char buf[8];
	snprintf(buf, sizeof(buf), "0x%02X", hexy);
	logToDisplay(buf, false);
}

void logger::logHexln(unsigned char hexy, int i) {
	Serial.println(hexy, i);
	char buf[8];
	snprintf(buf, sizeof(buf), "0x%02X", hexy);
	logToDisplay(buf, true);
}
