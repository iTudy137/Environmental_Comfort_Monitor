#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

// ------------------- INITIALIZATION -------------------
void initializeDisplay();

// ------------------- BUTTON HANDLING -------------------
void handleButton();

// ------------------- LCD DISPLAY FUNCTIONS -------------------
void displayModeOnLCD();
String formatShortMetric(const String& metric);

#endif