#ifndef INTERACTION_H
#define INTERACTION_H

#include "constants.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

#define LONG_PRESS_THRESHOLD 500 // 1 second threshold (in milliseconds)

void initInteraction();
void sharedISR(uint gpio, uint32_t events);
void interactionTask(void *pvParameters);

extern QueueHandle_t interactionQueue;

enum Interaction
{
  NONE,
  ENTER,
  BACK,
  UP,
  DOWN,
  COMPRESSOR,
  COMPRESSOR_LONG_PRESS,
  EXTRACTOR,
  EXTRACTOR_LONG_PRESS,
};

#endif // INTERACTION_H