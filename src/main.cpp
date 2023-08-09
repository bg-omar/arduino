// Include the libraries: mklink /J arduino_libraries "C:\Program Files (x86)\Arduino\libraries"
#include <Arduino.h>
#include <cmath>
#include <Wire.h>
#include <SD.h>


#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_PWMServoDriver.h>


#include <LiquidCrystal_I2C.h>
#include <TimerEvent.h>
#include <U8g2lib.h>
#include <WiFiS3.h>

#include "index.h"
#include "secrets.h"


#define RAW_BUFFER_LENGTH  750  // 750 (600 if we have only 2k RAM) is the value for air condition remotes. Default is 112 if DECODE_MAGIQUEST is enabled, otherwise 100.
#define MARK_EXCESS_MICROS    20    // Adapt it to your IR receiver module. 20 is recommended for the cheap VS1838 modules.
#include <IRremote.hpp>


U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define MIN_PULSE_WIDTH       650
#define MAX_PULSE_WIDTH       2350
#define DEFAULT_PULSE_WIDTH   1500
#define FREQUENCY             50

uint8_t servonum = 0;



// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;


// Adafruit SD shields and modules: pin 10
const int chipSelect = 10;


/********************************************** Declare all the functions**********************************************/
void Car_front();
void Car_left();
void Car_right();
void Car_Stop();
void Car_Back();

void I2CScanner();
void calibrate_sensor();
void detectMovement();
void gyroFunc();
void compass();
float checkdistance();

void dance();
void avoid();

void light_track();
void IIC_start();
void IIC_send(unsigned char send_data);
void IIC_end();
void matrix_display(unsigned char matrix_value[]);
void pestoMatrix();
void perstoTimer();
void sensorTimer();
int pulseWidth(int);

/********************************************** Make DotMatric Images**********************************************/
// Array, used to store the data of the pattern
unsigned char STOP01[] = {0x2E,0x2A,0x3A,0x00,0x02,0x3E,0x02,0x00,0x3E,0x22,0x3E,0x00,0x3E,0x0A,0x0E,0x00};
unsigned char hou[] =    {0x00,0x7f,0x08,0x08,0x7f,0x00,0x3c,0x42,0x42,0x3c,0x00,0x3e,0x40,0x40,0x3e,0x00};
unsigned char op[] =     {0x00,0x00,0x3c,0x42,0x42,0x3c,0x00,0x7e,0x12,0x12,0x0c,0x00,0x00,0x5e,0x00,0x00};
unsigned char met[] =    {0xf8,0x0c,0xf8,0x0c,0xf8,0x00,0x78,0xa8,0xa8,0xb8,0x00,0x08,0x08,0xf8,0x08,0x08};
unsigned char pesto[] =  {0xfe,0x12,0x12,0x7c,0xb0,0xb0,0x80,0xb8,0xa8,0xe8,0x08,0xf8,0x08,0xe8,0x90,0xe0};
unsigned char bleh[] =   {0x00,0x11,0x0a,0x04,0x8a,0x51,0x40,0x40,0x40,0x40,0x51,0x8a,0x04,0x0a,0x11,0x00};

