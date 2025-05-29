#include "sensors.h"

void initializeSensors() {
  dht.begin();
  pinMode(SOUND_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  if (bmp.begin()) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println("BMP280 initialized successfully");
  } else {
    Serial.println("BMP280 not found!");
  }
}

float readTemperature() {
  float t = dht.readTemperature();
  return isnan(t) ? -1 : t;
}

float readHumidity() {
  float h = dht.readHumidity();
  return isnan(h) ? -1 : h;
}

float readPressure() {
  float p = bmp.readPressure();
  return isnan(p) ? -1 : p / 100.0F;
}

String readNoise() {
  return digitalRead(SOUND_PIN) == HIGH ? "Loud" : "Quiet";
}

float readNoiseValue() {
  return digitalRead(SOUND_PIN) == HIGH ? 1.0 : 0.0;
}

float adcInMillivolts(int analogPin) {
  int analogValue = analogRead(analogPin);
  float voltage = (analogValue * analogReference) / adcMaxValue;
  return voltage * 1000.0;
}

int convertVoltageToUVIndex(float mvVoltage) {
  if (mvVoltage < 50) return 0;
  if (mvVoltage < 227) return 1;
  if (mvVoltage < 318) return 2;
  if (mvVoltage < 408) return 3;
  if (mvVoltage < 503) return 4;
  if (mvVoltage < 606) return 5;
  if (mvVoltage < 696) return 6;
  if (mvVoltage < 795) return 7;
  if (mvVoltage < 881) return 8;
  if (mvVoltage < 976) return 9;
  if (mvVoltage < 1079) return 10;
  return 11;
}

int readUVIndex() {
  float mvVoltage = adcInMillivolts(UV_SENSOR_PIN);
  int uvIndex = convertVoltageToUVIndex(mvVoltage);
  return uvIndex;
}

int readLightLevel() {
  float mvVoltage = adcInMillivolts(LDR_PIN);
  int level = map(mvVoltage, 0, 1100, 0, 5);
  return constrain(level, 0, 5);
}

float readAirQuality() {
  float mvVoltage = adcInMillivolts(MQ2_PIN);
  float qualityIndex = map(mvVoltage, 0, 1100, 500, 0);
  return mvVoltage;
}