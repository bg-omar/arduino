/*
UNO BLE -->     DC:54:75:C3:D9:ED   -
PC USB Dongel   00:1F:E2:C8:82:BA
ESP 1           66:CB:3E:E9:02:8A
ESP small cam   3C:E9:0E:88:65:16
PS4 Controller: A4:AE:11:E1:8B:B3 (SONYWA) GooglyEyes
PS5 Controller: 88:03:4C:B5:00:66
*/

/***************************************************************************************************************/
// section define
/***************************************************************************************************************/

//#define USE_WIFI_SERVER
#define USE_MOUTH_DISPLAY_ADAFRUIT
//#define USE_MOUTH_DISPLAY_U8G2
#define USE_SMALL_DISPLAY
#define USE_MOUTH_DISPLAY
//#define USE_JOYSTICK
#define ARDUINO_ARCH_RENESAS_UNO

/***************************************************************************************************************/
// section include
/***************************************************************************************************************/
#include "index.h"
#include "secrets.h"

#include "Arduino_LED_Matrix.h"
#include "animation.h"

#include <PS4BT.h>
#include <usbhub.h>

ArduinoLEDMatrix matrix;
#include <Arduino.h>
#include <cmath>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_BME280.h>

#include <LiquidCrystal_I2C.h>
#include <TimerEvent.h>
#include <ArduinoBLE.h>


#ifdef USE_MOUTH_DISPLAY_ADAFRUIT
    #include <SPI.h>
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1306.h>
    #include <Adafruit_SH110X.h>
    #define SCREEN_WIDTH 128 // OLED display width, in pixels
    #define SCREEN_HEIGHT 64 // OLED display height, in pixels

    #define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
    #define SCREEN_ADDRESS 0x3C

    #define NUMFLAKES     4 // Number of snowflakes in the animation example

    #define LOGO_HEIGHT   16
    #define LOGO_WIDTH    16
#endif

#ifdef USE_MOUTH_DISPLAY
    #include <U8g2lib.h>
#endif

#include <WiFiS3.h>

#define RAW_BUFFER_LENGTH  750
#define DECODE_NEC
#include <IRremote.hpp>

#define MIN_PULSE_WIDTH       650
#define MAX_PULSE_WIDTH       2350
#define FREQUENCY             50
#define SEALEVELPRESSURE_HPA (1013.25)


/********************************************** Declare all the functions***************************************/
// section declaration
/***************************************************************************************************************/
void Car_front();
void Car_left();
void Car_right();
void Car_Stop();
void Car_Back();

void I2CScanner();
void gyroCalibrate_sensor();
void gyroDetectMovement();
void gyroFunc();
void compass();
double checkDistance();

void dance();
void avoid();

void light_track();
void IIC_start();
void IIC_send(unsigned char send_data);
void IIC_end();
void matrix_display(unsigned char matrix_value[]);
void pestoMatrix();
void dotMatrixTimer();
void sensorTimer();
int pulseWidth(int);
void RGBled(int r_val, int g_val, int b_val);

/********************************************** Make DotMatric Images*******************************************/
// section DotMatrix Images
/***************************************************************************************************************/
// Array, used to store the data of the pattern
unsigned char STOP01[] = {0x2E,0x2A,0x3A,0x00,0x02,0x3E,0x02,0x00,0x3E,0x22,0x3E,0x00,0x3E,0x0A,0x0E,0x00};
unsigned char hou[] =    {0x00,0x7f,0x08,0x08,0x7f,0x00,0x3c,0x42,0x42,0x3c,0x00,0x3e,0x40,0x40,0x3e,0x00};
unsigned char op[] =     {0x00,0x00,0x3c,0x42,0x42,0x3c,0x00,0x7e,0x12,0x12,0x0c,0x00,0x00,0x5e,0x00,0x00};
unsigned char met[] =    {0xf8,0x0c,0xf8,0x0c,0xf8,0x00,0x78,0xa8,0xa8,0xb8,0x00,0x08,0x08,0xf8,0x08,0x08};
unsigned char pesto[] =  {0xfe,0x12,0x12,0x7c,0xb0,0xb0,0x80,0xb8,0xa8,0xe8,0x08,0xf8,0x08,0xe8,0x90,0xe0};
unsigned char bleh[] =   {0x00,0x11,0x0a,0x04,0x8a,0x51,0x40,0x40,0x40,0x40,0x51,0x8a,0x04,0x0a,0x11,0x00};