unsigned char front[] = {0x00,0x00,0x00,0x00,0x00,0x24,0x12,0x09,0x12,0x24,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char back[] = {0x00,0x00,0x00,0x00,0x00,0x24,0x48,0x90,0x48,0x24,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char left[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x28,0x10,0x44,0x28,0x10,0x44,0x28,0x10,0x00};
unsigned char right[] = {0x00,0x10,0x28,0x44,0x10,0x28,0x44,0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00,0x00};

unsigned char clear[] =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/********************************************** Set timer period for function **********************************************/
const int timerOnePeriod = 1000;
const int timerTwoPeriod = 250;
const int timerThreePeriod = 7000;
TimerEvent timerOne;
TimerEvent timerTwo;
TimerEvent timerThree;
boolean timerTwoActive = false;
boolean timerTreeActive = false;


/********************************************** Setup LCD & Make icon images**********************************************/
byte Heart[8] = { 0b00000, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000};
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

#define light_L_Pin A0 // define the pin of left photo resistor sensor
#define light_R_Pin A1 // define the pin of right photo resistor sensor
#define IR_Pin      A2

#define IR_RECEIVE_PIN      IR_Pin

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define Rem_OK 0xBF407F00 // Set al remote buttons
#define Rem_U  0xB946FF00
#define Rem_D  0xEA15FF00
#define Rem_L  0xBB44FF00
#define Rem_R  0xBC43FF00

#define Rem_1  0xE916FF00
#define Rem_2  0xE619FE00
#define Rem_3  0xF20DFE00
#define Rem_4  0xF30CFF00
#define Rem_5  0xE718FF00
#define Rem_6  0xA15EFD00
#define Rem_7  0xF708FF00
#define Rem_8  0xE31CFF00
#define Rem_9  0xA55AFF00
#define Rem_0  0xAD52FF00
#define Rem_x  0xBD42FF00
#define Rem_y  0xB54ADF00
#define IRepeat 0xFFFFFFFF

#define Trig 6  // ultrasonic trig Pin
#define Echo 7  // ultrasonic echo Pin
#define Led  2
const int LED_PIN = Led;  // Arduino pin connected to LED's pin

#define matrixData  4  // Set data  pin to 4
#define matrixClock 5  // Set clock pin to 5
#define SCL_Pin  matrixClock  //Set clock pin to A5
#define SDA_Pin  matrixData  //Set data pin to A4

#define MIC 8
unsigned long last_event = 0;

#define ML_PWM     11  // define PWM control pin of left motor
#define MR_Ctrl    12  // define the direction control pin of right motor
#define MR_PWM     3   // define PWM control pin of right motor
#define ML_Ctrl    13  // define the direction control pin of left motor

#define THRESHOLD 5

#define top  0
#define bot  1



int status = WL_IDLE_STATUS;

WiFiServer server(80);

uint64_t ir_rec, previousIR, timerButton; // set remote vars
int previousXY, previousZ;

int screen = 0;

Adafruit_MPU6050 mpu; // Set the gyroscope
float ax, ay, az, gx, gy, gz, baseAx, baseAy, baseAz, baseGx, baseGy, baseGz;

long random2, randomXY, randomZ;     //set random for choice making
float distanceF, distanceR, distanceL; // set var for distance mesure 

int lightSensorL ;        // value read from the R light sensor
int lightSensorR ;        // value read from the L light sensor
int outputValueR ;        // value output to the R PWM (analog out)
int outputValueL ;        // value output to the L PWM (analog out)
int calcValue ;           // inverse input

int posXY = 90;  // set horizontal servo position
int speedXY = 20;

int posZ = 25;   // set vertical servo position
int speedZ =  20;

int flag; // flag variable, it is used to entry and exist function

/* Assign a unique ID to this sensor at the same time */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);



/********************************************** the function to run motor**********************************************/
void Car_front(){
    digitalWrite(MR_Ctrl,HIGH);
    analogWrite(MR_PWM,255);
    digitalWrite(ML_Ctrl,HIGH);
    analogWrite(ML_PWM,255);
}

void Car_left(){
    digitalWrite(MR_Ctrl,LOW);
    analogWrite(MR_PWM,255);
    digitalWrite(ML_Ctrl,HIGH);
    analogWrite(ML_PWM,255);
}
void Car_right(){
    digitalWrite(MR_Ctrl,HIGH);
    analogWrite(MR_PWM,255);
    digitalWrite(ML_Ctrl,LOW);
    analogWrite(ML_PWM,255);
}
void Car_Stop(){
    digitalWrite(MR_Ctrl,LOW);
    analogWrite(MR_PWM,0);
    digitalWrite(ML_Ctrl,LOW);
    analogWrite(ML_PWM,0);
}

void Car_Back(){
    digitalWrite(MR_Ctrl,LOW);
    analogWrite(MR_PWM,255);
    digitalWrite(ML_Ctrl,LOW);
    analogWrite(ML_PWM,255);
}

void I2CScanner() {
    byte error, address;
    int nDevices;
    lcd.clear();
    lcd.setCursor(0, top);
    lcd.println("I2C Scanning...");
    nDevices = 0;
    lcd.setCursor(0,bot);
    delay(200);
    lcd.clear();
    lcd.setCursor(0, top);
    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            lcd.print(" 0x");
            if (address<16) {
                lcd.print("0");
            }
            lcd.print(address,HEX);
            nDevices++;
            delay(200);
        }
        else if (error==4) {
            lcd.clear();
            lcd.setCursor(0, top);
            lcd.print("Unknow error at address 0x");
            if (address<16) {
                lcd.setCursor(0,bot);
                lcd.print("0");
            }
            lcd.setCursor(0,bot);
            lcd.print(address,HEX);
        }
    }
    delay(20);
    lcd.clear();
    delay(20);
    lcd.setCursor(0, top);
    lcd.print(nDevices);
    delay(20);
    lcd.print(" devices");
    delay(100);
    if (nDevices == 0) {
        lcd.clear();
        lcd.setCursor(0, top);
        lcd.println("No I2C devices found");
    }
}


void gyroRead(){
    // Get new sensor events with the readings
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    ax = a.acceleration.x - baseAx;
    ay = a.acceleration.y - baseAy;
    az = a.acceleration.z - baseAz;
    gx = g.gyro.x - baseGx;
    gy = g.gyro.y - baseGy;
    gz = g.gyro.z - baseGz;
}

/********************************************** the gyroscope**********************************************/
void gyroFunc(){
    gyroRead();
    lcd.setCursor(0, top); // Sets the location at which subsequent text written to the LCD will be displayed
    (ax > 0) ? lcd.print("+"), lcd.print(ax) : lcd.print(ax);
    lcd.print(" ");
    (ay > 0) ? lcd.print("+"), lcd.print(ay) : lcd.print(ay);
    lcd.print(" ");
    (az > 0) ? lcd.print("+"), lcd.print(az) : lcd.print(az);
    lcd.print("   ");
    lcd.setCursor(0,bot);
    (gx > 0) ? lcd.print("+"), lcd.print(gx) : lcd.print(gx);
    lcd.print(" ");
    (gy > 0) ? lcd.print("+"), lcd.print(gy) : lcd.print(gy);
    lcd.print(" ");
    (gz > 0) ? lcd.print("+"), lcd.print(gz) : lcd.print(gz);
    lcd.print("   ");
}

