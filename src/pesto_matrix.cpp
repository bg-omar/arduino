//
// Created by mr on 10/27/2023.
//

#include <Arduino.h>
#include "pesto_matrix.h"
#include "oled_pesto_sync.h"
#include "main_ra.h"
#include "logger.h"
#include "timers.h"

int Pesto::screen = 0;
PestoEmotion Pesto::heldEmotion = PestoEmotion::Idle;
uint32_t Pesto::holdUntilMs = 0;

/********************************************** the function for dot matrix display ****************************/
// section Pesto Matrix
/***************************************************************************************************************/

// 16x8 face bitmaps (bit 0 = top). Column-major, 16 columns.
static const unsigned char FACE_IDLE[] = {
	0x00, 0x00, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x00
};
static const unsigned char FACE_HAPPY[] = {
	0x00, 0x00, 0x24, 0x24, 0x00, 0x00, 0x42, 0x3C,
	0x3C, 0x42, 0x00, 0x00, 0x24, 0x24, 0x00, 0x00
};
static const unsigned char FACE_SAD[] = {
	0x00, 0x00, 0x24, 0x24, 0x00, 0x00, 0x3C, 0x42,
	0x42, 0x3C, 0x00, 0x00, 0x24, 0x24, 0x00, 0x00
};
static const unsigned char FACE_LOOK_LEFT[] = {
	0x00, 0x48, 0x48, 0x00, 0x00, 0x00, 0x00, 0x18,
	0x18, 0x00, 0x00, 0x00, 0x48, 0x48, 0x00, 0x00
};
static const unsigned char FACE_LOOK_RIGHT[] = {
	0x00, 0x00, 0x12, 0x12, 0x00, 0x00, 0x00, 0x18,
	0x18, 0x00, 0x00, 0x00, 0x00, 0x12, 0x12, 0x00
};
static const unsigned char FACE_LOOK_UP[] = {
	0x00, 0x00, 0x12, 0x12, 0x00, 0x00, 0x00, 0x18,
	0x18, 0x00, 0x00, 0x00, 0x12, 0x12, 0x00, 0x00
};
static const unsigned char FACE_LOOK_DOWN[] = {
	0x00, 0x00, 0x48, 0x48, 0x00, 0x00, 0x00, 0x18,
	0x18, 0x00, 0x00, 0x00, 0x48, 0x48, 0x00, 0x00
};
static const unsigned char FACE_HEART[] = {
	0x00, 0x0C, 0x1E, 0x3E, 0x7C, 0x3E, 0x1E, 0x0C,
	0x0C, 0x1E, 0x3E, 0x7C, 0x3E, 0x1E, 0x0C, 0x00
};
static const unsigned char FACE_ALERT[] = {
	0x00, 0x00, 0x3C, 0x42, 0x5A, 0x42, 0x3C, 0x00,
	0x00, 0x3C, 0x42, 0x5A, 0x42, 0x3C, 0x00, 0x00
};
static const unsigned char FACE_CURIOUS[] = {
	0x00, 0x00, 0x18, 0x24, 0x24, 0x18, 0x00, 0x18,
	0x18, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x3C, 0x00
};
static const unsigned char FACE_WINK[] = {
	0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x42, 0x3C,
	0x3C, 0x42, 0x00, 0x00, 0x24, 0x24, 0x00, 0x00
};

//the condition to start conveying data
void Pesto::IIC_start() {
    digitalWrite(DotClockPIN,HIGH);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,HIGH);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,LOW);
    delayMicroseconds(3);
}
//Convey data
void Pesto::IIC_send(unsigned char send_data) {
    for(char i = 0;i < 8;i++){  //Each byte has 8 bits 8bit for every character
        digitalWrite(DotClockPIN,LOW);  // pull down clock pin DotClockPIN to change the signal of SDA
        delayMicroseconds(3);
        if(send_data & 0x01){  //set high and low level of DotDataPIN according to 1 or 0 of every bit
            digitalWrite(DotDataPIN,HIGH);
        } else {
            digitalWrite(DotDataPIN,LOW);
        }
        delayMicroseconds(3);
        digitalWrite(DotClockPIN,HIGH); //pull up the clock pin DotClockPIN to stop transmission
        delayMicroseconds(3);
        send_data = send_data >> 1;  // detect bit by bit, shift the data to the right by one
    }
}

//The sign of ending data transmission
void Pesto::IIC_end() {
    digitalWrite(DotClockPIN,LOW);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,LOW);
    delayMicroseconds(3);
    digitalWrite(DotClockPIN,HIGH);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,HIGH);
    delayMicroseconds(3);
}

void Pesto::matrix_display(unsigned char matrix_value[]) {
    IIC_start();  // use the function of the data transmission start condition
    IIC_send(0xc0);  //select address

    for(int i = 0;i < 16;i++) { //pattern data has 16 bits
        IIC_send(matrix_value[i]); //convey the pattern data
    }

    IIC_end();   //end the transmission of pattern data
    IIC_start();
    IIC_send(0x8A);  //display control, set pulse width to 4/16 s
    IIC_end();
}

