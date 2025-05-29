#include "display.h"
#include "sensors.h"
#include "comfort.h"

void initializeDisplay() {
  lcd.init();
  lcd.backlight();
  Serial.println("LCD initialized successfully");
}

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

void displayModeOnLCD() {
  if (millis() - lastDisplayUpdate < 1000) {
    return;
  }
  
  lastDisplayUpdate = millis();

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
    float aq = readAirQuality();
    float acceptableMax = comfortParams[displayMode].airQuality.acceptableMax;
    String status = (aq > acceptableMax) ? "Bad" : "OK";
    return "AQ:" + status;
  }
  if (metric == "Score") {
    float score = calculateComfortScore();
    return "Score:" + String(score, 0) + "%";
  }
  return "";
}