/************************************************ MouthDisplay *************************************************/


void u8g2_prepare(void) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
}

void u8g2_box_title(uint8_t a) {
    u8g2.drawStr( 10+a*2, 5, "U8g2");
    u8g2.drawStr( 10, 20, "GraphicsTest");

    u8g2.drawFrame(0,0,u8g2.getDisplayWidth(),u8g2.getDisplayHeight() );
}

void u8g2_box_frame(uint8_t a) {
    u8g2.drawStr( 0, 0, "drawBox");
    u8g2.drawBox(5,10,20,10);
    u8g2.drawBox(10+a,15,30,7);
    u8g2.drawStr( 0, 30, "drawFrame");
    u8g2.drawFrame(5,10+30,20,10);
    u8g2.drawFrame(10+a,15+30,30,7);
}

void u8g2_disc_circle(uint8_t a) {
    u8g2.drawStr( 0, 0, "drawDisc");
    u8g2.drawDisc(10,18,9);
    u8g2.drawDisc(24+a,16,7);
    u8g2.drawStr( 0, 30, "drawCircle");
    u8g2.drawCircle(10,18+30,9);
    u8g2.drawCircle(24+a,16+30,7);
}

void u8g2_r_frame(uint8_t a) {
    u8g2.drawStr( 0, 0, "drawRFrame/Box");
    u8g2.drawRFrame(5, 10,40,30, a+1);
    u8g2.drawRBox(50, 10,25,40, a+1);
}

void u8g2_string(uint8_t a) {
    u8g2.setFontDirection(0);
    u8g2.drawStr(30+a,31, " 0");
    u8g2.setFontDirection(1);
    u8g2.drawStr(30,31+a, " 90");
    u8g2.setFontDirection(2);
    u8g2.drawStr(30-a,31, " 180");
    u8g2.setFontDirection(3);
    u8g2.drawStr(30,31-a, " 270");
}

void u8g2_line(uint8_t a) {
    u8g2.drawStr( 0, 0, "drawLine");
    u8g2.drawLine(7+a, 10, 40, 55);
    u8g2.drawLine(7+a*2, 10, 60, 55);
    u8g2.drawLine(7+a*3, 10, 80, 55);
    u8g2.drawLine(7+a*4, 10, 100, 55);
}

void u8g2_triangle(uint8_t a) {
    uint16_t offset = a;
    u8g2.drawStr( 0, 0, "drawTriangle");
    u8g2.drawTriangle(14,7, 45,30, 10,40);
    u8g2.drawTriangle(14+offset,7-offset, 45+offset,30-offset, 57+offset,10-offset);
    u8g2.drawTriangle(57+offset*2,10, 45+offset*2,30, 86+offset*2,53);
    u8g2.drawTriangle(10+offset,40+offset, 45+offset,30+offset, 86+offset,53+offset);
}

void u8g2_ascii_1() {
    char s[2] = " ";
    uint8_t x, y;
    u8g2.drawStr( 0, 0, "ASCII page 1");
    for( y = 0; y < 6; y++ ) {
        for( x = 0; x < 16; x++ ) {
            s[0] = y*16 + x + 32;
            u8g2.drawStr(x*7, y*10+10, s);
        }
    }
}

void u8g2_ascii_2() {
    char s[2] = " ";
    uint8_t x, y;
    u8g2.drawStr( 0, 0, "ASCII page 2");
    for( y = 0; y < 6; y++ ) {
        for( x = 0; x < 16; x++ ) {
            s[0] = y*16 + x + 160;
            u8g2.drawStr(x*7, y*10+10, s);
        }
    }
}

void u8g2_extra_page(uint8_t a)
{
    u8g2.drawStr( 0, 0, "Unicode");
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.setFontPosTop();
    u8g2.drawUTF8(0, 24, "☀ ☁");
    switch(a) {
        case 0:
        case 1:
        case 2:
        case 3:
            u8g2.drawUTF8(a*3, 36, "☂");
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            u8g2.drawUTF8(a*3, 36, "☔");
            break;
    }
}
void u8g2_xor(uint8_t a) {
    uint8_t i;
    u8g2.drawStr( 0, 0, "XOR");
    u8g2.setFontMode(1);
    u8g2.setDrawColor(2);
    for( i = 0; i < 5; i++)
    {
        u8g2.drawBox(10+i*16, 18 + (i&1)*4, 21,31);
    }
    u8g2.drawStr( 5+a, 19, "XOR XOR XOR XOR");
    u8g2.setDrawColor(0);
    u8g2.drawStr( 5+a, 29, "CLR CLR CLR CLR");
    u8g2.setDrawColor(1);
    u8g2.drawStr( 5+a, 39, "SET SET SET SET");
    u8g2.setFontMode(0);

}

