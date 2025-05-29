#include "web_server.h"
#include "sensors.h"
#include "comfort.h"

void initializeWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Attempting WiFi connection...");
}

void handleWiFiAndServer() {
  if (!wifiConnected && WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    if (MDNS.begin("esp32")) Serial.println("MDNS responder started");
    server.on("/", handleRoot);
    server.on("/setmode", handleSetMode);
    server.begin();
    Serial.println("Web server started");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (wifiConnected) {
    server.handleClient();
  }
}

void handleRoot() {
  String html = generateWebPage();
  server.send(200, "text/html", html);
}

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
  .centered-container {\
    display: flex;\
    justify-content: center;\
    text-align: center;\
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
            + String(modes[i].icon) + "</a>";
  }
  html += "</div>";
  
  html += "<p>" + String(modes[displayMode].icon) + "<span class='label'>Mode:</span><span>" + String(modes[displayMode].name) + "</span></p>";

  for (String metric : modes[displayMode].metrics) {
    html += renderMetric(metric);
  }

  html += generateWarnings();

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
    float value = readAirQuality();

    // Access current profile’s comfort range for air quality
    ComfortRange airRange = comfortParams[displayMode].airQuality;
    float idealMax = airRange.idealMax;
    float acceptableMax = airRange.acceptableMax;

    // Normalize value for color interpolation
    float ratio = constrain((value - idealMax) / (acceptableMax - idealMax), 0.0, 1.0);

    // Interpolate from green (#69f0ae) to red (#ff5252)
    int r = (int)(105 + ratio * (255 - 105));  // red: 105 -> 255
    int g = (int)(240 - ratio * (240 - 82));   // green: 240 -> 82
    int b = (int)(174 - ratio * (174 - 82));   // blue: 174 -> 82

    char color[8];
    sprintf(color, "#%02x%02x%02x", r, g, b);

    return "<p><i class='fas fa-smog icon' style='color:#a1887f;'></i>\
  <span class='label'>Air Quality:</span><span style='color:" + String(color) + "'>" + String(value, 1) + "</span></p>";
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

String generateWarnings() {
  String warnings = "<div class='centered-container'><div class='score-container' style='max-width: 500px;'><h3 style='text-align:center;'>Environment Warnings</h3><ul style='text-align:center;'>";
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
    //If air quality is over ideal max, add a specific warning and write in red
    if (metric.name == "Air quality" && val > r.acceptableMax) {
      warnings += "<li style='color:red;'> There is a chance of a dangerous gas to be in the air. Take caution!.</li>";
    }

  }

  if (warnings.indexOf("<li>") == -1)
    warnings += "<li>All environmental conditions are within ideal ranges.</li>";

  warnings += "</ul></div></div>";
  return warnings;
}
