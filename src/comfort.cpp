#include "comfort.h"
#include "sensors.h"

float normalizeValue(float value, ComfortRange &range) {
  // Ideal range
  if (value >= range.idealMin && value <= range.idealMax) {
    return 100.0;
  }
  
  // Lower acceptable range
  if (value >= range.acceptableMin && value < range.idealMin) {
    return 50.0 + 50.0 * (value - range.acceptableMin) / (range.idealMin - range.acceptableMin);
  }
  
  // Upper acceptable range
  if (value > range.idealMax && value <= range.acceptableMax) {
    return 50.0 + 50.0 * (range.acceptableMax - value) / (range.acceptableMax - range.idealMax);
  }
  
  // Unacceptable range
  return 0.0;
}

float processComfortParam(ComfortRange &range, float value, float &totalWeight) {
  if (range.weight <= 0) {
    return 0.0;
  }
  
  float normalizedValue = normalizeValue(value, range);
  totalWeight += range.weight;
  return normalizedValue * range.weight;
}

float calculateComfortScore() {
  float score = 0.0;
  float totalWeight = 0.0;
  ComfortParam& params = comfortParams[displayMode];
  
  // Process each comfort parameter
  score += processComfortParam(params.temperature, readTemperature(), totalWeight);
  score += processComfortParam(params.humidity, readHumidity(), totalWeight);
  score += processComfortParam(params.noise, readNoiseValue(), totalWeight);
  score += processComfortParam(params.light, readLightLevel(), totalWeight);
  score += processComfortParam(params.airQuality, readAirQuality(), totalWeight);
  score += processComfortParam(params.uvIndex, readUVIndex(), totalWeight);
  score += processComfortParam(params.pressure, readPressure(), totalWeight);
  
  // Add up the scores for used parameters
  if (totalWeight > 0) {
    return score / totalWeight;
  } else {
    return 0.0;
  }
}