#define cross_width 24
#define cross_height 24
static const unsigned char cross_bits[] U8X8_PROGMEM  = {
        0x00, 0x18, 0x00, 0x00, 0x24, 0x00, 0x00, 0x24, 0x00, 0x00, 0x42, 0x00,
        0x00, 0x42, 0x00, 0x00, 0x42, 0x00, 0x00, 0x81, 0x00, 0x00, 0x81, 0x00,
        0xC0, 0x00, 0x03, 0x38, 0x3C, 0x1C, 0x06, 0x42, 0x60, 0x01, 0x42, 0x80,
        0x01, 0x42, 0x80, 0x06, 0x42, 0x60, 0x38, 0x3C, 0x1C, 0xC0, 0x00, 0x03,
        0x00, 0x81, 0x00, 0x00, 0x81, 0x00, 0x00, 0x42, 0x00, 0x00, 0x42, 0x00,
        0x00, 0x42, 0x00, 0x00, 0x24, 0x00, 0x00, 0x24, 0x00, 0x00, 0x18, 0x00, };

#define cross_fill_width 24
#define cross_fill_height 24
static const unsigned char cross_fill_bits[] U8X8_PROGMEM  = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x18, 0x64, 0x00, 0x26,
        0x84, 0x00, 0x21, 0x08, 0x81, 0x10, 0x08, 0x42, 0x10, 0x10, 0x3C, 0x08,
        0x20, 0x00, 0x04, 0x40, 0x00, 0x02, 0x80, 0x00, 0x01, 0x80, 0x18, 0x01,
        0x80, 0x18, 0x01, 0x80, 0x00, 0x01, 0x40, 0x00, 0x02, 0x20, 0x00, 0x04,
        0x10, 0x3C, 0x08, 0x08, 0x42, 0x10, 0x08, 0x81, 0x10, 0x84, 0x00, 0x21,
        0x64, 0x00, 0x26, 0x18, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

#define cross_block_width 14
#define cross_block_height 14
static const unsigned char cross_block_bits[] U8X8_PROGMEM  = {
        0xFF, 0x3F, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20,
        0xC1, 0x20, 0xC1, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20, 0x01, 0x20,
        0x01, 0x20, 0xFF, 0x3F, };

void u8g2_bitmap_overlay(uint8_t a) {
    uint8_t frame_size = 28;

    u8g2.drawStr(0, 0, "Bitmap overlay");

    u8g2.drawStr(0, frame_size + 12, "Solid / transparent");
    u8g2.setBitmapMode(false /* solid */);
    u8g2.drawFrame(0, 10, frame_size, frame_size);
    u8g2.drawXBMP(2, 12, cross_width, cross_height, cross_bits);
    if(a & 4)
        u8g2.drawXBMP(7, 17, cross_block_width, cross_block_height, cross_block_bits);

    u8g2.setBitmapMode(true /* transparent*/);
    u8g2.drawFrame(frame_size + 5, 10, frame_size, frame_size);
    u8g2.drawXBMP(frame_size + 7, 12, cross_width, cross_height, cross_bits);
    if(a & 4)
        u8g2.drawXBMP(frame_size + 12, 17, cross_block_width, cross_block_height, cross_block_bits);
}

void u8g2_bitmap_modes(uint8_t transparent) {
    const uint8_t frame_size = 24;

    u8g2.drawBox(0, frame_size * 0.5, frame_size * 5, frame_size);
    u8g2.drawStr(frame_size * 0.5, 50, "Black");
    u8g2.drawStr(frame_size * 2, 50, "White");
    u8g2.drawStr(frame_size * 3.5, 50, "XOR");

    if(!transparent) {
        u8g2.setBitmapMode(false /* solid */);
        u8g2.drawStr(0, 0, "Solid bitmap");
    } else {
        u8g2.setBitmapMode(true /* transparent*/);
        u8g2.drawStr(0, 0, "Transparent bitmap");
    }
    u8g2.setDrawColor(0);// Black
    u8g2.drawXBMP(frame_size * 0.5, 24, cross_width, cross_height, cross_bits);
    u8g2.setDrawColor(1); // White
    u8g2.drawXBMP(frame_size * 2, 24, cross_width, cross_height, cross_bits);
    u8g2.setDrawColor(2); // XOR
    u8g2.drawXBMP(frame_size * 3.5, 24, cross_width, cross_height, cross_bits);
}

uint8_t draw_state = 0;

void draw(void) {
    u8g2_prepare();
    switch(draw_state >> 3) {
        case 0: u8g2_box_title(draw_state&7); break;
        case 1: u8g2_box_frame(draw_state&7); break;
        case 2: u8g2_disc_circle(draw_state&7); break;
        case 3: u8g2_r_frame(draw_state&7); break;
        case 4: u8g2_string(draw_state&7); break;
        case 5: u8g2_line(draw_state&7); break;
        case 6: u8g2_triangle(draw_state&7); break;
        case 7: u8g2_ascii_1(); break;
        case 8: u8g2_ascii_2(); break;
        case 9: u8g2_extra_page(draw_state&7); break;
        case 10: u8g2_xor(draw_state&7); break;
        case 11: u8g2_bitmap_modes(0); break;
        case 12: u8g2_bitmap_modes(1); break;
        case 13: u8g2_bitmap_overlay(draw_state&7); break;
    }
}

