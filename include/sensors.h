#ifndef SENSORS_H
#define SENSORS_H

#include "constants.h"
#include "FreeRTOS.h"

void initSensors(void);
void tempSensorTask(void *params);
void lightSensorTask(void *params);

// External variables (declarations only)
extern volatile float lightsATemp;
extern volatile float lightsBTemp;
extern volatile float lightsCTemp;
extern volatile float boothTemp;
extern volatile float boothHumidity;
extern volatile float boothLux;

#endif // SENSORS_H