const unsigned char* Pesto::bitmapFor(PestoEmotion e) {
	switch (e) {
		case PestoEmotion::Happy:     return FACE_HAPPY;
		case PestoEmotion::Sad:       return FACE_SAD;
		case PestoEmotion::LookLeft:  return FACE_LOOK_LEFT;
		case PestoEmotion::LookRight: return FACE_LOOK_RIGHT;
		case PestoEmotion::LookUp:    return FACE_LOOK_UP;
		case PestoEmotion::LookDown:  return FACE_LOOK_DOWN;
		case PestoEmotion::Heart:     return FACE_HEART;
		case PestoEmotion::Alert:     return FACE_ALERT;
		case PestoEmotion::Curious:   return FACE_CURIOUS;
		case PestoEmotion::Wink:      return FACE_WINK;
		case PestoEmotion::Idle:
		default:                      return FACE_IDLE;
	}
}

bool Pesto::holdActive(uint32_t nowMs) {
	if (heldEmotion == PestoEmotion::Idle) {
		return false;
	}
	return static_cast<int32_t>(nowMs - holdUntilMs) < 0;
}

void Pesto::displayEmotion(PestoEmotion e) {
	unsigned char buf[16];
	const unsigned char* src = bitmapFor(e);
	for (int i = 0; i < 16; ++i) {
		buf[i] = src[i];
	}
	matrix_display(buf);
}

void Pesto::displayBitmap(const uint8_t colMajor16[16]) {
	unsigned char buf[16];
	for (int i = 0; i < 16; ++i) {
		buf[i] = colMajor16[i];
	}
	matrix_display(buf);
}

void Pesto::syncFromOledFrame(const uint8_t* oled1024) {
	if (timers::timerTwoActive && timers::timerButton == 2100) {
		return;
	}
	uint8_t pesto16[16];
	oledBitmapDownsample16x8(oled1024, pesto16);
	displayBitmap(pesto16);
	heldEmotion = PestoEmotion::Idle;
	holdUntilMs = 0;
}

void Pesto::showEmotion(PestoEmotion e, uint32_t nowMs, uint32_t holdMs) {
	if (e == PestoEmotion::Idle) {
		return;
	}
	// Compass owns the matrix while L1 sensor timer is active (L1 == 2100).
	if (timers::timerTwoActive && timers::timerButton == 2100) {
		return;
	}
	heldEmotion = e;
	holdUntilMs = nowMs + holdMs;
	displayEmotion(e);
}

void Pesto::pestoMatrix() {
    /********************************************** Make DotMatric Images*******************************************/
    // section DotMatrix Images
    /***************************************************************************************************************/

	const uint32_t now = millis();
	if (holdActive(now)) {
		displayEmotion(heldEmotion);
		return;
	}
	heldEmotion = PestoEmotion::Idle;

    // Array, used to store the data of the pattern
    unsigned char STOP01[] = {0x2E,0x2A,0x3A,0x00,0x02,0x3E,0x02,0x00,0x3E,0x22,0x3E,0x00,0x3E,0x0A,0x0E,0x00};
    unsigned char hou[] =    {0x00,0x7f,0x08,0x08,0x7f,0x00,0x3c,0x42,0x42,0x3c,0x00,0x3e,0x40,0x40,0x3e,0x00};
    unsigned char op[] =     {0x00,0x00,0x3c,0x42,0x42,0x3c,0x00,0x7e,0x12,0x12,0x0c,0x00,0x00,0x5e,0x00,0x00};
    unsigned char met[] =    {0xf8,0x0c,0xf8,0x0c,0xf8,0x00,0x78,0xa8,0xa8,0xb8,0x00,0x08,0x08,0xf8,0x08,0x08};
    unsigned char pesto[] =  {0xfe,0x12,0x12,0x7c,0xb0,0xb0,0x80,0xb8,0xa8,0xe8,0x08,0xf8,0x08,0xe8,0x90,0xe0};
    unsigned char bleh[] =   {0x00,0x11,0x0a,0x04,0x8a,0x51,0x40,0x40,0x40,0x40,0x51,0x8a,0x04,0x0a,0x11,0x00};


    switch (screen) {
        case 1: matrix_display(STOP01); break;
        case 2: matrix_display(hou);    break;
        case 3: matrix_display(op);     break;
        case 4: matrix_display(met);    break;
        case 5: matrix_display(pesto);  break;
        case 6: matrix_display(bleh);   break;
        default:matrix_display(bleh);
    }
    screen == 6 ? screen = 0 : screen += 1;
}

void Pesto::setup_pestoMatrix() {
    unsigned char clear[] =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    pinMode(DotClockPIN,OUTPUT);/***** 5 ******/
    pinMode(DotDataPIN,OUTPUT); /***** 4 ******/
    digitalWrite(DotClockPIN,LOW);
    digitalWrite(DotDataPIN,LOW);
    Pesto::matrix_display(reinterpret_cast<unsigned char *>(clear));
    Pesto::pestoMatrix();
	logger::logln("Dot matrix ok");
}
