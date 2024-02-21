#include <Arduino.h>
#include "EveryTimerB.h"
#include <U8g2lib.h>
#include <Encoder.h>
#include <EEPROM.h>
#include <max6675.h>
#define Timer1 TimerB2



// My Timer Function .........................................................................................

class TiggyTimer {
  private:
    unsigned long lastUpdate;  // when the last update happened
    unsigned long interval;  // interval between updates

  public:
    TiggyTimer(unsigned long interval) {
      this->interval = interval;
      this->lastUpdate = millis();
    }

    bool isReady() {
      unsigned long currentMillis = millis();

      if (currentMillis - this->lastUpdate > this->interval) {
        this->lastUpdate = currentMillis;
        return true;
      } else {
        return false;
      }
    }
};

// Create timers
TiggyTimer timer1(250); // 1/4 second
TiggyTimer timer2(500); // 1/2 second
TiggyTimer timer3(1000); // 1 second

// end timer function setup .............................................................................................


void updateDisplay();

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Encoder myEnc(2, 3);

long oldPosition = 0;
unsigned long previousMillis = 0;
unsigned long lastUpdate = 0;
long interval = 1000; // Default pulse interval in milliseconds
const int pulsePin = 8;
const int ledPin = 12;
const int buttonPin = 4;  // Button connected to digital pin 4
const int eepromAddress = 0;
double temp1;
double temp2;
unsigned long lastTempUpdate = 0;  // new variable to track last temperature update
long tempInterval = 5000;  // set interval to slightly higher than 220ms for safety
bool pulseEnabled = false;
unsigned long PulseCounter = 1;
int buttonState = 0;
bool isOperationInProgress = false;


int thermoDO1 = 11;
int thermoCS1 = 9;
int thermoCLK1 = 10;

int thermoDO2 = A2;
int thermoCS2 = A0;
int thermoCLK2 = A1;

MAX6675 thermocouple1;
MAX6675 thermocouple2;


void setup() {

  Timer1.initialize();
  Timer1.attachInterrupt(solenoidTime);
  Timer1.setPeriod(1000); 


  pinMode(pulsePin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Set button pin as input with pull-up resistor
  oled1.setI2CAddress(0x78);
  oled1.begin();
  oled2.setI2CAddress(0x7A);
  oled2.begin();

  thermocouple1.begin(thermoCLK1, thermoCS1, thermoDO1);
  thermocouple2.begin(thermoCLK2, thermoCS2, thermoDO2);

  // Load interval from EEPROM
  EEPROM.get(eepromAddress, interval);
  updateDisplay();


}

void solenoidTime (){ // o.5 second timer 
 

 if (pulseEnabled == true ){
        PulseCounter++;
  if (PulseCounter >= interval) {

 int pwmValue = interval; // Adjust this value between 0 (0% duty cycle) and 255 (100% duty cycle) as needed
      analogWrite(pulsePin, pwmValue);

     //digitalWrite(pulsePin, !digitalRead(pulsePin));
     //digitalWrite(ledPin, !digitalRead(ledPin));
 
    PulseCounter = 1;

 }
}
}


void updateDisplay() {
  
  oled1.clearBuffer();
  oled1.setFont(u8g2_font_t0_16_tf);
   oled1.drawStr(0, 15, "Waste Oil");
   oled1.setFont(u8g2_font_t0_16_tf);
   oled1.drawStr(0, 30, "   Controller ");
  oled1.drawStr(0, 60, "Fuel Pulse: ");
  oled1.setCursor(90, 60);
  oled1.print(interval);
  oled1.sendBuffer();

   oled2.clearBuffer();
    oled2.setFont(u8g2_font_ncenB14_tr);
    if (temp1 > 100 && temp2 > 100) {
      oled2.drawStr(0, 20, "Boiler ON");
      }
if (temp1 < 100 && temp2 < 100) {
      oled2.drawStr(0, 20, "Boiler OFF");
      }
if (isnan(temp1)) {
  oled2.drawStr(0, 20, "Sensor Fault");
}
if (isnan(temp2)) {
  oled2.drawStr(0, 20, "Sensor Fault");
}

    oled2.setFont(u8g2_font_t0_16_tf);
  //temp1 = thermocouple1.readCelsius();
  //temp2 = thermocouple2.readCelsius();
    oled2.setCursor(0, 40);
    oled2.print("Flu Temp: ");
    
    if (isnan (temp1) ) {
  oled2.print("----");
  
}
else
 {
  oled2.print(temp1);
  }

    oled2.setCursor(0, 60);
    oled2.print("Fire Box: ");
     if (isnan (temp2 )) {
  oled2.print("----");
  //oled2.print (buttonState);
  
}
else
{
   oled2.print(temp2);
   }
    
    
    oled2.sendBuffer();
  
}

void loop() {

 buttonState = digitalRead(buttonPin);  // Read the state of the button  
if (timer1.isReady()) { // update the display at 1/4 of a second.

if (!isOperationInProgress) {
    // Set flag to indicate an operation is in progress
    isOperationInProgress = true;

updateDisplay(); 

    // Clear the flag
    isOperationInProgress = false;
  }

 
}
  long newPosition = myEnc.read() / 4;
  unsigned long currentMillis = millis();
  // Update temperatures based on time interval
  if (timer2.isReady()) { // update the sensor at 1 second.
    temp1 = thermocouple1.readCelsius();
    temp2 = thermocouple2.readCelsius();
  }

  if (newPosition != oldPosition) {
    interval += (newPosition - oldPosition) * 1;
    if (interval <= 1) interval = 1;
    if (interval >= 255) interval = 255;
    oldPosition = newPosition;
    EEPROM.put(eepromAddress, interval);
      // Update temperature readings here
    
  }


  if ((temp1 > 400 && temp2 > 300) || buttonState == LOW) {  // Either both temps > 501 and 351 or button pressed
    pulseEnabled = true;
  }
  

 
 if (buttonState == HIGH) {
       if (isnan(temp1) || isnan(temp2) || temp1 < 400 || temp2 < 300) {
         PulseCounter = 1;
        pulseEnabled = false;
        analogWrite(pulsePin, 0); // This turns the PWM off
        //digitalWrite(pulsePin, LOW);
        //digitalWrite(ledPin, HIGH );
}


 }

 
}