/************************************************ MouthDisplay END *************************************************/

/************************************************ Is acceleration? *************************************************/
void detectMovement() {
    gyroRead();
    if(( abs(ax) + abs(ay) + abs(az)) > THRESHOLD){
        timerTwoActive = true;
        timerTreeActive = true;
        timerButton = Rem_9;
    }
    if(( abs(gx) + abs(gy) + abs(gz)) > THRESHOLD){
        timerTwoActive = true;
        timerTreeActive = true;
        timerButton = Rem_7;
    }
}

void calibrate_sensor() {
    float totX = 0;
    float totY = 0;
    float totZ = 0;
    float totgX = 0;
    float totgY = 0;
    float totgZ = 0;
    sensors_event_t a, g, temp;
    delay(10);
    for (size_t i = 0; i < 10; i++) {
        mpu.getEvent(&a, &g, &temp);
        delay(10);
        totX += a.acceleration.x;
        delay(10);
        totY += a.acceleration.y;
        delay(10);
        totZ += a.acceleration.z;
        delay(10);
        totgX += g.gyro.x;
        delay(10);
        totgY += g.gyro.y;
        delay(10);
        totgZ += g.gyro.z;
        delay(10);
    }
    baseAx = totX / 10;
    baseAy = totY / 10;
    baseAz = totZ / 10;
    baseGx = totgX / 10;
    baseGy = totgY / 10;
    baseGz = totgZ / 10;
}
/*********************************************** the Compass **********************************************/
float readCompass(){
    lcd.setCursor(0, top);   //Set cursor to character 2 on line 0
    lcd.print("Compass ");
    sensors_event_t event; /// Get a new sensor event */
    mag.getEvent(&event);

    float heading = atan2(event.magnetic.y, event.magnetic.x);
    float declinationAngle = 0.035;
    heading += declinationAngle;

    if(heading < 0) {
        heading += 2 * PI;
    }
    if(heading > 2*PI) {
        heading -= 2 * PI;
    }
    float headingDegrees = (heading * 180/M_PI) - 90;
    return (headingDegrees < 0) ? 360 + headingDegrees : headingDegrees;
}

void compass(){
    float headingDegrees = readCompass();
    lcd.print(headingDegrees);
    if (headingDegrees >= 0 && headingDegrees < 45){
        lcd.setCursor(4,bot);
        lcd.print("North       ");
    }
    if (headingDegrees >= 45 && headingDegrees < 135){
        lcd.setCursor(4,bot);
        lcd.print("East        ");
    }
    if (headingDegrees >= 135 && headingDegrees < 225){
        lcd.setCursor(4,bot);
        lcd.print("South       ");
    }
    if (headingDegrees >= 225 && headingDegrees < 315){
        lcd.setCursor(4,bot);
        lcd.print("West        ");
    }
    if (headingDegrees >= 315 && headingDegrees < 360){
        lcd.setCursor(4,bot);
        lcd.print("North       ");
    }
}


/********************************************** control ultrasonic sensor**********************************************/
float checkdistance() {
    digitalWrite(Trig, LOW);
    delayMicroseconds(2);
    digitalWrite(Trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trig, LOW);
    float checkDistance = pulseIn(Echo, HIGH) / 58.00;  //58.20, that is, 2*29.1=58.2
    delay(10);
    return checkDistance;
}

/********************************************** arbitrary sequence**********************************************/
void dance() {

        randomXY = random(1, 180);
        randomZ = random(1, 160);
            pwm.setPWM(0, 0, pulseWidth(randomXY));
        delay(500);
            pwm.setPWM(1, 0, pulseWidth(randomZ));
        delay(500);

       // One pixel, row by row
        randomXY = random(1, 180);
        randomZ = random(1, 160);
            pwm.setPWM(0, 0, pulseWidth(randomXY));
        delay(500);
            pwm.setPWM(1, 0, pulseWidth(randomZ));
        delay(500);


        randomXY = random(1, 180);
        randomZ = random(1, 160);
            pwm.setPWM(0, 0, pulseWidth(randomXY));
        delay(500);
            pwm.setPWM(1, 0, pulseWidth(randomZ));
        delay(500);


}

