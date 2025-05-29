#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"

// ------------------- INITIALIZATION -------------------
void initializeWiFi();
void handleWiFiAndServer();

// ------------------- WEB HANDLERS -------------------
void handleRoot();
void handleSetMode();

// ------------------- WEB PAGE GENERATION -------------------
String generateWebPage();
String renderMetric(const String &metric);
String generateWarnings();

#endif