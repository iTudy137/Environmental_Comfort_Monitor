#include "config.h"

// ------------------- WIFI CONFIGURATION -------------------
const char *ssid = "iPhone_iTudy";
const char *password = "Parola137";

// ------------------- GLOBAL OBJECTS -------------------
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// ------------------- GLOBAL VARIABLES -------------------
unsigned long lastDisplayUpdate = 0;
int buttonState = HIGH;
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
int displayMode = 0;
bool wifiConnected = false;
unsigned long lastMetricCycleTime = 0;
const unsigned long metricCycleInterval = 4000;
int currentMetricGroupIndex = 0;
String lastDisplayedLine = "";
const int adcMaxValue = 4095;
const float analogReference = 1.1;

// ------------------- MODE CONFIGURATIONS -------------------
ModeConfig modes[3] = {
  { "Home", "<i class='fas fa-home icon' style='color:#4caf50;'></i>", {"Temperature", "Humidity", "AirQuality", "Noise", "Score"} },
  { "Outdoor", "<i class='fas fa-tree icon' style='color:#81c784;'></i>", {"Temperature", "Humidity", "AirQuality", "Noise", "UV", "Pressure", "Score"} },
  { "GreenHouse", "<i class='fas fa-seedling icon' style='color:#aed581;'></i>", {"Temperature", "Humidity", "AirQuality", "Light", "Score"} }
};

// ------------------- COMFORT PARAMETERS -------------------
ComfortParam comfortParams[3] = {
  // Home
  {
    {20.0, 22.0, 18.0, 26.0, 0.4},
    {40.0, 50.0, 30.0, 60.0, 0.3},
    {0.0, 0.0, 0.0, 1.0, 0.2},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 200.0, 0.0, 400.0, 0.1},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0}
  },
  // Outdoor
  {
    {18.0, 25.0, 12.0, 30.0, 0.25},
    {30.0, 50.0, 25.0, 65.0, 0.15},
    {0.0, 0.0, 0.0, 1.0, 0.10},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 300.0, 0.0, 400.0, 0.20},
    {0.0, 6.0, 0.0, 8.0, 0.15},
    {1013.0, 1023.0, 990.0, 1030.0, 0.15}
  },
  // Greenhouse
  {
    {22.0, 27.0, 20.0, 32.0, 0.35},
    {60.0, 80.0, 50.0, 90.0, 0.30},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {4.0, 5.0, 2.0, 5.0, 0.25},
    {0.0, 400.0, 0.0, 500.0, 0.10},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0}
  }
};