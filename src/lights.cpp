#include "constants.h"
#include "lights.h"
#include "pwm.pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"

#include "FreeRTOS.h"
#include "task.h"

// Declare global LED PWM controllers
LEDPWMController lightsA, lightsB, lightsC;

// Initialize a single LED PWM controller
void ledPwmInit(LEDPWMController *controller, PIO pio, uint sm, uint pin)
{
  controller->pio = pio;
  controller->sm = sm;
  controller->pin = pin;
  controller->targetBrightness = 0;
  controller->currentBrightness = 0;

  uint32_t sysClkFreq = clock_get_hz(clk_sys);
  float divider = (float)sysClkFreq / (PWM_LED_FREQUENCY * 65536);

  pio_gpio_init(pio, pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

  pio_sm_config c = pwm_program_get_default_config(pio_add_program(pio, &pwm_program));
  sm_config_set_sideset_pins(&c, pin);
  sm_config_set_clkdiv(&c, divider);
  pio_sm_init(pio, sm, 0, &c);
  pio_sm_set_enabled(pio, sm, true);
}

// Set the duty cycle of an LED PWM controller
void ledPwmSetDutyCycle(LEDPWMController *controller, uint dutyCycle)
{
  uint scaledDutyCycle = (dutyCycle * 65535) / 100;
  pio_sm_put_blocking(controller->pio, controller->sm, scaledDutyCycle);
  controller->currentBrightness = dutyCycle;
}

// Function to start the transition for a specific controller
void setLightsBrightness(LEDPWMController *controller, int brightness)
{
  controller->targetBrightness = brightness;
}

// Specific functions for each LED bank
void setLightsABrightness(int percent)
{
  setLightsBrightness(&lightsA, percent);
}

void setLightsBBrightness(int percent)
{
  setLightsBrightness(&lightsB, percent);
}

void setLightsCBrightness(int percent)
{
  setLightsBrightness(&lightsC, percent);
}

// Initialize all controllers
void initLights()
{
  ledPwmInit(&lightsA, pio0, 0, LIGHTS_A_PWM_GPIO);
  ledPwmInit(&lightsB, pio0, 1, LIGHTS_B_PWM_GPIO);
  ledPwmInit(&lightsC, pio0, 2, LIGHTS_C_PWM_GPIO);
}

void lightsTask(void *pvParameters)
{
  while (1)
  {
    if (lightsA.currentBrightness > lightsA.targetBrightness)
    {
      ledPwmSetDutyCycle(&lightsA, (lightsA.currentBrightness - 1));
    }
    else if (lightsA.currentBrightness < lightsA.targetBrightness)
    {
      ledPwmSetDutyCycle(&lightsA, (lightsA.currentBrightness + 1));
    }
    if (lightsB.currentBrightness > lightsB.targetBrightness)
    {
      ledPwmSetDutyCycle(&lightsB, (lightsB.currentBrightness - 1));
    }
    else if (lightsB.currentBrightness < lightsB.targetBrightness)
    {
      ledPwmSetDutyCycle(&lightsB, (lightsB.currentBrightness + 1));
    }
    if (lightsC.currentBrightness > lightsC.targetBrightness)
    {
      ledPwmSetDutyCycle(&lightsC, (lightsC.currentBrightness - 1));
    }
    else if (lightsC.currentBrightness < lightsC.targetBrightness)
    {
      ledPwmSetDutyCycle(&lightsC, (lightsC.currentBrightness + 1));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}