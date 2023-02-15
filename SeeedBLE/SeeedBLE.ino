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
BLEWordCharacteristic coConcentration("2BD0", BLERead | BLENotify);
BLEByteCharacteristic statusByte("2BBB", BLERead | BLENotify);
BLEByteCharacteristic commandByte("2A9F", BLEWrite);
BLEWordCharacteristic coMax("2AF4", BLERead | BLENotify);
 
int bytes;
char serialBuffer [9] = {0xFF,0x01,0x78,0x03,0x00,0x00,0x00,0x00,0x84};

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

// Data class for sensor measurement at different times of operation

float coValue = 0;
float coBkgPretest = 0;
float coBkgConnect = 0;
float coBkgPosttest = 0;

int testMax = 0;
float coTrend [5] = {0,0,0,0,0};

 
void setup() {

  Serial1.begin(9600);
  Serial1.setTimeout(2000);
 
  // begin initialization
  BLE.begin();
 
  // set advertised local name and service UUID:
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
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();
  // Read the most current value of CO sensor
  bytes = Serial1.readBytes(serialBuffer,9);
  if (bytes > 0){
    coBkgPretest = (serialBuffer[2]*256+serialBuffer[3]);
    checkStatus(coBkgPretest);
  }
  else{
    coBkgPretest = 0xFF;
    statusReady |= 0xFF;
  }
  delay(800);
  
  // Device is connected
  while (central.connected()) {
    statusReady |= STATUS_BIT_CONNECTED;
    bytes = Serial1.readBytes(serialBuffer,9);
    if (bytes > 0){
      coValue = (serialBuffer[2]*256+serialBuffer[3]);
    }
    else{
      coValue = -1;
      statusReady |= 0xFF;
    }
    // Check device readiness: must be on for more time than WARMUP
    if (checkStatus(coValue)){
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
      coBkgConnect = coValue;
      coConcentration.writeValue(coValue);
    }
    else if ((statusReady & 0b00000001) && !(statusReady & 0b00000010)){
      coConcentration.writeValue(coValue);
      if (coValue > testMax){
        testMax = coValue;
        coMax.writeValue(testMax);
      }
      if ((millis() - timerStart) > TIMER_TEST){
        statusReady |= STATUS_BIT_TEST_DONE;
        statusByte.writeValue(statusReady);
        timerEnd = millis();
      }          
    }
    else {
      coBkgPosttest = coValue;
      coConcentration.writeValue(coBkgPosttest);
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
    
}

// Verify the current data value, must be on for more time than WARMUP
int checkStatus(float dataValue){
  int previousStatus = statusReady;
  if (millis() > TIMER_WARMUP){           // Check device "ON" time
      statusReady |= STATUS_BIT_START;
  }
  
  if (dataValue > 2){             // Check magnitude of signal
    statusReady |= STATUS_BIT_BKG_ZERO;
  }
  else {
    statusReady &= 0b10111111;
  }
  if (dataValue < 0){                  // Check for DC voltage drift
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

void modeMin(){
  NRF_UART0 ->TASKS_STOPTX = 1;
  NRF_UART0 ->TASKS_STOPRX = 1;
  NRF_UART0 ->ENABLE = 0;
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
