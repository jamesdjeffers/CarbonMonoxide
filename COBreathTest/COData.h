/*****************************************************************************
 * University of Oklahoma NPL CO Sensor
 * 
 * Data analysis and processing class
 * 
 */

#ifndef COData_h
#define COData_h

#include <Arduino.h>
#include <FIR.h>

#define ANALOG_INPUT    A0
#define SAMPLE_DELAY    10
#define SAMPLE_SIZE     25
#define BKG_SIZE        8
#define TEST_SIZE       4

#define CAL_VOLT_ZERO       186
#define CAL_VOLT_RANGE      435
#define SENSOR_MAX          1000

class COData
{
   private:
     float bkgAverage = 0;
     float bkgTrend [BKG_SIZE] = {0,0,0,0,0,0,0,0};
     int bkgIndex = 0;

     float testAverage = 0;
     float testTrend [TEST_SIZE] = {0,0,0,0};
     int testIndex = 0;

     FIR<float, 25> fir_avg;
     // For a moving average we use all ones as coefficients.
     float coef_avg[25] = {1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1.};

   public:
     COData();
     float readSensor();
     float readBkg();
     float readTest();
     int getBkg();
     int convert(float current);
};

#endif
