#include "config.h"
#include "sensors.h"
#include "comfort.h"
#include "display.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);

  initializeDisplay();
  initializeSensors();
  initializeWiFi();
}

void loop() {
  handleWiFiAndServer();
  handleButton();
  displayModeOnLCD();
}
