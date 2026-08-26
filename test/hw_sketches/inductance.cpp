/*
#include <Arduino.h>

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize LCD


//double pulse, frequency, capacitance, inductance;

const int inputPin = 8;  // Input from the comparator output
const int triggerPin = 9; // Output to the LC circuit

double pulse, frequency, inductance_uH, inductance_mH, inductance;
const double capacitance = 1.E-6;  // Capacitance value (adjust as needed)


void setup() {

    Serial.begin(115200);
    pinMode(inputPin, INPUT_PULLUP);
    pinMode(triggerPin, OUTPUT);

    lcd.init();  // Initialize LCD
    lcd.backlight();  // Turn on backlight

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("booting");
    lcd.setCursor(0, 1);
    delay(500);



}

void loop() {

    digitalWrite(triggerPin, HIGH); // Trigger the LC circuit
    delay(5); // Allow time for the inductor to charge
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(5); // Ensure resonation is measured
    pulse = pulseIn(inputPin, HIGH, 5000); // Measure pulse width (returns 0 if timeout)

    if (pulse > 0.1) { // If a timeout did not occur and a reading was taken
        frequency = 1.E6 / (2 * pulse); // Calculate frequency in Hz
        inductance = 1. / (capacitance * frequency * frequency * 4. * 3.14159 * 3.14159); // Calculate inductance in microhenries
        inductance_uH = 1E6 * inductance;
        inductance_mH = inductance_uH / 1000; // Convert inductance to millihenries

        // Serial print
        Serial.print("High for uS:");
        Serial.print(pulse);
        Serial.print("\tFrequency Hz:");
        Serial.print(frequency);
        Serial.print("\tInductance uH:");
        Serial.print(inductance_uH);
        Serial.print("\tInductance mH:");
        Serial.println(inductance_mH);


        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("uH Inductance mH");
        lcd.setCursor(0, 1);
        lcd.print(inductance_uH);
        lcd.setCursor(10, 1);
        lcd.print(inductance_mH);
    }
    delay(1000); // Allow time for the inductor to charge
}

#include<LiquidCrystal.h>
LiquidCrystal lcd(A5, A4, A3, A2, A1, A0);

#define serial

#define charge 3
#define freqIn 2
#define mode 10

#define Delay 15

double frequency, capacitance, inductance;

typedef struct
{
    int flag: 1;
}Flag;

Flag Bit;

void setup()
{
#ifdef serial
    Serial.begin(9600);
#endif
    lcd.begin(16, 2);
    pinMode(freqIn, INPUT);
    pinMode(charge, OUTPUT);
    pinMode(mode, INPUT_PULLUP);
    lcd.print(" LC Meter Using ");
    lcd.setCursor(0, 1);
    lcd.print("     Arduino    ");
    delay(2000);
    lcd.clear();
    lcd.print("Circuit Digest");
    delay(2000);
}

void loop()
{
    for(int i=0;i<Delay;i++)
    {
        digitalWrite(charge, HIGH);
        delayMicroseconds(100);
        digitalWrite(charge, LOW);
        delayMicroseconds(50);
        double Pulse = pulseIn(freqIn, HIGH, 10000);
        if (Pulse > 0.1)
            frequency+= 1.E6 / (2 * Pulse);
        delay(20);
    }
    frequency/=Delay;
#ifdef serial
    Serial.print("frequency:");
    Serial.print( frequency );
    Serial.print(" Hz     ");
#endif

    lcd.setCursor(0, 0);
    lcd.print("freq:");
    lcd.print( frequency );
    lcd.print(" Hz      ");

    if (Bit.flag)
    {
        inductance = 1.E-3;
        capacitance = ((1. / (inductance * frequency * frequency * 4.*3.14159 * 3.14159)) * 1.E9);
        if((int)capacitance < 0)
            capacitance=0;
#ifdef serial
        Serial.print("Capacitance:");
        Serial.print( capacitance,6);
        Serial.println(" uF   ");
#endif
        lcd.setCursor(0, 1);
        lcd.print("Cap: ");
        if(capacitance > 47)
        {
            lcd.print( (capacitance/1000));
            lcd.print(" uF                 ");
        }
        else
        {
            lcd.print(capacitance);
            lcd.print(" nF                 ");
        }
    }

    else
    {
        capacitance = 0.1E-6;
        inductance = (1. / (capacitance * frequency * frequency * 4.*3.14159 * 3.14159)) * 1.E6;
#ifdef serial
        Serial.print("Ind:");
        if(inductance>=1000)
        {
            Serial.print( inductance/1000 );
            Serial.println(" mH");
        }
        else
        {
            Serial.print( inductance );
            Serial.println(" uH");
        }
#endif

        lcd.setCursor(0, 1);
        lcd.print("Ind:");
        if(inductance>=1000)
        {
            lcd.print( inductance/1000 );
            lcd.print(" mH            ");
        }
        else
        {
            lcd.print( inductance );
            lcd.print(" uH              ");
        }
    }

    if (digitalRead(mode) == LOW)
    {
        Bit.flag = !Bit.flag;
        delay(1000);
        while (digitalRead(mode) == LOW);
    }
    delay(50);
}

//import libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>      // in that project i use I2C protocol to interface lcd with arduino
// if you don't use I2C the simply import LiquidCrystal lib. and use
// traditional connections of lcd with arduino
//initiallize the Lcd
LiquidCrystal_I2C lcd(0x3f,16,2);  //sometimes the adress is not 0x3f. Change to 0x27 if it dosn't work.

//define GPIO pins to be used
#define pulse_in 3                // pin 3 is used to capture pulse signals from external circuit's output
#define pulse_out 12              // pin 12 is used to send pulse signal as external circuit's input
//initiallize some global variabels
double pulse ;
double frequency ;                    // measure frequency using the pulse as in usec use 1.E6
double inductance ;
double mH_induct ;
double cap = 1.0E-6 ;                // to find inductance capacitance must be fixed i use 1uF capatance in proj.
// if you use another capacitor , change the value of cap in code

void setup()
{
    Serial.begin(9600);           // Serial communication for serial print of the output values
    lcd.begin();                  // lcd communication for output display
    lcd.backlight();              // make the lcd light On
    //initiallize GPIO pins
    pinMode(pulse_in , INPUT);    // output of lc circuit as input to arduino
    pinMode(pulse_out , OUTPUT);  // output of arduino to input of lc circuit
    // serial and lcd printing
    Serial.println("INDUCTANCE METER powered by Arduino");
    delay(1000);
}

void loop()
{
    // generation of pulse for 5 ms through pulse_out pin
    digitalWrite(pulse_out, HIGH);
    delay(5);                                    //time to charge inductor.
    digitalWrite(pulse_out,LOW);

    delayMicroseconds(100);                     //make sure resination is measured
    // capturing of pulse from lc external ckt
    pulse = pulseIn(pulse_in,HIGH,5000);        //checking the High pulse upto 5 msec to measure & returns 0 if timeout

    //if a timeout did not occur and it took a reading
    if(pulse > 0.1)
    {
        frequency = 1.E6/(2*pulse) ;
        inductance = 1./(4*cap*pow(frequency , 2 )*pow(3.14159 , 2));
        inductance *= 1.0E6;                    //inductance = inductance*1E6
        if(inductance>=1000)                 // if inductor is present
        {
            serialPrint_1();
            lcdPrint_1();
        }
        else
        {
            serialPrint_2();
            lcdPrint_2();
        }
    }
    else if(pulse < 0.1)                     //if no inductor present
    {
        serialPrint_3();
        lcdPrint_3();
    }
}

// function for serial print
void serialPrint_1()
{
    Serial.print("High for uS:");
    Serial.print( pulse );
    Serial.print("\tfrequency Hz:");
    Serial.print( frequency );
    Serial.print("\tinductance mH:");
    Serial.println( inductance/1000 );
}

void serialPrint_2()
{
    Serial.print("High for uS:");
    Serial.print( pulse );
    Serial.print("\tfrequency Hz:");
    Serial.print( frequency );
    Serial.print("\tinductance uH:");
    Serial.println(inductance);
}

void serialPrint_3()
{
    Serial.println("Insert Inductor");
}

// function for lcd print
void lcdPrint_1()
{
    lcd.setCursor(0,0);           // initiallize 1st line of lcd by printing "INDUCTANCE METER"
    lcd.print("INDUCTANCE METER");
    lcd.setCursor(5 , 1);
    lcd.print ((inductance/1000));
    lcd.print ("mH" );
    delay(500);
    lcd.clear();
}

void lcdPrint_2()
{
    lcd.setCursor(0,0);           // initiallize 1st line of lcd by printing "INDUCTANCE METER"
    lcd.print("INDUCTANCE METER");
    lcd.setCursor(5 , 1);
    lcd.print (inductance);
    lcd.print ("uH" );
    delay(500);
    lcd.clear();
}

void lcdPrint_3()
{
    lcd.setCursor(0,0);           // initiallize 1st line of lcd by printing "INDUCTANCE METER"
    lcd.print("INDUCTANCE METER");
    lcd.setCursor(0,1);
    lcd.print("insert Inductor");
}*/