unsigned char north[] =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x04, 0x08, 0x10, 0x20, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char east[]  =  {0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x92, 0x92, 0x92, 0x92, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char south[] =  {0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x92, 0x92, 0x92, 0x92, 0xf2, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char west[]  =  {0x00, 0x00, 0x00, 0x3c, 0x40, 0x40, 0x40, 0x78, 0x78, 0x40, 0x40, 0x40, 0x3c, 0x00, 0x00, 0x00};

unsigned char front[] =  {0x00,0x00,0x00,0x00,0x00,0x24,0x12,0x09,0x12,0x24,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char back[] =   {0x00,0x00,0x00,0x00,0x00,0x24,0x48,0x90,0x48,0x24,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char left[] =   {0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x28,0x10,0x44,0x28,0x10,0x44,0x28,0x10,0x00};
unsigned char right[] =  {0x00,0x10,0x28,0x44,0x10,0x28,0x44,0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00,0x00};

unsigned char clear[] =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
byte Heart[8] = { 0b00000, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000};

static const unsigned char PROGMEM logo_bmp[] =
        { 0b00000000, 0b11000000,
          0b00000001, 0b11000000,
          0b00000001, 0b11000000,
          0b00000011, 0b11100000,
          0b11110011, 0b11100000,
          0b11111110, 0b11111000,
          0b01111110, 0b11111111,
          0b00110011, 0b10011111,
          0b00011111, 0b11111100,
          0b00001101, 0b01110000,
          0b00011011, 0b10100000,
          0b00111111, 0b11100000,
          0b00111111, 0b11110000,
          0b01111100, 0b11110000,
          0b01110000, 0b01110000,
          0b00000000, 0b00110000 };



/********************************************** PIN Defines ****************************************************/
// section pin define
/***************************************************************************************************************/

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define Rem_OK  0xBF407F
#define Rem_U   0xB946FF
#define Rem_D   0xEA15FF
#define Rem_L   0xBB44FF
#define Rem_R   0xBC43FF

#define Rem_1   0xE916FF
#define Rem_2   0xE619FE
#define Rem_3   0xF20DFE
#define Rem_4   0xF30CFF
#define Rem_5   0xE718FF
#define Rem_6   0xA15EFD
#define Rem_7   0xF708FF
#define Rem_8   0xE31CFF
#define Rem_9   0xA55AFF
#define Rem_0   0xAD52FF
#define Rem_x   0xBD42FF
#define Rem_y   0xB54ADF
#define IRepeat 0xFFFFFF

#define RX_PIN       0
#define TX_PIN       1
#define LED_PIN      2
#define MR_PWM       3   // define PWM control pin of right motor

#define DotDataPIN   4  // Set data  pin to 4
#define DotClockPIN  5  // Set clock pin to 5
#define Trig_PIN     6  // ultrasonic trig Pinz
#define Echo_PIN     7  // ultrasonic echo Pin

#define PIN_9        9
#define PIN_10      10
#define ML_PWM      11  // define PWM control pin of left motor
#define MR_Ctrl     12  // define the direction control pin of right motor
#define ML_Ctrl     13  // define the direction control pin of left motor


#define IR_Pin      A2
#define MIC_PIN     A3

#ifdef USE_JOYSTICK
    #define Joystick_X  A0
    #define Joystick_Y  A1
#else
    #define light_L_Pin A0
    #define light_R_Pin A1
#endif



#define PWM_0        0
#define PWM_1        1
#define PWM_2        2
#define PWM_3        3
#define PWM_4        4
#define PWM_5        5
#define PWM_6        6
#define PWM_7        7
#define PWM_8        8
#define PWM_9        9
#define PWM_10      10
#define PWM_11      11
#define PWM_12      12
#define PWM_13      13
#define PWM_14      14
#define PWM_15      15
#define PWM_16      16


/*************************************************** the Global Variables **************************************/
// section Global Variables
/***************************************************************************************************************/

const int timerOnePeriod = 1000;
const int timerTwoPeriod = 250;
const int timerThreePeriod = 7000;

boolean timerTwoActive = false;
boolean timerTreeActive = false;
unsigned long last_event = 0;
int status = WL_IDLE_STATUS;
#define THRESHOLD 5
#define top  0 // lcd screen top line
#define bot  1 // lcd screen bottom line

uint8_t servonum = 0;
uint64_t ir_rec, previousIR, timerButton; // set remote vars
int previousXY, previousZ;
int screen = 0;
float ax, ay, az, gx, gy, gz, baseAx, baseAy, baseAz, baseGx, baseGy, baseGz, temperature;
long random2, randomXY, randomZ;
double distanceF, distanceR, distanceL;

int R_velocity = 0;
int L_velocity = 0;

long baseSound;
int r,g,b;
int lightSensorL, lightSensorR, outputValueR, outputValueL, calcValue ;           // inverse input

int posXY = 90;  // set horizontal servo position
int speedXY = 20;

int posZ = 25;   // set vertical servo position
int speedZ =  20;

int flag; // flag variable, it is used to entry and exist function

/***************************************************** the Sensor Assigns **************************************/
// section Sensor Assigns
/***************************************************************************************************************/

TimerEvent timerOne;
TimerEvent timerTwo;
TimerEvent timerThree;

/* Assign a unique ID to this sensor at the same time */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
#ifdef USE_MOUTH_DISPLAY_ADAFRUIT
    #ifdef USE_SMALL_DISPLAY
        Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    #else
        Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    #endif
#endif

#ifdef USE_MOUTH_DISPLAY_U8G2
    U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif


BLEService carService("180A"); // create service: "Device Information"
// 2A57 is "Digital Output" // create direction control characteristic and allow remote device to read and write
BLEByteCharacteristic carControlCharacteristic("2A57", BLERead | BLEWrite);


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
LiquidCrystal_I2C lcd(0x27,16,2);
WiFiServer server(80);
Adafruit_MPU6050 mpu; // Set the gyroscope
Adafruit_BME280 bme;

// Satisfy the IDE, which needs to see the include statement in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

USB Usb;
//USBHub Hub1(&Usb); // Some dongles have a hub inside
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

/* You can create the instance of the PS4BT class in two ways */
// This will start an inquiry and then pair with the PS4 controller - you only have to do this once
// You will need to hold down the PS and Share button at the same time, the PS4 controller will then start to blink rapidly indicating that it is in pairing mode
//PS4BT PS4(&Btd, PAIR);

// After that you can simply create the instance like so and then press the PS button on the device
PS4BT PS4(&Btd);

bool printAngle, printTouch;
uint8_t oldL2Value, oldR2Value;
/************************************************** the I2CScanner *********************************************/
// section I2CScanner
/***************************************************************************************************************/

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

/************************************************** the gyroscope **********************************************/
// section Bluetooth5
/***************************************************************************************************************/

void bluetoothSetup() {
    lcd.clear();
    delay(20);
    lcd.setCursor(0, top);
    if (!BLE.begin()) {
        lcd.println("Bluetooth® Low Energy!");
        lcd.setCursor(0, bot);
        lcd.println("Module failed!");
    } else {
        lcd.println("Bluetooth® Low Energy!");
        lcd.setCursor(0, bot);
        lcd.println("Wall-Z Started!");
    }

    BLE.setLocalName("Wall-Z");
    BLE.setAdvertisedService(carService);

    // add the characteristics to the service
    carService.addCharacteristic(carControlCharacteristic);

    // add the service
    BLE.addService(carService);

    carControlCharacteristic.writeValue(0);

    // start advertising
    BLE.advertise();
    lcd.clear();
    delay(20);
    lcd.setCursor(0, top);
    lcd.println("Bluetooth® active,");
    delay(20);
    lcd.setCursor(0, bot);
    lcd.println("waiting for connections...");
}

void bluetoothListener() {
    // listen for BLE peripherals to connect:
    BLEDevice controller = BLE.central();
    
    lcd.clear();
    lcd.setCursor(0, top);
    // if a central is connected to peripheral:
    if (controller) {
        lcd.println("Connected to controller: ");
        lcd.setCursor(0, bot);
        // print the controller's MAC address:
        lcd.println(controller.address());

        for (int j = 1; j <= 10; j++) { ///  the data will be more accurate if sensor detect a few times.
            RGBled(0, 0, 255);
            delay(50);
            RGBled(0, 0, 0);
            delay(50);
        }
        
        // while the controller is still connected to peripheral:
        while (controller.connected()) {
            if (carControlCharacteristic.written()) {
                lcd.clear();
                lcd.setCursor(0, top);
                switch (carControlCharacteristic.value()) {
                    case 01:
                        lcd.println("LEFT");
                        break;
                    case 02:
                        lcd.println("RIGHT");
                        break;
                    case 03:
                        lcd.println("UP");
                        break;
                    case 04:
                        lcd.println("DOWN");
                        break;
                    default:  // 0 or invalid control
                        lcd.println("STOP");
                        break;
                }
            }
        }

        // when the central disconnects, print it out:
        lcd.print(F("Disconnected from controller: "));
        lcd.println(controller.address());
        digitalWrite(LED_BUILTIN, LOW);         // when the central disconnects, turn off the LED
    }
}

void bluetoothSerial(){
    int bluetooth_val;
    if (Serial.available())
    {
        bluetooth_val = Serial.read();
        Serial.println(bluetooth_val);
    }
    switch (bluetooth_val)
    {
        case 'F': //Forward instruction
            Car_front();
            matrix_display(front); //display forward pattern
            break;
        case 'B': //Back instruction
            Car_Back();
            matrix_display(back); // display back pattern
            break;
        case 'L': //left-turning instruction
            Car_left();
            matrix_display(left); //show left-turning pattern
            break;
        case 'R': //right-turning instruction
            Car_right();
            matrix_display(right); //show right-turning pattern
            break;
        case 'S': //stop instruction
            Car_Stop();
            matrix_display(STOP01); //display stop pattern
            break;
        case 'Y':
            matrix_display(pesto); //show start pattern
            dance();
            break;
        case 'U':
            matrix_display(bleh); //show start pattern
            avoid();
            break;
        case 'X':
            matrix_display(bleh); //show start pattern
            light_track();
            break;
        case 'C':
            lcd.clear();
            timerTwoActive = !timerTwoActive;
            timerTreeActive = false;
            timerButton = Rem_9;
            delay(100);
            break;
        case 'A':
            lcd.clear();
            timerTwoActive = !timerTwoActive;
            timerTreeActive = false;
            timerButton = Rem_7;
            delay(100);
            break;
        case 'a': posXY = min(180, posXY + speedXY); break;
        case 'w': posZ = min(160, posZ + speedZ); break;
        case 'd': posXY = max(0, posXY - speedXY); break;
        case 'q': posXY = 90; posZ = 45; break;
        case 's': posZ = max(0, posZ - speedZ); break;
        case 'e': posXY = 90; posZ = 15; break;
    }
}

void ps4() {
    if (PS4.getAnalogHat(LeftHatX) > 137 || PS4.getAnalogHat(LeftHatX) < 117 || PS4.getAnalogHat(LeftHatY) > 137 || PS4.getAnalogHat(LeftHatY) < 117 || PS4.getAnalogHat(RightHatX) > 137 || PS4.getAnalogHat(RightHatX) < 117 || PS4.getAnalogHat(RightHatY) > 137 || PS4.getAnalogHat(RightHatY) < 117) {
        Serial.print(F("\r\nLeftHatX: "));
        Serial.print(PS4.getAnalogHat(LeftHatX));
        Serial.print(F("\tLeftHatY: "));
        Serial.print(PS4.getAnalogHat(LeftHatY));
        Serial.print(F("\tRightHatX: "));
        Serial.print(PS4.getAnalogHat(RightHatX));
        Serial.print(F("\tRightHatY: "));
        Serial.print(PS4.getAnalogHat(RightHatY));
    }

    if (PS4.getAnalogButton(L2) || PS4.getAnalogButton(R2)) { // These are the only analog buttons on the PS4 controller
        Serial.print(F("\r\nL2: "));
        Serial.print(PS4.getAnalogButton(L2));
        Serial.print(F("\tR2: "));
        Serial.print(PS4.getAnalogButton(R2));
    }
    if (PS4.getAnalogButton(L2) != oldL2Value || PS4.getAnalogButton(R2) != oldR2Value) // Only write value if it's different
        PS4.setRumbleOn(PS4.getAnalogButton(L2), PS4.getAnalogButton(R2));
    oldL2Value = PS4.getAnalogButton(L2);
    oldR2Value = PS4.getAnalogButton(R2);

    if (PS4.getButtonClick(PS)) {
        Serial.print(F("\r\nPS"));
        PS4.disconnect();
    }
    else {
        if (PS4.getButtonClick(TRIANGLE)) {
            Serial.print(F("\r\nTriangle"));
            PS4.setRumbleOn(RumbleLow);
        }
        if (PS4.getButtonClick(CIRCLE)) {
            Serial.print(F("\r\nCircle"));
            PS4.setRumbleOn(RumbleHigh);
        }
        if (PS4.getButtonClick(CROSS)) {
            Serial.print(F("\r\nCross"));
            PS4.setLedFlash(10, 10); // Set it to blink rapidly
        }
        if (PS4.getButtonClick(SQUARE)) {
            Serial.print(F("\r\nSquare"));
            PS4.setLedFlash(0, 0); // Turn off blinking
        }

        if (PS4.getButtonClick(UP)) {
            Serial.print(F("\r\nUp"));
            PS4.setLed(Red);
        } if (PS4.getButtonClick(RIGHT)) {
            Serial.print(F("\r\nRight"));
            PS4.setLed(Blue);
        } if (PS4.getButtonClick(DOWN)) {
            Serial.print(F("\r\nDown"));
            PS4.setLed(Yellow);
        } if (PS4.getButtonClick(LEFT)) {
            Serial.print(F("\r\nLeft"));
            PS4.setLed(Green);
        }

        if (PS4.getButtonClick(L1))
            Serial.print(F("\r\nL1"));
        if (PS4.getButtonClick(L3))
            Serial.print(F("\r\nL3"));
        if (PS4.getButtonClick(R1))
            Serial.print(F("\r\nR1"));
        if (PS4.getButtonClick(R3))
            Serial.print(F("\r\nR3"));

        if (PS4.getButtonClick(SHARE))
            Serial.print(F("\r\nShare"));
        if (PS4.getButtonClick(OPTIONS)) {
            Serial.print(F("\r\nOptions"));
            printAngle = !printAngle;
        }
        if (PS4.getButtonClick(TOUCHPAD)) {
            Serial.print(F("\r\nTouchpad"));
            printTouch = !printTouch;
        }

        if (printAngle) { // Print angle calculated using the accelerometer only
            Serial.print(F("\r\nPitch: "));
            Serial.print(PS4.getAngle(Pitch));
            Serial.print(F("\tRoll: "));
            Serial.print(PS4.getAngle(Roll));
        }

        if (printTouch) { // Print the x, y coordinates of the touchpad
            if (PS4.isTouching(0) || PS4.isTouching(1)) // Print newline and carriage return if any of the fingers are touching the touchpad
                Serial.print(F("\r\n"));
            for (uint8_t i = 0; i < 2; i++) { // The touchpad track two fingers
                if (PS4.isTouching(i)) { // Print the position of the finger if it is touching the touchpad
                    Serial.print(F("X")); Serial.print(i + 1); Serial.print(F(": "));
                    Serial.print(PS4.getX(i));
                    Serial.print(F("\tY")); Serial.print(i + 1); Serial.print(F(": "));
                    Serial.print(PS4.getY(i));
                    Serial.print(F("\t"));
                }
            }
        }
    }
}
/************************************************** the gyroscope **********************************************/
// section gyroRead
/***************************************************************************************************************/

void gyroRead(){
    sensors_event_t a, gyro, temp;
    mpu.getEvent(&a, &gyro, &temp);

    temperature = temp.temperature;
    ax = a.acceleration.x - baseAx;
    ay = a.acceleration.y - baseAy;
    az = a.acceleration.z - baseAz;
    gx = gyro.gyro.x - baseGx;
    gy = gyro.gyro.y - baseGy;
    gz = gyro.gyro.z - baseGz;
}

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

void gyroDetectMovement() {
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

void gyroCalibrate_sensor() {
    float totX = 0;
    float totY = 0;
    float totZ = 0;
    float totgX = 0;
    float totgY = 0;
    float totgZ = 0;
    sensors_event_t a, gyro, temp;
    delay(10);
    for (size_t i = 0; i < 10; i++) {
        mpu.getEvent(&a, &gyro, &temp);
        delay(10);
        totX += a.acceleration.x;
        delay(10);
        totY += a.acceleration.y;
        delay(10);
        totZ += a.acceleration.z;
        delay(10);
        totgX += gyro.gyro.x;
        delay(10);
        totgY += gyro.gyro.y;
        delay(10);
        totgZ += gyro.gyro.z;
        delay(10);
    }
    baseAx = totX / 10;
    baseAy = totY / 10;
    baseAz = totZ / 10;
    baseGx = totgX / 10;
    baseGy = totgY / 10;
    baseGz = totgZ / 10;
}

void gyroSetup() {
    // Try to initialize!
    if (!mpu.begin()) {
        lcd.setCursor(0,bot);
        lcd.println("MPU6050 not found");
        delay(500);

    } else {
        lcd.setCursor(2,bot);
        lcd.println("MPU6050 Found!    ");
        delay(500);
        mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_260_HZ); /// 5, 10, 21, 44, 94, 184, 260(off)
        gyroCalibrate_sensor();
        delay(500);
    }
}

/*************************************************** the Compass ***********************************************/
// section Compass
/***************************************************************************************************************/

double readCompass(){
    lcd.setCursor(0, top);   //Set cursor to character 2 on line 0
    lcd.print("Compass ");
    sensors_event_t event; /// Get a new sensor event */
    mag.getEvent(&event);

    double heading = atan2(event.magnetic.y, event.magnetic.x);
    double declinationAngle = 0.035;
    heading += declinationAngle;

    if(heading < 0) {
        heading += 2 * PI;
    }
    if(heading > 2*PI) {
        heading -= 2 * PI;
    }
    double headingDegrees = (heading * 180/M_PI) - 90;
    return (headingDegrees < 0) ? 360 + headingDegrees : headingDegrees;
}

void compass(){
    double headingDegrees = readCompass();
    lcd.print(headingDegrees);
    if (headingDegrees >= 0 && headingDegrees < 45){
        matrix_display(north);
        lcd.setCursor(4,bot);
        lcd.print("North       ");
    }
    if (headingDegrees >= 45 && headingDegrees < 135){
        matrix_display(east);
        lcd.setCursor(4,bot);
        lcd.print("East        ");
    }
    if (headingDegrees >= 135 && headingDegrees < 225){
        matrix_display(south);
        lcd.setCursor(4,bot);
        lcd.print("South       ");
    }
    if (headingDegrees >= 225 && headingDegrees < 315){
        matrix_display(west);
        lcd.setCursor(4,bot);
        lcd.print("West        ");
    }
    if (headingDegrees >= 315 && headingDegrees < 360){
        matrix_display(north);
        lcd.setCursor(4,bot);
        lcd.print("North       ");
    }
}

void compassSetup() {
    /* Initialise the sensor */
    if(!mag.begin()) {
        lcd.setCursor(0,top);
        lcd.print("HMC5883 not found   ");
        delay(500);

    } else {
        lcd.setCursor(0,top);
        lcd.print("HMC5883 Found!     ");
        delay(500);
    }
}

/********************************************** control ultrasonic sensor***************************************/
// section BaroMeter
/***************************************************************************************************************/
void baroSetup() {
    lcd.clear();
    /* Initialise the sensor */
    if (!bme.begin(0x76)) {
        lcd.setCursor(0,top);
        lcd.println("BME280,not found!");
        delay(500);
    } else {
        lcd.setCursor(0,top);
        lcd.print("BME280 Found!     ");
        delay(500);
    }
}

void baroMeter() {
    lcd.clear();
    lcd.setCursor(0,top);
    lcd.print("Temp= ");
    lcd.print(bme.readTemperature());
    lcd.print("*C ");

    lcd.print("P= ");
    lcd.print(bme.readPressure() / 100.0F);
    lcd.print("hPa");

    lcd.setCursor(0,bot);
    lcd.print("Alt= ");
    lcd.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    lcd.println("m ");

    lcd.print("H= ");
    lcd.print(bme.readHumidity());
    lcd.println("%");
    delay(500);
}
/********************************************** control ultrasonic sensor***************************************/
// section UltraSonic
/***************************************************************************************************************/

double checkDistance() {
    digitalWrite(Trig_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(Trig_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trig_PIN, LOW);
    double checkDistance = pulseIn(Echo_PIN, HIGH) / 58.00;  //58.20, that is, 2*29.1=58.2
    delay(10);
    return checkDistance;
}

/********************************************** arbitrary sequence *********************************************/
// section Dance
/***************************************************************************************************************/
void dance() {

        randomXY = random(1, 180);
        randomZ = random(1, 160);
            pwm.setPWM(PWM_0, 0, pulseWidth(randomXY));
        delay(500);
            pwm.setPWM(PWM_1, 0, pulseWidth(randomZ));
        delay(500);

       // One pixel, row by row
        randomXY = random(1, 180);
        randomZ = random(1, 160);
            pwm.setPWM(PWM_0, 0, pulseWidth(randomXY));
        delay(500);
            pwm.setPWM(PWM_1, 0, pulseWidth(randomZ));
        delay(500);


        randomXY = random(1, 180);
        randomZ = random(1, 160);
            pwm.setPWM(PWM_0, 0, pulseWidth(randomXY));
        delay(500);
            pwm.setPWM(PWM_1, 0, pulseWidth(randomZ));
        delay(500);
    if (IrReceiver.decode()) {
        ir_rec = IrReceiver.decodedIRData.decodedRawData;
        IrReceiver.resume();
        if (ir_rec == Rem_OK) {
            flag = 1;
        }
    }


}

/********************************************** Obstacle Avoidance Function*************************************/
// section Avoid
/***************************************************************************************************************/

void avoid() {
    flag = 0; ///the design that enter obstacle avoidance function
    while (flag == 0) {
        random2 = random(1, 100);
        distanceF= checkDistance();
        if (distanceF < 25) {
            analogWrite (LED_PIN, 255);
            Car_Stop(); /// robot stops
                pwm.setPWM(PWM_1, 0, pulseWidth(115));
            delay(10); ///delay in 200ms
                pwm.setPWM(PWM_1, 0, pulseWidth(90));
            delay(10); ///delay in 200ms
            analogWrite (LED_PIN, 0);
                pwm.setPWM(PWM_0, 0, pulseWidth(160)); /// look left
            for (int j = 1; j <= 10; j = j + (1)) { ///  the data will be more accurate if sensor detect a few times.
                distanceL = checkDistance();
            }
            delay(200);
                pwm.setPWM(PWM_0, 0, pulseWidth(20)); /// look right
            for (int k = 1; k <= 10; k = k + (1)) {
                distanceR = checkDistance();
            }
            if (distanceL < 50 || distanceR < 50) {
                if (distanceL > distanceR) {
                        pwm.setPWM(PWM_0, 0, pulseWidth(90));
                    Car_left();
                    delay(500); ///turn left 500ms
                    Car_front();
                }
                else {
                        pwm.setPWM(PWM_0, 0, pulseWidth(90));
                    Car_right();
                    delay(500);
                    Car_front();
                }
            } else {  /// not (distanceL < 50 || distanceR < 50)
                if ((long) (random2) % (long) (2) == 0) ///when the random number is even
                {
                        pwm.setPWM(PWM_0, 0, pulseWidth(90));
                    Car_left(); ///robot turns left
                    delay(500);
                    Car_front(); ///go forward
                }
                else
                {
                        pwm.setPWM(PWM_0, 0, pulseWidth(90));
                    Car_right(); ///robot turns right
                    delay(500);
                    Car_front(); ///go forward
                } } }
        else /// if (distanceF < 25) { If the front distance is greater than or equal, robot car will go forward
        {
            Car_front();
        }
        if (IrReceiver.decode()) {
            ir_rec = IrReceiver.decodedIRData.decodedRawData;
            IrReceiver.resume();
            if (ir_rec == Rem_OK) {
                flag = 1;
            }
        }
    }
}

/*************************************************** Light Follow **********************************************/
// section Follow Light
/***************************************************************************************************************/

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
        if (IrReceiver.decode()) {
            ir_rec = IrReceiver.decodedIRData.decodedRawData;
            IrReceiver.resume();
            if (ir_rec == Rem_OK) {
                flag = 1;
            }
        }
    }
}

/********************************************** the function for dot matrix display ****************************/
// section Pesto Matrix
/***************************************************************************************************************/

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
    digitalWrite(DotClockPIN,HIGH);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,HIGH);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,LOW);
    delayMicroseconds(3);
}
//Convey data
void IIC_send(unsigned char send_data) {
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
void IIC_end() {
    digitalWrite(DotClockPIN,LOW);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,LOW);
    delayMicroseconds(3);
    digitalWrite(DotClockPIN,HIGH);
    delayMicroseconds(3);
    digitalWrite(DotDataPIN,HIGH);
    delayMicroseconds(3);
}

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

