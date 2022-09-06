#include <ArduinoBLE.h>
 
BLEService carbonMonoxide("19b10000-e8f2-537e-4f6c-d104768a1214"); // Bluetooth® Low Energy LED Service
 
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("19b10000-e8f3-537e-4f6c-d104768a1214", BLERead | BLEWrite);
BLEByteCharacteristic coAnalog("19b10000-e8f4-537e-4f6c-d104768a1214", BLERead);
BLEByteCharacteristic coMonitor("19b10000-e8f5-537e-4f6c-d104768a1214", BLERead);
 
const int ledPin = LED_BUILTIN; // pin to use for the LED
char serialBuffer [9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


int timerWarmup = 30;
int timerSample = 1;
int timerBreath = 15;
int timerReset = 45;
 
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
 
  // set LED pin to output mode
  pinMode(ledPin, OUTPUT);
  pinMode(A0, INPUT);
 
  // begin initialization
  if (!BLE.begin()) {
    //Serial.println("starting Bluetooth® Low Energy module failed!");
 
    while (1);
  }
 
  // set advertised local name and service UUID:
  BLE.setLocalName("Univ. Oklahoma NPL CO Monitor");
  BLE.setAdvertisedService(carbonMonoxide);
 
  // add all the characteristic to the service
  carbonMonoxide.addCharacteristic(switchCharacteristic);
  carbonMonoxide.addCharacteristic(coAnalog);
  carbonMonoxide.addCharacteristic(coMonitor);
 
  // add service
  BLE.addService(carbonMonoxide);
 
  // set the initial value for the characeristic:
  switchCharacteristic.writeValue(0);
 
  // start advertising
  BLE.advertise();
 
  //Serial.println("BLE LED Peripheral");
}
 
void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();
 
  // if a central is connected to peripheral:
  if (central) {
    //Serial.print("Connected to central: ");
    // print the central's MAC address:
    //Serial.println(central.address());
 
    // while the central is still connected to peripheral:
  while (central.connected()) {
        if (switchCharacteristic.written()) {
          if (switchCharacteristic.value()) {   
            //Serial.println("LED on");
            digitalWrite(ledPin, LOW); // changed from HIGH to LOW       
          } else {                              
            //Serial.println(F("LED off"));
            digitalWrite(ledPin, HIGH); // changed from LOW to HIGH     
          }
        }
        int coAnalogValue = analogRead(A0);
        coAnalog.writeValue(map(coAnalogValue,185,931,0,1000));
        int bytes = Serial.readBytes(serialBuffer,9);
        coMonitor.writeValue(serialBuffer[2]*256+serialBuffer[3]);
      }
 
    // when the central disconnects, print it out:
    // Serial.print(F("Disconnected from central: "));
    // Serial.println(central.address());
  }
}