/********************************************** Obstacle Avoidance Function**********************************************/
void avoid()
{
    flag = 0; ///the design that enter obstacle avoidance function
    while (flag == 0) {
        random2 = random(1, 100);
        distanceF= checkdistance();
        if (distanceF < 25) {
            analogWrite (Led, 255);
            Car_Stop(); /// robot stops
                pwm.setPWM(1, 0, pulseWidth(115));
            delay(10); ///delay in 200ms
                pwm.setPWM(1, 0, pulseWidth(90));
            delay(10); ///delay in 200ms
            analogWrite (Led, 0);
                pwm.setPWM(0, 0, pulseWidth(160)); /// look left
            for (int j = 1; j <= 10; j = j + (1)) { ///  the data will be more accurate if sensor detect a few times.
                distanceL = checkdistance();
            }
            delay(200);
                pwm.setPWM(0, 0, pulseWidth(20)); /// look right
            for (int k = 1; k <= 10; k = k + (1)) {
                distanceR = checkdistance();
            }
            if (distanceL < 50 || distanceR < 50) {
                if (distanceL > distanceR) {
                        pwm.setPWM(0, 0, pulseWidth(90));
                    Car_left();
                    delay(500); ///turn left 500ms
                    Car_front();
                }
                else {
                        pwm.setPWM(0, 0, pulseWidth(90));
                    Car_right();
                    delay(500);
                    Car_front();
                }
            } else {  /// not (distanceL < 50 || distanceR < 50)
                if ((long) (random2) % (long) (2) == 0) ///when the random number is even
                {
                        pwm.setPWM(0, 0, pulseWidth(90));
                    Car_left(); ///robot turns left
                    delay(500);
                    Car_front(); ///go forward
                }
                else
                {
                        pwm.setPWM(0, 0, pulseWidth(90));
                    Car_right(); ///robot turns right
                    delay(500);
                    Car_front(); ///go forward
                } } }
        else /// if (distanceF < 25) { If the front distance is greater than or equal, robot car will go forward
        {
            Car_front();
        }
    }
}

