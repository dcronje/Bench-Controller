#ifndef LIGHTS_H
#define LIGHTS_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

#define PWM_LED_FREQUENCY 1000 // 1 kHz for LED control
#define TRANSITION_STEP 5      // Brightness transition step
#define TRANSITION_DELAY 10    // Delay in ms between steps

typedef struct
{
  PIO pio;
  uint sm;
  uint pin;
  int targetBrightness;
  int currentBrightness;
} LEDPWMController;

void initLights();
void lightsTask(void *pvParameters);

extern volatile int lightBrightness;
extern volatile int lightTargetBrightness;

#endif // LIGHTS_H