/********************************************** the function to run motor **************************************/
// section motor function
/***************************************************************************************************************/

void Car_front(){
    digitalWrite(MR_Ctrl,HIGH);
    analogWrite(MR_PWM,200);
    digitalWrite(ML_Ctrl,HIGH);
    analogWrite(ML_PWM,200);
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
    analogWrite(MR_PWM,200);
    digitalWrite(ML_Ctrl,LOW);
    analogWrite(ML_PWM,200);
}


/********************************************** the function to run motor **************************************/
// section Analog Joystick
/***************************************************************************************************************/


void joystick() {
    #ifdef USE_JOYSTICK
        int xAxis = analogRead(Joystick_X); // Read Joysticks X-axis
        int yAxis = analogRead(Joystick_Y); // Read Joysticks Y-axis
    #else
        int xAxis = analogRead(light_R_Pin);
        int yAxis = analogRead(light_L_Pin);
    #endif

    // Y-axis used for forward and backward control
    if (yAxis < 470) { // Set Motor A backward
        digitalWrite(MR_Ctrl, LOW);
        digitalWrite(ML_Ctrl, LOW);
        // Convert the declining Y-axis readings for going backward from 470 to 0 into 0 to 255 value for the PWM signal for increasing the motor speed
        R_velocity = map(yAxis, 470, 0, 0, 255);
        L_velocity = map(yAxis, 470, 0, 0, 255);
    }
    else if (yAxis > 550) { // Set Motor A forward
        digitalWrite(MR_Ctrl, HIGH);
        digitalWrite(ML_Ctrl, HIGH);
        // Convert the increasing Y-axis readings for going forward from 550 to 1023 into 0 to 255 value for the PWM signal for increasing the motor speed
        R_velocity = map(yAxis, 550, 1023, 0, 255);
        L_velocity = map(yAxis, 550, 1023, 0, 255);
    }
        // If joystick stays in middle the motors are not moving
    else {
        R_velocity = 0;
        L_velocity = 0;
    }

    // X-axis used for left and right control
    if (xAxis < 470) {
        // Convert the declining X-axis readings from 470 to 0 into increasing 0 to 255 value
        int xMapped = map(xAxis, 470, 0, 0, 255);
        // Move to left - decrease left motor speed, increase right motor speed
        R_velocity = R_velocity - xMapped;
        L_velocity = L_velocity + xMapped;
        // Confine the range from 0 to 255
        if (R_velocity < 0) {
            R_velocity = 0;
        }
        if (L_velocity > 255) {
            L_velocity = 255;
        }
    }
    if (xAxis > 550) {
        // Convert the increasing X-axis readings from 550 to 1023 into 0 to 255 value
        int xMapped = map(xAxis, 550, 1023, 0, 255);
        // Move right - decrease right motor speed, increase left motor speed
        R_velocity = R_velocity + xMapped;
        L_velocity = L_velocity - xMapped;
        // Confine the range from 0 to 255
        if (R_velocity > 255) {
            R_velocity = 255;
        }
        if (L_velocity < 0) {
            L_velocity = 0;
        }
    }
    // Prevent buzzing at low speeds (Adjust according to your motors. My motors couldn't start moving if PWM value was below value of 70)
    if (R_velocity < 70) {
        R_velocity = 0;
    }
    if (L_velocity < 70) {
        L_velocity = 0;
    }
    analogWrite(MR_PWM, R_velocity); // Send PWM signal to motor A
    analogWrite(ML_PWM, L_velocity); // Send PWM signal to motor B
}

