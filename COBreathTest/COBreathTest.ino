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
#include <FIR.h>
 
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
int statusReady = STATUS_BIT_START;

// Sensor timing for different operation modes
// Sensor data must be stable for 60 seconds before enabling test
#define TIMER_WARMUP  60000
#define TIMER_TEST 15000
#define TIMER_OFF  30000
int timerSample = 1;
int timerStart = 0;

#define ANALOG_INPUT    A0
#define CAL_VOLT_ZERO   185
#define CAL_VOLT_MAX    931
#define SAMPLE_DELAY    10
#define SAMPLE_SIZE     25
float coAnalogValue = 0;
float coTrend [5] = {0,0,0,0,0};

int testMax = 0;

FIR<float, 13> fir_lp;
float coef_lp[13] = { 660, 470, -1980, -3830, 504, 10027, 15214, 10027, 504, -3830, -1980, 470, 660};

FIR<float, 10> fir_avg;
// For a moving average we use all ones as coefficients.
float coef_avg[25] = {1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1.};

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

  // Set the coefficients
  fir_lp.setFilterCoeffs(coef_lp);
  fir_avg.setFilterCoeffs(coef_avg);

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
 
void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();
  //NRF_SAADC ->ENABLE = 0; //disable ADC
  NRF_RADIO ->TXPOWER = -20;
  // Read the most current value of CO sensor
  coAnalogValue = map(analogRead(ANALOG_INPUT),CAL_VOLT_ZERO,CAL_VOLT_MAX,0,1000);

  if (statusReady && STATUS_BIT_START){
    if (millis() > TIMER_WARMUP){
      statusReady = statusReady ^ STATUS_BIT_START;
      //statusByte.writeValue(statusReady);
    }
  }
  else if (statusReady <= 1){
    if (coAnalogValue > 2){
      statusReady = 1;
      
    }
    else if (coAnalogValue < 2){
      statusReady = 0;
      
    }
  }

  if (millis() > TIMER_OFF){
    //digitalWrite(D10,LOW);
  }

  // Device is connected
  while (central.connected()) {
    statusReady = statusReady | STATUS_BIT_CONNECTED;
    testMax++;
    
    // Check device readiness: must be on for more time than WARMUP
    if (statusReady && STATUS_BIT_START){
      if (millis() > TIMER_WARMUP){
        statusReady = statusReady ^ STATUS_BIT_START;
        statusByte.writeValue(statusReady);
      }
    }

    // Check for external commands
    if (commandByte.written()) {
      // Identify the command: x01 => start test timer
      if (commandByte.value() & 0b00000001) {   
        // Clear the test status bits and previous results
        statusReady = statusReady | STATUS_BIT_TEST_NOW;
        testMax = 0;
        timerStart = millis();
        statusByte.writeValue(statusReady);
        coMax.writeValue(testMax);
      } else {                              
        statusReady = statusReady | STATUS_BIT_TEST_NOW;     
      }
    }

    // Data acquisiton and processing code
    for (int i = 0; i < SAMPLE_SIZE; i++){
      coAnalogValue = fir_avg.processReading(analogRead(ANALOG_INPUT));
      delay(SAMPLE_DELAY);
    }
    
    coConcentration.writeValue(map(coAnalogValue,CAL_VOLT_ZERO,CAL_VOLT_MAX,0,1000));
    coAnalogValue = fir_lp.processReading(coAnalogValue);
    coMax.writeValue(coAnalogValue);
        

        // Check for data analysis
        if ((statusReady & 0b00000001) && !(statusReady & 0b00000010)){
          if (coAnalogValue > testMax){
            testMax = coAnalogValue;
            coMax.writeValue(testMax);
          }
          if ((millis() - timerStart) > TIMER_TEST){
            statusReady = statusReady | STATUS_BIT_TEST_DONE;
            statusByte.writeValue(statusReady);
          }          
        }
        
  }
  if (statusReady & STATUS_BIT_CONNECTED){
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
    //NRF_POWER->SYSTEMOFF = 1;
  }
    
}
