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
void handleSetMode(); // Nouă funcție pentru schimbarea modului prin web
void displayModeOnLCD();
String generateWebPage();
String renderMetric(const String &metric);
String generateWarnings();
String formatShortMetric(const String &metric);
float calculateComfortScore();

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

// COMFORT SCORE PARAMETERS
// Parameters for comfort ranges and weights
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
} comfortParams[] = {
  // Home
  {
    {21.0, 24.0, 18.0, 26.0, 0.5},
    {40.0, 60.0, 30.0, 70.0, 0.3},
    {0.0, 0.0, 0.0, 1.0, 0.2},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0}
  },
  // Outdoor
  {
    {18.0, 25.0, 12.0, 30.0, 0.25},
    {40.0, 60.0, 30.0, 75.0, 0.15},
    {0.0, 0.0, 0.0, 1.0, 0.10},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 300.0, 0.0, 500.0, 0.20},
    {0.0, 6.0, 0.0, 8.0, 0.15},
    {1013.0, 1023.0, 990.0, 1040.0, 0.15}
  },
  // Greenhouse
  {
    {22.0, 28.0, 18.0, 32.0, 0.35},
    {60.0, 80.0, 50.0, 90.0, 0.30},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {3.0, 5.0, 2.0, 5.0, 0.25},
    {0.0, 200.0, 0.0, 400.0, 0.10},
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0}
  }
};

// MODES
enum DisplayMode { HOME = 0, OUTDOOR = 1, GREENHOUSE = 2 };

struct ModeConfig {
  const char* name;
  const char* icon;
  std::vector<String> metrics;
};

ModeConfig modes[] = {
  { "Home", "<i class='fas fa-home icon' style='color:#4caf50;'></i>", {"Temperature", "Humidity", "Noise", "Score"} },
  { "Outdoor", "<i class='fas fa-tree icon' style='color:#81c784;'></i>", {"Temperature", "Humidity", "AirQuality", "Noise", "UV", "Pressure", "Score"} },
  { "GreenHouse", "<i class='fas fa-seedling icon' style='color:#aed581;'></i>", {"Temperature", "Humidity", "AirQuality", "Light", "Score"} }
};

// SETUP
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