/********************************************** Light Sensors **************************************/
// section Light Sensors
/***************************************************************************************************************/

int lightSensor(){
    #ifdef USE_JOYSTICK
        calcValue = 0
    #else
        lightSensorL = analogRead(light_R_Pin);
        lightSensorR = analogRead(light_L_Pin);
        outputValueR = map(lightSensorL, 0, 1023, 0, 255);
        outputValueL = map(lightSensorR, 0, 1023, 0, 255);
        calcValue = 255 - (outputValueR + outputValueL)*.5;
    #endif
        return (calcValue < 0) ? 0 : calcValue;
}


/************************************************ Display Adafruit  *************************************************/
// section Display Adafruit
/***************************************************************************************************************/

#ifdef USE_MOUTH_DISPLAY_ADAFRUIT
    void testdrawline() {
        int16_t i;

        display.clearDisplay(); // Clear display buffer

        for(i=0; i<display.width(); i+=4) {
            display.drawLine(0, 0, i, display.height()-1, SSD1306_WHITE);
            display.display(); // Update screen with each newly-drawn line
            delay(1);
        }
        for(i=0; i<display.height(); i+=4) {
            display.drawLine(0, 0, display.width()-1, i, SSD1306_WHITE);
            display.display();
            delay(1);
        }
        delay(250);

        display.clearDisplay();

        for(i=0; i<display.width(); i+=4) {
            display.drawLine(0, display.height()-1, i, 0, SSD1306_WHITE);
            display.display();
            delay(1);
        }
        for(i=display.height()-1; i>=0; i-=4) {
            display.drawLine(0, display.height()-1, display.width()-1, i, SSD1306_WHITE);
            display.display();
            delay(1);
        }
        delay(250);

        display.clearDisplay();

        for(i=display.width()-1; i>=0; i-=4) {
            display.drawLine(display.width()-1, display.height()-1, i, 0, SSD1306_WHITE);
            display.display();
            delay(1);
        }
        for(i=display.height()-1; i>=0; i-=4) {
            display.drawLine(display.width()-1, display.height()-1, 0, i, SSD1306_WHITE);
            display.display();
            delay(1);
        }
        delay(250);

        display.clearDisplay();

        for(i=0; i<display.height(); i+=4) {
            display.drawLine(display.width()-1, 0, 0, i, SSD1306_WHITE);
            display.display();
            delay(1);
        }
        for(i=0; i<display.width(); i+=4) {
            display.drawLine(display.width()-1, 0, i, display.height()-1, SSD1306_WHITE);
            display.display();
            delay(1);
        }

        delay(200); // Pause for 2 seconds
    }

    void testdrawrect() {
        display.clearDisplay();

        for(int16_t i=0; i<display.height()/2; i+=2) {
            display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
            display.display(); // Update screen with each newly-drawn rectangle
            delay(1);
        }

        delay(200);
    }

    void testfillrect() {
        display.clearDisplay();

        for(int16_t i=0; i<display.height()/2; i+=3) {
            // The INVERSE color is used so rectangles alternate white/black
            display.fillRect(i, i, display.width()-i*2, display.height()-i*2, SSD1306_INVERSE);
            display.display(); // Update screen with each newly-drawn rectangle
            delay(1);
        }

        delay(200);
    }

    void testdrawcircle() {
        display.clearDisplay();

        for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
            display.drawCircle(display.width()/2, display.height()/2, i, SSD1306_WHITE);
            display.display();
            delay(1);
        }

        delay(200);
    }

    void testfillcircle() {
        display.clearDisplay();

        for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
            // The INVERSE color is used so circles alternate white/black
            display.fillCircle(display.width() / 2, display.height() / 2, i, SSD1306_INVERSE);
            display.display(); // Update screen with each newly-drawn circle
            delay(1);
        }

        delay(200);
    }

    void testdrawroundrect() {
        display.clearDisplay();
        display.drawRoundRect(0, 0, display.width()-2, display.height()-2,
                              display.height()/8, SSD1306_WHITE);
        display.display();
        delay(200);
    }

    void testfillroundrect() {
        display.clearDisplay();

        for(int16_t i=0; i<display.height()/2-2; i+=2) {
            // The INVERSE color is used so round-rects alternate white/black
            display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
                                  display.height()/4, SSD1306_INVERSE);
            display.display();
            delay(1);
        }

        delay(200);
    }

    void testdrawtriangle() {
        display.clearDisplay();

        for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
            display.drawTriangle(
                    display.width()/2  , display.height()/2-i,
                    display.width()/2-i, display.height()/2+i,
                    display.width()/2+i, display.height()/2+i, SSD1306_WHITE);
            display.display();
            delay(1);
        }

        delay(200);
    }

    void testfilltriangle() {
        display.clearDisplay();

        for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
            // The INVERSE color is used so triangles alternate white/black
            display.fillTriangle(
                    display.width()/2  , display.height()/2-i,
                    display.width()/2-i, display.height()/2+i,
                    display.width()/2+i, display.height()/2+i, SSD1306_INVERSE);
            display.display();
            delay(1);
        }

        delay(200);
    }

    void testdrawchar() {
        display.clearDisplay();

        display.setTextSize(1);      // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE); // Draw white text
        display.setCursor(0, 0);     // Start at top-left corner
        display.cp437(true);         // Use full 256 char 'Code Page 437' font

        // Not all the characters will fit on the display. This is normal.
        // Library will draw what it can and the rest will be clipped.
        for(int16_t i=0; i<256; i++) {
            if(i == '\n') display.write(' ');
            else          display.write(i);
        }

        display.display();
        delay(1000);
    }

    void testdrawstyles() {
        display.clearDisplay();

        display.setTextSize(2);             // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE);        // Draw white text
        display.setCursor(0,0);             // Start at top-left corner
        display.println(F("Hellow, Pesto!"));

        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
        display.println(1337);

        display.setTextSize(2);             // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.print(F("0x")); display.println(0xDEADBEEF, HEX);

        display.display();
        delay(2000);
    }

    void testscrolltext() {
        display.clearDisplay();

        display.setTextSize(2); // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 0);
        display.println(F("PESTO?"));
        display.display();      // Show initial text
        delay(1000);
    #ifdef USE_SMALL_DISPLAY
        // Scroll in various directions, pausing in-between:
                display.startscrollright(0x00, 0x0F);
                display.stopscroll();
                delay(100);
                display.startscrollleft(0x00, 0x0F);
                display.stopscroll();
                delay(100);
                display.startscrolldiagright(0x00, 0x07);
                display.startscrolldiagleft(0x00, 0x07);
                display.stopscroll();
    #endif
    }

    void testdrawbitmap() {
        display.clearDisplay();

        display.drawBitmap(
                (display.width()  - LOGO_WIDTH ) / 2,
                (display.height() - LOGO_HEIGHT) / 2,
                logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
        display.display();
        delay(100);
    }

    #define XPOS   0 // Indexes into the 'icons' array in function below
    #define YPOS   1
    #define DELTAY 2

    void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
        int8_t f, icons[NUMFLAKES][3];

        // Initialize 'snowflake' positions
        for(f=0; f< NUMFLAKES; f++) {
            icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
            icons[f][YPOS]   = -LOGO_HEIGHT;
            icons[f][DELTAY] = random(1, 6);
        }

        for(f=0; f< 25; f++) { // Loop 25...
            display.clearDisplay(); // Clear the display buffer

            // Draw each snowflake:
            for(f=0; f< NUMFLAKES; f++) {
                display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, SSD1306_WHITE);
            }

            display.display(); // Show the display buffer on the screen
            delay(40);        // Pause for 1/10 second

            // Then update coordinates of each flake...
            for(f=0; f< NUMFLAKES; f++) {
                icons[f][YPOS] += icons[f][DELTAY];
                // If snowflake is off the bottom of the screen...
                if (icons[f][YPOS] >= display.height()) {
                    // Reinitialize to a random position, just off the top
                    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
                    icons[f][YPOS]   = -LOGO_HEIGHT;
                    icons[f][DELTAY] = random(1, 6);
                }
            }
        }
    }

