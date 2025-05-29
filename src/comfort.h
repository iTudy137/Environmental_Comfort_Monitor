#ifndef COMFORT_H
#define COMFORT_H

#include "config.h"

// ------------------- COMFORT CALCULATION FUNCTIONS -------------------
float calculateComfortScore();
float normalizeValue(float value, ComfortRange &range);
float processComfortParam(ComfortRange &range, float value, float &totalWeight);

#endif