/********************************************** Light Follow **********************************************/
void light_track() {
    flag = 0;
    while (flag == 0) {
        lightSensorR = analogRead(light_R_Pin);
        lightSensorL = analogRead(light_L_Pin);
        if (lightSensorR > 650 && lightSensorL > 650) {
            Car_front();
        }
        else if (lightSensorR > 650 && lightSensorL <= 650) {
            Car_left();
        }
        else if (lightSensorR <= 650 && lightSensorL > 650) {
            Car_right();
        }
        else {
            Car_Stop();
        }
    }
}
/********************************************** the function for dot matrix display ***********************/
void matrix_display(unsigned char matrix_value[]) {
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

//the condition to start conveying data
void IIC_start() {
    digitalWrite(SCL_Pin,HIGH);
    delayMicroseconds(3);
    digitalWrite(SDA_Pin,HIGH);
    delayMicroseconds(3);
    digitalWrite(SDA_Pin,LOW);
    delayMicroseconds(3);
}
//Convey data
void IIC_send(unsigned char send_data) {
    for(char i = 0;i < 8;i++){  //Each byte has 8 bits 8bit for every character
        digitalWrite(SCL_Pin,LOW);  // pull down clock pin SCL_Pin to change the signal of SDA
        delayMicroseconds(3);
        if(send_data & 0x01){  //set high and low level of SDA_Pin according to 1 or 0 of every bit
            digitalWrite(SDA_Pin,HIGH);
        } else {
            digitalWrite(SDA_Pin,LOW);
        }
        delayMicroseconds(3);
        digitalWrite(SCL_Pin,HIGH); //pull up the clock pin SCL_Pin to stop transmission
        delayMicroseconds(3);
        send_data = send_data >> 1;  // detect bit by bit, shift the data to the right by one
    }
}

//The sign of ending data transmission
void IIC_end() {
    digitalWrite(SCL_Pin,LOW);
    delayMicroseconds(3);
    digitalWrite(SDA_Pin,LOW);
    delayMicroseconds(3);
    digitalWrite(SCL_Pin,HIGH);
    delayMicroseconds(3);
    digitalWrite(SDA_Pin,HIGH);
    delayMicroseconds(3);
}
/********************************************** END of the function for dot matrix display ***********************/

/********************************************** Show matrix images**********************************************/
void pestoMatrix() {
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

void defaultLCD(){
    lcd.setCursor(2,top);   //Set cursor to character 2 on line 0
    lcd.print("Hello Pesto!");
    lcd.setCursor(3, bot);
    lcd.write(0); // show custom caracter (heart)
    lcd.setCursor(5, bot);
    lcd.print("Love u");
    lcd.setCursor(12, bot);
    lcd.write(0);
}

void perstoTimer(){
    pestoMatrix();
    u8g2.clearBuffer();
    draw();
    u8g2.sendBuffer();

    // increase the state
    draw_state++;
    if ( draw_state >= 12*8 )
        draw_state = 0;

}
void sensorTimer(){
    if (timerTwoActive && timerButton == Rem_7){
        compass();
    }
    if (timerTwoActive && timerButton == Rem_9){
        gyroFunc();
    }
}
void resetTimers(){
    timerTwoActive = false;
    timerTreeActive = false;
    lcd.clear();
}


int pulseWidth(int angle)  //  pwm.setPWM(0, 0, pulseWidth(0));
{
    int pulse_wide, analog_value;
    pulse_wide   = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
    analog_value = int(float(pulse_wide) / 1000000 * FREQUENCY * 4096);
    lcd.println(analog_value);
    return analog_value;
}


void ledColorSet(int r_val, int g_val, int b_val)
{
    pwm.setPWM(15, 0, pulseWidth(r_val));
    pwm.setPWM(14, 0, pulseWidth(g_val));
    pwm.setPWM(13, 0, pulseWidth(b_val));
}

float getTemperature() {
    // YOUR SENSOR IMPLEMENTATION HERE
    // simulate the temperature value
    float temp_x100 = random(0, 10000);  // a ramdom value from 0 to 10000
    return temp_x100 / 100;              // return the simulated temperature value from 0 to 100 in float
}




void printWifiStatus() {
    lcd.clear();
    // print your board's IP address:
    lcd.setCursor(0, top);   //Set cursor to line 0
    lcd.println(WiFi.localIP());

    // print the received signal strength:
    lcd.setCursor(0,bot);   //Set cursor to line 1
    lcd.print("signal: ");
    lcd.print(WiFi.RSSI());
    lcd.println(" dBm");
    delay(1000);
}

/********************************************** Setup (booting the arduino)**********************************************/
void setup(){

    Wire.begin();
    lcd.init();
    lcd.backlight();      // Make sure backlight is on
    // create a new characters
    lcd.createChar(0, Heart);
    I2CScanner();
    delay(1000);

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(IR_Pin, ENABLE_LED_FEEDBACK);
    lcd.print(F("Ready to receive IR signals"));



    /* U8g2 Project: SSD1306 Test Board */
    pinMode(10, OUTPUT);
    pinMode(9, OUTPUT);
    digitalWrite(10, 0);
    digitalWrite(9, 0);
    u8g2.begin();


    lcd.clear();
    lcd.setCursor(2,top);   //Set cursor to character 2 on line 0
    lcd.print("MPU6050 test!    ");

    // Try to initialize!
    if (!mpu.begin()) {
        lcd.setCursor(0,bot);
        lcd.println("MPU6050 not found");
        while (1) {
            delay(1000);
        }
    } else {
        lcd.setCursor(2,bot);
        lcd.println("MPU6050 Found!    ");
        delay(1000);
        mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_260_HZ); /// 5, 10, 21, 44, 94, 184, 260(off)
        calibrate_sensor();
        lcd.clear();
        gyroFunc();
        delay(1000);
    }
    lcd.clear();
    /* Initialise the sensor */
    if(!mag.begin()) {
        lcd.setCursor(0,bot);
        lcd.print("HMC5883 not found   ");
        while (1) {
            delay(1000);
        }
    } else {
        lcd.setCursor(0,bot);
        lcd.print("HMC5883 Found!     ");
        delay(1000);
        lcd.clear();
        compass();
        delay(1000);
    }

    pinMode(MIC, INPUT);

    pwm.begin();
    pwm.setPWMFreq(FREQUENCY);  // Analog servos run at ~50 Hz updates


    delay(1000);

    lcd.clear();
    lcd.setCursor(0,top);
    lcd.print("WiFi Starting!");

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        lcd.clear();
        lcd.setCursor(2,top);
        lcd.println("Please upgrade the firmware");
    }

    lcd.setCursor(0,bot);
    lcd.println(ssid);
    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
        lcd.clear();
        lcd.setCursor(2,top);
        lcd.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 4 seconds for connection:
        delay(4000);
    }
    server.begin();
    // you're connected now, so print out the status:
    printWifiStatus();



    timerButton = Rem_OK;

    pinMode(Trig, OUTPUT);
    pinMode(Echo, INPUT);
    pinMode(ML_Ctrl, OUTPUT);
    pinMode(ML_PWM, OUTPUT);
    pinMode(MR_Ctrl, OUTPUT);
    pinMode(MR_PWM, OUTPUT);
    pinMode(Led, OUTPUT);

    pinMode(matrixClock,OUTPUT);
    pinMode(matrixData,OUTPUT);
    digitalWrite(matrixClock,LOW);
    digitalWrite(matrixData,LOW);
    matrix_display(clear);
    timerOne.set(timerOnePeriod, perstoTimer);
    timerTwo.set(timerTwoPeriod, sensorTimer);
    timerThree.set(timerThreePeriod, resetTimers);
    pestoMatrix();
    delay(1000);



    lcd.clear();
    pwm.setPWM(0, 0, pulseWidth(posXY));
    pwm.setPWM(1, 0, pulseWidth(posZ));
}