#endif


void displaySetup() {
#ifdef USE_MOUTH_DISPLAY_ADAFRUIT

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {

    } else {

        // Show initial display buffer contents on the screen --
        // the library initializes this with an Adafruit splash screen.
        display.display();
        delay(500); // Pause for 2 seconds

        testdrawline();      // Draw many lines

        testdrawrect();      // Draw rectangles (outlines)

        testfillrect();      // Draw rectangles (filled)

        testdrawcircle();    // Draw circles (outlines)

        testfillcircle();    // Draw circles (filled)

        testdrawroundrect(); // Draw rounded rectangles (outlines)

        testfillroundrect(); // Draw rounded rectangles (filled)

        testdrawtriangle();  // Draw triangles (outlines)

        testfilltriangle();  // Draw triangles (filled)

        testdrawchar();      // Draw characters of the default font

        testdrawstyles();    // Draw 'stylized' characters

        testscrolltext();    // Draw scrolling text

        testdrawbitmap();    // Draw a small bitmap image

        // Invert and restore display, pausing in-between
        display.invertDisplay(true);
        delay(1000);
        display.invertDisplay(false);
        delay(1000);

        //testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps
    }
#endif
}

/************************************************ Display u8g2  *************************************************/
// section Display u8g2
/***************************************************************************************************************/

