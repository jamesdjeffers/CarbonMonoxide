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
#define BKG_SIZE        5

class COData
{
   private:
     float bkgAverage = 0;
     float bkgTrend [BKG_SIZE] = {0,0,0,0,0};
     int bkgIndex = 0;
     
     FIR<float, 13> fir_lp;
     float coef_lp[13] = { 660, 470, -1980, -3830, 504, 10027, 15214, 10027, 504, -3830, -1980, 470, 660};

     FIR<float, 10> fir_avg;
     // For a moving average we use all ones as coefficients.
     float coef_avg[25] = {1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1.};

   public:
     COData();
     float readSensor();
     float readBkg();
     int getBkg();
     float filterData(int COAnalogValue);
};

#endif