/********************************************** Main loop (running the arduino)**********************************************/
void loop(){
    // listen for incoming clients
    WiFiClient client = server.available();
    if (client) {
        // read the first line of HTTP request header
        String HTTP_req = "";
        while (client.connected()) {
            if (client.available()) {
                lcd.println("New HTTP Request");
                HTTP_req = client.readStringUntil('\n');  // read the first line of HTTP request
                lcd.print("<< ");
                lcd.println(HTTP_req);  // print HTTP request to lcd Monitor
                break;
            }
        }

        // read the remaining lines of HTTP request header
        while (client.connected()) {
            if (client.available()) {
                String HTTP_header = client.readStringUntil('\n');  // read the header line of HTTP request

                if (HTTP_header.equals("\r"))  // the end of HTTP request
                    break;

                lcd.print("<< ");
                lcd.println(HTTP_header);  // print HTTP request to lcd Monitor
            }
        }

        if (HTTP_req.indexOf("GET") == 0) {  // check if request method is GET
            // expected header is one of the following:
            // - GET led1/on
            // - GET led1/off
            if (HTTP_req.indexOf("led1/on") > -1) {  // check the path
                digitalWrite(LED_PIN, HIGH);           // turn on LED
                lcd.println("Turned LED on");
            } else if (HTTP_req.indexOf("led1/off") > -1) {  // check the path
                digitalWrite(LED_PIN, LOW);                    // turn off LED
                lcd.println("Turned LED off");
            } else {
                lcd.println("No command");
            }
        }

        // send the HTTP response
        // send the HTTP response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");  // the connection will be closed after completion of the response
        client.println();                     // the separator between HTTP header and body

        // send the HTTP response body
        float temp = getTemperature();
        String html = String(HTML_CONTENT);
        html.replace("TEMPERATURE_MARKER", String(temp, 2)); // replace the marker by a real value
        client.println(html);
        client.flush();

        // give the web browser time to receive the data
        delay(10);

        // close the connection:
        client.stop();
    }


    if (IrReceiver.decode()) {  // Grab an IR code   At 115200 baud, printing takes 200 ms for NEC protocol and 70 ms for NEC repeat
        if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {         // Check if the buffer overflowed
            lcd.clear();
            lcd.println(F("Try to increase the \"RAW_BUFFER_LENGTH\" value of " STR(RAW_BUFFER_LENGTH) " in " __FILE__));
            delay(100);
        } else {
            if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
                lcd.clear();
                lcd.println(F("Received noise or an unknown (or not yet enabled) protocol"));
                delay(100);
            }
            ir_rec = IrReceiver.decodedIRData.decodedRawData;
            lcd.clear();
            lcd.print(ir_rec, HEX);
        }
        IrReceiver.resume();                            // Prepare for the next value

        if (ir_rec == IRepeat) {
            ir_rec = previousIR;
        }
        switch (ir_rec) {
            /****** Head movements    ******/
            case Rem_1: posXY = min(180, posXY + speedXY); break;
            case Rem_2: posZ = min(160, posZ + speedZ); break;
            case Rem_3: posXY = max(0, posXY - speedXY); break;
            case Rem_4: posXY = 90; posZ = 45; break;
            case Rem_5: posZ = max(0, posZ - speedZ); break;
            case Rem_6: posXY = 90; posZ = 15; break;

                /****** Options & Sensors ******/
            case Rem_7:
                lcd.clear();
                timerTwoActive = !timerTwoActive;
                timerTreeActive = false;
                timerButton = Rem_7;
                delay(100);
                break;
            case Rem_8: dance(); break;
            case Rem_9:
                lcd.clear();
                timerTwoActive = !timerTwoActive;
                timerTreeActive = false;
                timerButton = Rem_9;
                delay(100);
                break;
            case Rem_x: avoid(); break;
            case Rem_y: light_track(); break;

                /****** Engine - driving  ******/
            case Rem_OK: Car_Stop(); break;
            case Rem_U: Car_front(); break;
            case Rem_D:
                Car_Back(); break;
            case Rem_L: Car_left(); break;
            case Rem_R: Car_right(); break;
        }
        if (posXY != previousXY) {
            pwm.setPWM(0, 0, pulseWidth(posXY));
        }
        if (posZ != previousZ) {
            pwm.setPWM(1, 0, pulseWidth(posZ));
        }
        previousIR = ir_rec;
        previousXY = posXY;
        previousZ = posZ;
    }
    detectMovement();
    timerOne.update();
    timerTwo.update();
    timerThree.update();
    if (!timerTwoActive){defaultLCD();}

    lightSensorL = analogRead(light_R_Pin);
    lightSensorR = analogRead(light_L_Pin);
    outputValueR = map(lightSensorL, 0, 1023, 0, 255);
    outputValueL = map(lightSensorR, 0, 1023, 0, 255);
    calcValue = 255 - (outputValueR + outputValueL);
    calcValue = (calcValue < 0) ? 0 : calcValue;
    analogWrite(Led, calcValue);
    distanceF = checkdistance();  /// assign the front distance detected by ultrasonic sensor to variable a
    if (distanceF < 35) {
        analogWrite (Led, 255);
        pestoMatrix();
        delay(distanceF);
    } else {
        analogWrite (Led, 0);
    }
/*    int r,g,b;
    r=random(0,100)%100; //get a random in (0,100)
    g=random(0,100)%100;
    b=random(0,100)%100;
    ledColorSet(r,g,b);//set random as a duty cycle value*/

    int output = digitalRead(MIC);
    if (output == LOW) {
        if (millis() - last_event > 25) {
            lcd.println("Clap sound was detected!");
        }
        last_event = millis();
    }
    // picture loop
    u8g2.firstPage();
    do {
        draw();
    } while( u8g2.nextPage() );

    // increase the state
    draw_state++;
    if ( draw_state >= 14*8 )
        draw_state = 0;


}


