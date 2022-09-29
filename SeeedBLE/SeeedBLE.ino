/*****************************************************************************
 * Bluetooth Low Energy (BLE) Sensor Data Display Code
 * 
 * Controls a Xiao Seeed Sense BLE interfacing a sensor with both serial and analog output.
 * Uses on-board three color LED to signify operational status.
 * 
 * Requirements:
 * https://www.seeedstudio.com/Seeed-XIAO-BLE-Sense-nRF52840-p-5253.html
 * Include the following link in preferences: 
 * https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
 * Install the following library from the manager:
 * "ArduinoBLE.h"
 */

#include <ArduinoBLE.h>
 
BLEService carbonMonoxide("181A"); // Bluetooth® Low Energy - Environmental Sensing
 
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
// BLEByteCharacteristic switchCharacteristic("2BD1", BLERead | BLEWrite);
BLEByteCharacteristic coAnalog("19b10001-e8f4-537e-4f6c-d104768a1214", BLERead | BLENotify);
// BLEByteCharacteristic coMonitor("19b10001-e8f5-537e-4f6c-d104768a1214", BLERead | BLENotify);
 
//const int ledRed = LED_BUILTIN; // pin to use for the LED
//const int ledBlue = LEDG;
//const int ledGreen = LEDB;

//int bytes;
//char serialBuffer [9] = {0xFF,0x01,0x78,0x03,0x00,0x00,0x00,0x00,0x84};

// Sensor must be on for 30 seconds
#define TIMER_WARMUP  30000
int statusReady = -1;

int timerSample = 1;
int timerBreath = 15;
#define TIMER_OFF  30000
int timerReset = 0;

int coAnalogValue = 0;
float coTrend [5] = {0,0,0,0,0};

 
void setup() {
  //Serial.begin(9600);

  //Serial1.begin(9600);
  //Serial1.setTimeout(1000);
  //Serial1.write(serialBuffer,9);
 
  // set LED pin to output mode
  //pinMode(ledRed, OUTPUT);
  //pinMode(ledBlue, OUTPUT);
  //pinMode(ledGreen, OUTPUT);
  pinMode(A0, INPUT);
  //pinMode(D10, OUTPUT);
  //digitalWrite(D10, LOW); // Enable red LED for error
 
  // begin initialization
  if (!BLE.begin()) {
//    digitalWrite(ledRed, HIGH); // Enable red LED for error
    while (1);
  }
 
  // set advertised local name and service UUID:
  BLE.setLocalName("Univ. Oklahoma NPL CO Monitor");
  BLE.setAdvertisedService(carbonMonoxide);
 
  // add all the characteristic to the service
  //carbonMonoxide.addCharacteristic(switchCharacteristic);
  carbonMonoxide.addCharacteristic(coAnalog);
  //carbonMonoxide.addCharacteristic(coMonitor);
 
  // add service
  BLE.addService(carbonMonoxide);
 
  // set the initial value for the characeristic:
  //switchCharacteristic.writeValue(0);
 
  // start advertising
  BLE.advertise();  
  //digitalWrite(ledGreen, HIGH); // Enable red LED for error
  //digitalWrite(ledRed, LOW); // Enable red LED for error

  
  //digitalWrite(D10, HIGH); // Enable red LED for error
  //Serial.println("BLE LED Peripheral");
}
 
void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();
  // Read the most current value of CO sensor
  coAnalogValue = map(analogRead(A0),185,931,0,1000);

  if (statusReady < 0){
    if (millis() > TIMER_WARMUP){
      statusReady = 0;
      //digitalWrite(ledGreen, HIGH); // Enable red LED for error
      //digitalWrite(ledRed, HIGH); // Enable red LED for error
    }
  }
  else if (statusReady <= 1){
    if (coAnalogValue > 2){
      statusReady = 1;
      //digitalWrite(ledBlue, HIGH); // Enable red LED for error
      //digitalWrite(ledGreen, LOW); // Enable red LED for error
      //digitalWrite(ledRed, HIGH); // Enable red LED for error
    }
    else if (coAnalogValue < 2){
      statusReady = 0;
      //digitalWrite(ledBlue, LOW); // Enable red LED for error
      //digitalWrite(ledGreen, HIGH); // Enable red LED for error
      //digitalWrite(ledRed, HIGH); // Enable red LED for error
    }
  }

  if (millis() > TIMER_OFF){
    //digitalWrite(D10,LOW);
  }

  // Device is connected
  while (central.connected()) {
    //digitalWrite(D10, HIGH); // Enable red LED for error
        /*if (switchCharacteristic.written()) {
          if (switchCharacteristic.value()) {   
            //Serial.println("LED on");
            digitalWrite(ledRed, LOW); // changed from HIGH to LOW       
          } else {                              
            //Serial.println(F("LED off"));
            digitalWrite(ledRed, HIGH); // changed from LOW to HIGH     
          }
        }*/
        coAnalogValue = analogRead(A0);
        coTrend[0] = map(coAnalogValue,185,931,0,1000);
        delay(100);
        coAnalogValue = analogRead(A0);
        coTrend[1] = map(coAnalogValue,185,931,0,1000);
        delay(100);
        coAnalogValue = analogRead(A0);
        coTrend[2] = map(coAnalogValue,185,931,0,1000);
        delay(100);
        coAnalogValue = analogRead(A0);
        coTrend[3] = map(coAnalogValue,185,931,0,1000);
        delay(100);
        coAnalogValue = analogRead(A0);
        coTrend[4] = map(coAnalogValue,185,931,0,1000);
        delay(100);
        coAnalogValue = (coTrend[0] + coTrend[1] + coTrend[2] + coTrend[3] + coTrend[4])/5;
        //coAnalog.writeValue(map(coAnalogValue,185,931,0,1000));
        coAnalog.writeValue(coAnalogValue);
        /*if (bytes > 0){
          coMonitor.writeValue(serialBuffer[2]*256+serialBuffer[3]);
          Serial1.write(int(serialBuffer[3]));
        }
        else{
          coMonitor.writeValue(0xFF);
        }*/
      }
  //digitalWrite(D10, LOW); // Enable red LED for error
    // when the central disconnects, print it out:
    
}
