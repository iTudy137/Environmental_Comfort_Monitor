#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"

// ------------------- INITIALIZATION -------------------
void initializeSensors();

// ------------------- SENSOR READING FUNCTIONS -------------------
float readTemperature();
float readHumidity();
float readPressure();
String readNoise();
float readNoiseValue();
int readUVIndex();
int readLightLevel();
float readAirQuality();

// ------------------- HELPER FUNCTIONS -------------------
float adcInMillivolts(int analogPin);
int convertVoltageToUVIndex(float mvVoltage);

#endif