#ifdef USE_MOUTH_DISPLAY_U8G2
    void u8g2_prepare() {
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

    void draw() {
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
#endif


/***************************************************** Functions s**********************************************/
// section Timer Functions
/***************************************************************************************************************/


void dotMatrixTimer(){
    pestoMatrix();
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


/***************************************************** Servo PWM Angle s**********************************************/
// section Servo PWM Angle
/***************************************************************************************************************/

int pulseWidth(int angle){  //  pwm.setPWM(PWM_0, 0, pulseWidth(0));
    int pulse_wide, analog_value;
    pulse_wide   = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
    analog_value = int(float(pulse_wide) / 1000000 * FREQUENCY * 4096);
    return analog_value;
}

/***************************************************** Servo PWM Angle s**********************************************/
// section RGBled
/***************************************************************************************************************/

void RGBled(int r_val, int g_val, int b_val) {
    pwm.setPWM(PWM_8, 0, (16*b_val<4080) ? 16*b_val : 4080);
    pwm.setPWM(PWM_9, 0, (16*g_val<4080) ? 16*g_val : 4080);
    pwm.setPWM(PWM_10, 0, (16*r_val<4080) ? 16*r_val : 4080);
}

void defaultLCD(){
    lcd.setCursor(0,top);
    lcd.write(0); /********** heart **********/
    lcd.setCursor(2,top);   //Set cursor to character 2 on line 0
    lcd.print("Hello Pesto!");
    lcd.setCursor(0,bot);   //Set cursor to line 1
    lcd.print(previousIR, HEX);
}


/********************************************** Main loop running the arduino **********************************/
// section webserver
/***************************************************************************************************************/

void printWifiStatus() {
    lcd.clear();
    // print your board's IP address:
    lcd.setCursor(0, top);   //Set cursor to line 0
    lcd.print(WiFi.localIP());

    // print the received signal strength:
    lcd.setCursor(0,bot);   //Set cursor to line 1
    lcd.print("signal: ");
    lcd.print(WiFi.RSSI());
    lcd.print(" dBm");
    delay(1000);
}


void setupWifi(){
    lcd.clear();
    lcd.setCursor(0,top);
    lcd.print("WiFi Starting!");

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        lcd.clear();
        lcd.setCursor(2,top);
        lcd.print("Please upgrade the firmware");
    }

    lcd.setCursor(0,bot);
    lcd.println(ssid);
    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
        lcd.clear();
        lcd.setCursor(2,top);
        lcd.print(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 4 seconds for connection:
        delay(4000);
    }
    server.begin();
    // you're connected now, so print out the status:
    printWifiStatus();

}
void webserver(){
    WiFiClient client = server.available();
    if (client) {
        String HTTP_req = ""; // read the first line of HTTP request header
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
        String html = String(HTML_CONTENT);
        html.replace("TEMPERATURE_MARKER", String(temperature, 2)); // replace the marker by a real value
        client.println(html);
        client.flush();

        // give the web browser time to receive the data
        delay(10);

        // close the connection:
        client.stop();
    }
}
/********************************************** Setup booting the arduino **************************************/
// section Setup
/***************************************************************************************************************/

void setup(){
    matrix.loadSequence(animation);
    matrix.begin();
    matrix.autoscroll(300);
    matrix.play(true);

    Wire.begin();
    lcd.init();
    lcd.backlight();      // Make sure backlight is on
    lcd.createChar(0, Heart); // create a new characters
    lcd.clear();
    lcd.setCursor(0,top);
    bluetoothSetup();

    pinMode(Trig_PIN, OUTPUT);    /***** 6 ******/
    pinMode(Echo_PIN, INPUT);     /***** 7 ******/
    pinMode(ML_Ctrl, OUTPUT);     /***** 13 ******/
    pinMode(ML_PWM, OUTPUT);      /***** 11 ******/
    pinMode(MR_Ctrl, OUTPUT);     /***** 12 ******/
    pinMode(MR_PWM, OUTPUT);      /***** 3 ******/
    digitalWrite(ML_Ctrl, HIGH);
    digitalWrite(MR_Ctrl, HIGH);


    pinMode(LED_PIN, OUTPUT);     /***** 2 ******/
    pinMode(MIC_PIN, INPUT);


    pinMode(DotClockPIN,OUTPUT);/***** 5 ******/
    pinMode(DotDataPIN,OUTPUT); /***** 4 ******/

    baseSound = map(analogRead(MIC_PIN), 0, 1023, 0, 255);

    digitalWrite(DotClockPIN,LOW);
    digitalWrite(DotDataPIN,LOW);
    matrix_display(clear);
    pestoMatrix();

    I2CScanner();
    delay(500);

    lcd.clear();

    gyroSetup();

    lcd.clear();

    compassSetup();

    lcd.clear();

    baroSetup();

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(IR_Pin, ENABLE_LED_FEEDBACK);
    lcd.print("InfraRed remote");

    pwm.begin();
    pwm.setPWMFreq(FREQUENCY);  // Analog servos run at ~50 Hz updates
    pwm.setPWM(PWM_0, 0, pulseWidth(posXY));
    pwm.setPWM(PWM_1, 0, pulseWidth(posZ));
    delay(500);
    #ifdef USE_MOUTH_DISPLAY_U8G2
        u8g2.begin();
    #endif

    #ifdef USE_MOUTH_DISPLAY_ADAFRUIT
        displaySetup();
    #else
        lcd.clear();
        lcd.setCursor(0,top);
        lcd.print("Not using the");
        lcd.setCursor(0,bot);
        lcd.print(" 128x64 Adafruit");
    #endif

    #ifdef USE_WIFI_SERVER
        setupWifi();
    #else
        lcd.clear();
        lcd.setCursor(0,top);
        lcd.print("Not using the");
        lcd.setCursor(0,bot);
        lcd.print(" WIFI server");
    #endif


    timerButton = Rem_OK;
    timerOne.set(timerOnePeriod, dotMatrixTimer);
    timerTwo.set(timerTwoPeriod, sensorTimer);
    timerThree.set(timerThreePeriod, resetTimers);

    lcd.clear();
}

/*********************************** Loop **********************************/
// section Loop
/***************************************************************************/

void loop(){
    #ifdef USE_WIFI_SERVER
        webserver();
    #endif


    #ifdef USE_JOYSTICK
        joystick();
    #endif

    #ifdef USE_MOUTH_DISPLAY_U8G2
        // picture loop
        u8g2.firstPage();
        do {
            draw();
        } while( u8g2.nextPage() );

        // increase the state
        draw_state++;
        if ( draw_state >= 14*8 )
            draw_state = 0;

        // delay between each page
        delay(150);
    #endif
    Usb.Task();

    if (PS4.connected()) {
        ps4();
    }
    bluetoothListener();
    bluetoothSerial();

    /***************************** IrReceiver **********************************/
    // section Loop IrReceiver
    /***************************************************************************/

    if (IrReceiver.decode()) {  // Grab an IR code   At 115200 baud, printing takes 200 ms for NEC protocol and 70 ms for NEC repeat
        if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {         // Check if the buffer overflowed
            lcd.clear();
            lcd.setCursor(0,top);
            lcd.println(F("Try to increase the \"RAW_BUFFER_LENGTH\" value of " STR(RAW_BUFFER_LENGTH) " in " __FILE__));
            delay(100);
        } else {
            lcd.clear();
            if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
                ir_rec = previousIR;
                lcd.setCursor(0,top);
                lcd.print(F("?"));
                delay(100);
            } else {
                ir_rec = IrReceiver.decodedIRData.decodedRawData;
            }
            lcd.setCursor(0,bot);
            lcd.print(ir_rec, HEX);
        }
        IrReceiver.resume();                            // Prepare for the next value

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
            case Rem_OK:
                Car_Stop(); break;
            case Rem_U:
                Car_front(); break;
            case Rem_D:
                Car_Back(); break;
            case Rem_L:
                Car_left(); break;
            case Rem_R:
                Car_right(); break;
            default:
                break;
        }
        if (posXY != previousXY) {
            pwm.setPWM(PWM_0, 0, pulseWidth(posXY));
        }
        if (posZ != previousZ) {
            pwm.setPWM(PWM_1, 0, pulseWidth(posZ));
        }
        previousIR = ir_rec;
        previousXY = posXY;
        previousZ = posZ;
    }

    gyroDetectMovement();
    timerOne.update();
    timerTwo.update();
    timerThree.update();

    Car_Stop();
    distanceF = checkDistance();  /// assign the front distance detected by ultrasonic sensor to variable a
    if (distanceF < 35) {
        RGBled(230, 0, 0);
        pestoMatrix();
    } else {
        int micStatus = analogRead(MIC_PIN);
        int mic255 = map(micStatus, 0, 1023, 0, 255);

        if (mic255 > baseSound) {
            RGBled(mic255, 0, mic255);
        } else {
            RGBled(0, mic255, 0);
        }
        defaultLCD();
        RGBled(0, 0, lightSensor());
    }
}

