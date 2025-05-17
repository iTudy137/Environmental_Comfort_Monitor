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

void handleRoot();
void handleButton();
void displayModeOnLCD();
String generateWebPage();
String renderMetric(const String &metric);
String formatShortMetric(const String &metric);

// ------------------- CONFIGURATION -------------------
const char *ssid = "iPhone_iTudy";
const char *password = "Parola137";

#define DHTPIN 27
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define SOUND_PIN 26
#define BUTTON_PIN 14
#define MQ2_PIN 32
#define UV_SENSOR_PIN 34
#define LDR_PIN 35

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

unsigned long lastDisplayUpdate = 0;
int buttonState = HIGH, lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
int displayMode = 0;
bool wifiConnected = false;

unsigned long lastMetricCycleTime = 0;
const unsigned long metricCycleInterval = 4000; // 4 seconds
int currentMetricGroupIndex = 0;
String lastDisplayedLine = "";

const int adcMaxValue = 4095;
const float analogReference = 1.1;

// ------------------- MODES -------------------
enum DisplayMode { HOME = 0, OUTDOOR = 1, GREENHOUSE = 2 };

struct ModeConfig {
  const char* name;
  const char* icon;
  std::vector<String> metrics;
};

ModeConfig modes[] = {
  { "Home", "<i class='fas fa-home icon' style='color:#4caf50;'></i>", {"Temperature", "Humidity", "Noise"} },
  { "Outdoor", "<i class='fas fa-tree icon' style='color:#81c784;'></i>", {"Temperature", "Humidity", "AirQuality", "Noise", "UV", "Pressure"} },
  { "GreenHouse", "<i class='fas fa-seedling icon' style='color:#aed581;'></i>", {"Temperature", "Humidity", "AirQuality", "Light"} }
};

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(SOUND_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  lcd.init();
  lcd.backlight();

  if (bmp.begin()) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  } else {
    Serial.println("BMP280 not found!");
  }

  WiFi.begin(ssid, password);
  Serial.println("Attempting WiFi connection...");
}

// ------------------- LOOP -------------------
void loop() {
  if (!wifiConnected && WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    if (MDNS.begin("esp32")) Serial.println("MDNS responder started");
    server.on("/", handleRoot);
    server.begin();
    Serial.println("Web server started");
  }

  if (wifiConnected) server.handleClient();

  handleButton();

  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();
    displayModeOnLCD();
  }
}

// ------------------- BUTTON HANDLER -------------------
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonReading) lastDebounceTime = millis();
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && buttonState == HIGH) {
      displayMode = (displayMode + 1) % 3;
      currentMetricGroupIndex = 0;
      lastMetricCycleTime = millis();
    }
    buttonState = reading;
  }
  lastButtonReading = reading;
}

// ------------------- SENSOR READERS -------------------
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
  // Serial.print("mv Voltage: ");
  // Serial.println(mvVoltage);

  int uvIndex = convertVoltageToUVIndex(mvVoltage);
  // Serial.print("UV Index: ");
  // Serial.println(uvIndex);

  return uvIndex;
}

int readLightLevel() {
  float mvVoltage = adcInMillivolts(LDR_PIN);
  // Serial.print("LDR mv Voltage: ");
  // Serial.println(mvVoltage);

  int level = map(mvVoltage, 0, 1100, 0, 5);
  
  return constrain(level, 0, 5);
}

float readAirQuality() {
  float mvVoltage = adcInMillivolts(MQ2_PIN);
  // Serial.print("MQ2 mv Voltage: ");
  // Serial.println(mvVoltage);

  float qualityIndex = map(mvVoltage, 0, 1100, 500, 0);

  // return constrain(qualityIndex, 0, 500);
  return mvVoltage;
}

// ------------------- LCD DISPLAY -------------------
void displayModeOnLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  lcd.print(modes[displayMode].name);
  int nameLen = strlen(modes[displayMode].name);
  for (int i = 6 + nameLen; i < 16; i++) lcd.print(" ");  // Clear rest of line

  std::vector<String>& metrics = modes[displayMode].metrics;

  if (millis() - lastMetricCycleTime >= metricCycleInterval) {
    lastMetricCycleTime = millis();
    currentMetricGroupIndex++;
  }

  // Build lines to display, each max 16 chars
  std::vector<String> lines;
  String currentLine = "";
  for (String metric : metrics) {
    String mStr = formatShortMetric(metric);
    if (currentLine.length() + mStr.length() + 1 <= 16) {
      if (!currentLine.isEmpty()) currentLine += " ";
      currentLine += mStr;
    } else {
      lines.push_back(currentLine);
      currentLine = mStr;
    }
  }
  if (!currentLine.isEmpty()) lines.push_back(currentLine);

  if (currentMetricGroupIndex >= lines.size()) currentMetricGroupIndex = 0;

  String lineToShow = lines[currentMetricGroupIndex];

  // Update only if text is different
  if (lineToShow != lastDisplayedLine) {
    lcd.setCursor(0, 1);
    lcd.print(lineToShow);
    for (int i = lineToShow.length(); i < 16; i++) lcd.print(" ");
    lastDisplayedLine = lineToShow;
  }
}

