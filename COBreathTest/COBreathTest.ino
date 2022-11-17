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
#include "COData.h"
 
BLEService carbonMonoxide("181A"); // Bluetooth® Low Energy - Environmental Sensing
 
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEWordCharacteristic coConcentration("2BD0", BLERead | BLENotify);
BLEByteCharacteristic statusByte("2BBB", BLERead | BLENotify);
BLEByteCharacteristic commandByte("2A9F", BLEWrite);
BLEWordCharacteristic coMax("2AF4", BLERead | BLENotify);

// Status flags for system states and performance
#define STATUS_BIT_START        0b10000000
#define STATUS_BIT_BKG_ZERO     0b01000000
#define STATUS_BIT_BKG_OFFSET   0b00100000
#define STATUS_BIT_CONNECTED    0b00010000
#define STATUS_BIT_TEST_NOW     0b00000001
#define STATUS_BIT_TEST_DONE    0b00000010
#define STATUS_BIT_TEST_GOOD    0b00000100
#define STATUS_BIT_TEST_END     0b00001000
int statusReady = 0;

// Sensor timing for different operation modes
// Sensor data must be stable for 60 seconds before enabling test
#define TIMER_WARMUP  60000
#define TIMER_TEST 15000
#define TIMER_OFF  30000
int timerSample = 1;
int timerStart = 0;
int timerEnd = 0;

#define CAL_VOLT_ZERO   185
#define CAL_VOLT_MAX    931

// Data class for sensor measurement at different times of operation
COData COValue;
float coAnalogValue = 0;
float coBkgPretest = 0;
float coBkgConnect = 0;
float coBkgPosttest = 0;

int testMax = 0;

void setup() {
  
  // One analog voltage signal
  pinMode(ANALOG_INPUT, INPUT);
   
  // set advertised local name and service UUID:
  BLE.begin();
  BLE.setLocalName("Univ. Oklahoma NPL CO Monitor");
  BLE.setAdvertisedService(carbonMonoxide);
 
  // add all the characteristic to the service
  carbonMonoxide.addCharacteristic(coConcentration);
  carbonMonoxide.addCharacteristic(statusByte);
  carbonMonoxide.addCharacteristic(commandByte);
  carbonMonoxide.addCharacteristic(coMax);
 
  // add service
  BLE.addService(carbonMonoxide);
 
  // set the initial value for the characeristic:
  statusByte.writeValue(statusReady);
  coConcentration.writeValue(0);
  coMax.writeValue(0);
 
  // start advertising
  BLE.advertise();

  modeMin();
}
 
void loop() {
  
  BLEDevice central = BLE.central();      // Check for connection
  coBkgPretest = COValue.readBkg();       // Read the sensor value

  checkStatus(coBkgPretest);                          // Check device readiness

  if (millis() > TIMER_OFF){
    //digitalWrite(D10,LOW);
  }

  // Device is connected
  while (central.connected()) {
    statusReady |= STATUS_BIT_CONNECTED;
    // Check device readiness: must be on for more time than WARMUP
    if (checkStatus(coBkgConnect)){
      statusByte.writeValue(statusReady);
    }
    
    // Check for external commands
    if (commandByte.written()) {
      // Identify the command: x01 => start test timer
      if (commandByte.value() & 0b00000001) {   
        // Clear the test status bits and previous results
        statusReady |= STATUS_BIT_TEST_NOW;
        testMax = 0;
        timerStart = millis();
        statusByte.writeValue(statusReady);
        coMax.writeValue(testMax);
      }
      else if (commandByte.value() & 0b00000010){
        // Clear the result and prepare for a new test
        statusReady &= 0b11110000;
        testMax = 0;
        statusByte.writeValue(statusReady);
        coMax.writeValue(testMax);
      }
      else {                              
        statusReady = statusReady | STATUS_BIT_TEST_NOW;     
      }
    }

    // Check for data analysis, 3 states: pre, test, post
    if (!(statusReady & 0b00000011)){
      coBkgConnect = COValue.readBkg();
      coConcentration.writeValue(map(coBkgConnect,CAL_VOLT_ZERO,CAL_VOLT_MAX,0,1000));
    }
    else if ((statusReady & 0b00000001) && !(statusReady & 0b00000010)){
      coAnalogValue = map(COValue.readSensor() + (CAL_VOLT_ZERO - COValue.getBkg()),CAL_VOLT_ZERO,CAL_VOLT_MAX,0,1000);
      coConcentration.writeValue(coAnalogValue);
      if (coAnalogValue > testMax){
        testMax = coAnalogValue;
        coMax.writeValue(testMax);
      }
      if ((millis() - timerStart) > TIMER_TEST){
        statusReady |= STATUS_BIT_TEST_DONE;
        statusByte.writeValue(statusReady);
        timerEnd = millis();
      }          
    }
    else {
      coBkgPosttest = COValue.readBkg();
      coConcentration.writeValue(map(coBkgPosttest,CAL_VOLT_ZERO,CAL_VOLT_MAX,0,1000));
      if (millis() - timerEnd > TIMER_TEST){
        statusReady |= STATUS_BIT_TEST_END;
        if (abs(coBkgConnect - coBkgPosttest) < 2){
          statusReady |= STATUS_BIT_TEST_GOOD;
        }
        statusReady &= 0b11111100;
        statusByte.writeValue(statusReady);
      }
    }
        
  }
  if (statusReady & STATUS_BIT_CONNECTED){
    statusReady &= !STATUS_BIT_CONNECTED;
    //modeSleep();
  }
    
}

// Verify the current data value, must be on for more time than WARMUP
int checkStatus(float dataValue){
  int previousStatus = statusReady;
  if (millis() > TIMER_WARMUP){           // Check device "ON" time
      statusReady |= STATUS_BIT_START;
  }
  
  if (abs(dataValue - CAL_VOLT_ZERO) > 2){             // Check magnitude of signal
    statusReady |= STATUS_BIT_BKG_ZERO;
  }
  else {
    statusReady &= 0b10111111;
  }
  if ((dataValue - CAL_VOLT_ZERO) < 0){                  // Check for DC voltage drift
    statusReady |= STATUS_BIT_BKG_OFFSET;
  }
  else {
    statusReady &= 0b11011111;
  }
  if (previousStatus != statusReady){
    return 1;
  }
  else{
    return 0;
  }
}

// Manually disable all parts of the microcontroller
void modeSleep(){
  NRF_POWER->SYSTEMOFF = 1;
}

void modeMin(){
  NRF_UART0 ->TASKS_STOPTX = 1;
  NRF_UART0 ->TASKS_STOPRX = 1;
  NRF_UART0 ->ENABLE = 0;
  NRF_UARTE0->ENABLE = 0;  //disable UART
  NRF_SAADC ->ENABLE = 0; //disable ADC
  NRF_RADIO ->TXPOWER = -20;
  NRF_PWM0  ->ENABLE = 0; //disable all pwm instance
  NRF_PWM1  ->ENABLE = 0;
  NRF_PWM2  ->ENABLE = 0;
  NRF_TWIM1 ->ENABLE = 0; //disable TWI Master
  NRF_TWIS1 ->ENABLE = 0; //disable TWI Slave

  NRF_SPI0 -> ENABLE = 0; //disable SPI
  NRF_SPI1 -> ENABLE = 0; //disable SPI
  NRF_SPI2 -> ENABLE = 0; //disable SPI
}
