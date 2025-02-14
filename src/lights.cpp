#include "constants.h"
#include "lights.h"
#include "settings.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "FreeRTOS.h"
#include "task.h"

// Declare global LED PWM controllers
LEDPWMController lightsA, lightsB, lightsC;

volatile int lightBrightness = 0;
volatile int lightTargetBrightness = currentSettings.lightBrightness;

// Initialize a single LED PWM controller
void ledPwmInit(uint gpio)
{
  gpio_set_function(gpio, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(gpio);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 4.f); // Set the clock divider
  pwm_config_set_wrap(&config, 65535); // Set the maximum PWM counter value
  pwm_init(slice_num, &config, true);
}

// Function to set the duty cycle for a given PWM GPIO
void ledPwmSetDutyCycle(uint gpio, uint dutyCycle)
{
  uint slice_num = pwm_gpio_to_slice_num(gpio);
  uint duty = (uint32_t)dutyCycle * 65535 / 100; // Convert percentage to duty cycle value
  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), duty);
}

void initLights()
{
  ledPwmInit(LIGHTS_A_PWM_GPIO);
  ledPwmInit(LIGHTS_B_PWM_GPIO);
  ledPwmInit(LIGHTS_C_PWM_GPIO);
}

void lightsTask(void *pvParameters)
{
  while (1)
  {
    // Set the same brightness for all lights
    if (lightBrightness > lightTargetBrightness)
    {
      lightBrightness--;
    }
    else if (lightBrightness < lightTargetBrightness)
    {
      lightBrightness++;
    }
    ledPwmSetDutyCycle(LIGHTS_A_PWM_GPIO, lightBrightness);
    ledPwmSetDutyCycle(LIGHTS_B_PWM_GPIO, lightBrightness);
    ledPwmSetDutyCycle(LIGHTS_C_PWM_GPIO, lightBrightness);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}