String formatShortMetric(const String& metric) {
  if (metric == "Temperature") {
    float t = readTemperature();
    if (t < 0) t = 0;
    return "T:" + String(t, 1) + "\xDF" "C";
  }
  if (metric == "Humidity") {
    float h = readHumidity();
    if (h < 0) h = 0;
    return "H:" + String(h, 0) + "%";
  }
  if (metric == "Noise") {
    return "N:" + String(readNoise() == "Quiet" ? "Quiet" : "Loud");
  }
  if (metric == "UV") {
    return "UV:" + String(readUVIndex());
  }
  if (metric == "Light") {
    int level = readLightLevel();
    String bar = "";
    for (int i = 0; i < 5; i++) {
      bar += (i < level) ? "\xFF" : " ";
    }
    return "L:[" + bar + "]";
  }
  if (metric == "Pressure") {
    float p = readPressure();
    return "P:" + String(p, 0) + "hPa";
  }
  if (metric == "AirQuality") {
    return "AQ:" + String(readAirQuality(), 0);
  }
  return "";
}


// ------------------- WEB HANDLER -------------------
void handleRoot() {
  String html = generateWebPage();
  server.send(200, "text/html", html);
}

String generateWebPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>\
<meta http-equiv='refresh' content='2'/><meta name='viewport' content='width=device-width, initial-scale=1'>\
<title>Environmental Comfort Monitor</title>\
<link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css'>\
<style>\
  body{background:#121212;color:#eee;font-family:'Segoe UI',sans-serif;text-align:center;padding:20px;}\
  .title{font-size:1.2rem;color:#888;margin-bottom:14px;}p{font-size:1.5rem;margin:12px 0;}i{margin-right:10px;font-size:1.5rem;}\
  .icon{margin-right:10px;font-size:2rem;vertical-align:middle;}.label{font-weight:bold;margin-right:8px;}\
  .units{margin-right:8px;}\
  .bar-container {\
    display: inline-flex;\
    gap: 4px;\
    margin-top: 0px;\
  }\
  .bar-segment {\
    width: 12px;\
    height: 20px;\
    background-color: #e0e0e0;\
    border-radius: 3px;\
  }\
  .bar-segment.filled {\
    background-color: #fff176;\
  }\
</style></head><body>";

  html += "<div class='title'>Environmental Comfort Monitor</div>";
  html += "<p>" + String(modes[displayMode].icon) + "<span class='label'>Mode:</span><span>" + String(modes[displayMode].name) + "</span></p>";

  for (String metric : modes[displayMode].metrics) {
    html += renderMetric(metric);
  }

  html += "</body></html>";
  return html;
}

String renderMetric(const String& metric) {
  if (metric == "Temperature") {
    return "<p><i class='fas fa-thermometer-half icon' style='color:#ff5252;'></i>\
<span class='label'>Temperature:</span><span>" + String(readTemperature(), 1) + "</span><span class='units'> °C</span></p>";
  }
  if (metric == "Humidity") {
    return "<p><i class='fas fa-tint icon' style='color:#4fc3f7;'></i>\
<span class='label'>Humidity:</span><span>" + String(readHumidity(), 0) + "</span><span class='units'> %</span></p>";
  }
  if (metric == "Noise") {
    return "<p><i class='fas fa-volume-up icon' style='color:#ce93d8;'></i>\
<span class='label'>Noise:</span><span>" + readNoise() + "</span></p>";
  }
  if (metric == "UV") {
    return "<p><i class='fas fa-sun icon' style='color:#fdd835;'></i>\
<span class='label'>UV Index:</span><span>" + String(readUVIndex()) + "</span></p>";
  }
  if (metric == "Light") {
    int level = readLightLevel();
    String bar = "<div class='bar-container' style='display:inline-flex; margin-left:8px;'>";
    for (int i = 0; i < 5; i++) {
      bar += "<div class='bar-segment" + String((i < level) ? " filled" : "") + "'></div>";
    }
    bar += "</div>";
    return "<p><i class='fas fa-lightbulb icon' style='color:#fff176;'></i>\
<span class='label'>Light Level:</span>" + bar + "</p>";
  }
  if (metric == "Pressure") {
    return "<p><i class='fas fa-cloud icon' style='color:#90caf9;'></i>\
<span class='label'>Pressure:</span><span>" + String(readPressure(), 1) + "</span><span class='units'> hPa</span></p>";
  }
  if (metric == "AirQuality") {
    return "<p><i class='fas fa-smog icon' style='color:#a1887f;'></i>\
<span class='label'>Air Quality:</span><span>" + String(readAirQuality(), 1) + "</span></p>";
  }
  return "";
}
