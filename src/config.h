#ifndef CONFIG_H
#define CONFIG_H

#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <vector>

// ------------------- WIFI CONFIGURATION -------------------
extern const char *ssid;
extern const char *password;

// ------------------- PIN DEFINITIONS -------------------
#define DHTPIN 27
#define DHTTYPE DHT22
#define SOUND_PIN 26
#define BUTTON_PIN 14
#define MQ2_PIN 32
#define UV_SENSOR_PIN 34
#define LDR_PIN 35
#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

// ------------------- GLOBAL OBJECTS -------------------
extern DHT dht;
extern Adafruit_BMP280 bmp;
extern LiquidCrystal_I2C lcd;
extern WebServer server;

// ------------------- GLOBAL VARIABLES -------------------
extern unsigned long lastDisplayUpdate;
extern int buttonState, lastButtonReading;
extern unsigned long lastDebounceTime;
extern const unsigned long debounceDelay;
extern int displayMode;
extern bool wifiConnected;
extern unsigned long lastMetricCycleTime;
extern const unsigned long metricCycleInterval;
extern int currentMetricGroupIndex;
extern String lastDisplayedLine;
extern const int adcMaxValue;
extern const float analogReference;

// ------------------- MODES AND STRUCTURES -------------------
enum DisplayMode { HOME = 0, OUTDOOR = 1, GREENHOUSE = 2 };

struct ModeConfig {
  const char* name;
  const char* icon;
  std::vector<String> metrics;
};

extern ModeConfig modes[3];

// ------------------- COMFORT SCORE STRUCTURES -------------------
struct ComfortRange {
  float idealMin;
  float idealMax;
  float acceptableMin;
  float acceptableMax;
  float weight;
};

struct ComfortParam {
  ComfortRange temperature;
  ComfortRange humidity;
  ComfortRange noise;
  ComfortRange light;
  ComfortRange airQuality;
  ComfortRange uvIndex;
  ComfortRange pressure;
};

extern ComfortParam comfortParams[3];

#endif