// LOOP
void loop() {
  if (!wifiConnected && WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    if (MDNS.begin("esp32")) Serial.println("MDNS responder started");
    server.on("/", handleRoot);
    server.on("/setmode", handleSetMode);
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

// BUTTON HANDLER
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

// WEB MODE HANDLER
void handleSetMode() {
  if (server.hasArg("mode")) {
    String modeArg = server.arg("mode");
    int newMode = modeArg.toInt();
    
    if (newMode >= 0 && newMode <= 2) {
      displayMode = newMode;
      currentMetricGroupIndex = 0;
      lastMetricCycleTime = millis();
    }
  }
  
  // Redirect back to main page
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// SENSOR READERS
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

// COMFORT SCORE
// Function to normalize the value based on the comfort range
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

// LCD DISPLAY
void displayModeOnLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  lcd.print(modes[displayMode].name);
  int nameLen = strlen(modes[displayMode].name);

  // Clear the rest of the line
  for (int i = 6 + nameLen; i < 16; i++) {
    lcd.print(" ");
  }

  std::vector<String>& metrics = modes[displayMode].metrics;

  if (millis() - lastMetricCycleTime >= metricCycleInterval) {
    lastMetricCycleTime = millis();
    currentMetricGroupIndex++;
  }

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
  if (metric == "Score") {
    float score = calculateComfortScore();
    return "SCORE:" + String(score, 0) + "%";
  }
  return "";
}


// WEB HANDLER
void handleRoot() {
  String html = generateWebPage();
  server.send(200, "text/html", html);
}

String generateWebPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>\
<meta http-equiv='refresh' content='5'/><meta name='viewport' content='width=device-width, initial-scale=1'>\
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
  .score-container {\
    background: #2d2d2d;\
    border-radius: 10px;\
    padding: 10px;\
    margin-top: 10px;\
    border: 1px solid #444;\
  }\
  .progress-outer {\
    height: 20px;\
    border-radius: 10px;\
    background-color: #444;\
    margin: 10px 0;\
    overflow: hidden;\
  }\
  .progress-inner {\
    height: 100%;\
    background: linear-gradient(90deg, #ff5252, #69f0ae);\
    border-radius: 10px;\
    transition: width 0.3s ease;\
  }\
  .mode-buttons {\
    display: flex;\
    justify-content: center;\
    gap: 10px;\
    margin-bottom: 20px;\
  }\
  .mode-button {\
    background-color: #2d2d2d;\
    color: #eee;\
    border: 1px solid #444;\
    border-radius: 5px;\
    padding: 10px 0px 10px 10px;\
    font-size: 1rem;\
    cursor: pointer;\
    transition: all 0.3s ease;\
  }\
  .mode-button:hover {\
    background-color: #444;\
  }\
  .mode-button.active {\
    background-color:#b6ffb6;\
    border-color:#b6ffb6;\
  }\
</style></head><body>";

  html += "<div class='title'>Environmental Comfort Monitor</div>";

  // Buttons to change modes
html += "<div class='mode-buttons'>";
for (int i = 0; i < 3; i++) {
  html += "<a href='/setmode?mode=" + String(i) + "' class='mode-button' style='padding-left:10px;'>"
          + String(modes[i].icon) + "</a>";}
html += "</div>";
  
  html += "<p>" + String(modes[displayMode].icon) + "<span class='label'>Mode:</span><span>" + String(modes[displayMode].name) + "</span></p>";

  for (String metric : modes[displayMode].metrics) {
    html += renderMetric(metric);
  }

  html += generateWarnings();

  html += "</body></html>";
  return html;
}

String generateWarnings() {
  String warnings = "<div class='score-container'><h3>Environment Warnings</h3><ul style='text-align:left;'>";
  ComfortParam& params = comfortParams[displayMode];

  struct Metric {
    float value;
    ComfortRange range;
    String name;
    String unit;
  } metrics[] = {
    { readTemperature(), params.temperature, "Temperature", "°C" },
    { readHumidity(),    params.humidity,    "Humidity", "%" },
    { readNoiseValue(),  params.noise,       "Noise level", "" },
    { (float)readLightLevel(),  params.light,       "Light level", "/5" },
    { readAirQuality(),  params.airQuality,  "Air quality", "" },
    { (float)readUVIndex(),     params.uvIndex,     "UV Index", "" },
    { readPressure(),    params.pressure,    "Pressure", " hPa" }
  };

  for (auto& metric : metrics) {
    if (metric.range.weight <= 0) continue;

    float val = metric.value;
    auto& r = metric.range;

    String level;

    if (val < r.idealMin) {
      level = (val >= r.acceptableMin) ? "slightly below ideal" : "too low";
    } else if (val > r.idealMax) {
      level = (val <= r.acceptableMax) ? "slightly above ideal" : "too high";
    } else {
      continue;
    }

    warnings += "<li>" + metric.name + " is " + level + ". Ideal range: " +
                String(r.idealMin, 1) + "-" + String(r.idealMax, 1) + metric.unit + ".</li>";
  }

  if (warnings.indexOf("<li>") == -1)
    warnings += "<li>All environmental conditions are within ideal ranges.</li>";

  warnings += "</ul></div>";
  return warnings;
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
  if (metric == "Score") {
    float score = calculateComfortScore();
    String scoreColor;
    
    if (score >= 80) {
      scoreColor = "#69f0ae";
    } else if (score >= 60) {
      scoreColor = "#ffd54f";
    } else {
      scoreColor = "#ff5252";
    }
    
    return "<div class='score-container'><p><i class='fas fa-chart-line icon' style='color:#64b5f6;'></i>\
<span class='label'>Comfort Score:</span><span style='color:" + scoreColor + "'>" + String(score, 0) + "</span><span class='units'> %</span></p>\
<div class='progress-outer'><div class='progress-inner' style='width: " + String(score) + "%;'></div></div></div>";
  }
  return "";
}