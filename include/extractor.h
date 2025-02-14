#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "constants.h"
#include "FreeRTOS.h"
#include "task.h"

void initExtractor(void);
static void configurePWM(uint gpio);
static void configureTachometer(uint gpio);
static uint calculatePWMWrapValue(uint frequency);
void extractorTask(void *params);
void startExtractor(void);
void stopExtractor(void);
void updateExtractorSpeed(void);
void tachometerISR(uint gpio, uint32_t events);

// External variable to hold the current fan speed
extern volatile int currentFanSpeed;
extern volatile int targetFanSpeed;
extern volatile uint64_t extractorRPM;
extern volatile uint64_t pulseCount;
extern volatile bool extractorOn;

#endif // EXTRACTOR_H
