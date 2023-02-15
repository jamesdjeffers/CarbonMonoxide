
#include "COData.h"

COData::COData(){
  // Set the coefficients
  fir_avg.setFilterCoeffs(coef_avg);
}

// Data acquisiton and processing code
float COData::readSensor(){
  // Loop over given number of iterations with delay to acquire multiple data points
  for (int i = 0; i < SAMPLE_SIZE-1; i++){
    fir_avg.processReading(analogRead(ANALOG_INPUT));
    delay(SAMPLE_DELAY);
  }
  return fir_avg.processReading(analogRead(ANALOG_INPUT));
}

// Read the most current value of CO sensor
float COData::readBkg(){
  bkgTrend[bkgIndex] = readSensor();
  bkgIndex++;
  bkgIndex %= BKG_SIZE;

  bkgAverage = 0;
  for (int i = 0; i < BKG_SIZE; i++){
    bkgAverage += bkgTrend[i];
  }
  bkgAverage = bkgAverage / BKG_SIZE;
  return bkgAverage;
}

// Read the most current value of CO sensor
float COData::readTest(){
  testTrend[testIndex] = readSensor();
  testIndex++;
  testIndex %= TEST_SIZE;

  testAverage = 0;
  for (int i = 0; i < TEST_SIZE; i++){
    testAverage += testTrend[i];
  }
  testAverage = testAverage / TEST_SIZE;
  return testAverage;
}

// Read the most current value of CO sensor
int COData::getBkg(){
  
  return bkgAverage;
}

int COData::convert(float current){
  return (current-CAL_VOLT_ZERO)*1000/CAL_VOLT_RANGE;
}
