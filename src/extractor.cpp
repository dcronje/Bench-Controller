#include "extractor.h"
#include "constants.h"
#include "isr-handlers.h"
#include "settings.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"

// Define constants
#define PWM_FREQUENCY 25000    // 25 kHz suitable for PC fans
#define SYSTEM_CLOCK 125000000 // 125 MHz system clock of the Raspberry Pi Pico
#define CLOCK_DIV 1            // PWM clock divider (should be 1, 2, 4, 8, etc.)

volatile int currentFanSpeed = 0;
volatile int targetFanSpeed = 0;
volatile uint64_t extractorRPM = 0;
volatile bool extractorOn = false;
volatile uint64_t pulseCount = 0;

TimerHandle_t rpmTimer;
// uint64_t lastTimerCall = 0;
// SemaphoreHandle_t mutex;

// Assuming deltaTime is in microseconds and we want RPM:
// There are 60,000,000 microseconds in a minute.
// If there are two pulses per revolution, the number of revolutions is pulseCount / 2.
void rpmTimerCallback(TimerHandle_t xTimer)
{
  // static uint64_t lastTime = 0; // Ensure this is static or globally defined

  // uint64_t currentTime = to_us_since_boot(get_absolute_time());
  // uint64_t deltaTime = currentTime - lastTime; // Calculate elapsed time in microseconds

  uint64_t revolutions = pulseCount / 2; // Two pulses per revolution
  uint64_t rpm = revolutions * 60;       // * 1000000 / deltaTime; // Convert to RPM

  // printf("Pulse Count: %llu, deltaTime: %llu us\n", pulseCount, deltaTime);
  printf("RPM: %llu pulseCount: %d\n", rpm, pulseCount);

  // lastTime = currentTime; // Update last time
  pulseCount = 0; // Reset pulse count
}

void initExtractor(void)
{
  // mutex = xSemaphoreCreateMutex();
  configurePWM(EXTRACTOR_PWM_GPIO);
  configureTachometer(EXTRACTOR_TACH_GPIO);

  rpmTimer = xTimerCreate("RPM Timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, rpmTimerCallback);
  if (rpmTimer == NULL)
  {
    // Handle error
    printf("Failed to create RPM timer\n");
  }
  else
  {
    xTimerStart(rpmTimer, 0); // Start the timer
  }
}

void startExtractor(void)
{
  targetFanSpeed = 100; // currentSettings.fanSpeed
  extractorOn = true;
}

void stopExtractor(void)
{
  targetFanSpeed = 0;
  extractorOn = false; // Ensure fan is actually stopping
}

void updateExtractorSpeed(void)
{
  if (extractorOn)
  {
    targetFanSpeed = currentSettings.fanSpeed;
  }
}

void extractorTask(void *params)
{
  const TickType_t xDelay = pdMS_TO_TICKS(100); // Increase delay for slower ramp
  while (1)
  {
    if (currentFanSpeed != targetFanSpeed)
    {
      if (currentFanSpeed < targetFanSpeed)
      {
        currentFanSpeed += 1;
      }
      else
      {
        currentFanSpeed -= 1;
      }
    }

    // Set PWM level or drive it low if speed is zero
    uint pwmLevel = (currentFanSpeed > 0) ? (currentFanSpeed * calculatePWMWrapValue(PWM_FREQUENCY)) / 100 : 0;
    pwm_set_gpio_level(EXTRACTOR_PWM_GPIO, pwmLevel);

    // printf("FAN SPEED: %d RPM: %d\n", currentFanSpeed, extractorRPM);
    vTaskDelay(xDelay);
  }
}

static void configurePWM(uint gpio)
{
  uint slice_num = pwm_gpio_to_slice_num(gpio);
  gpio_set_function(gpio, GPIO_FUNC_PWM);

  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, CLOCK_DIV);
  pwm_config_set_wrap(&config, calculatePWMWrapValue(PWM_FREQUENCY));

  pwm_init(slice_num, &config, true);
  pwm_set_gpio_level(gpio, 0); // Ensure fan starts off
}

static uint calculatePWMWrapValue(uint frequency)
{
  return (SYSTEM_CLOCK / (frequency * CLOCK_DIV)) - 1;
}

static void configureTachometer(uint gpio)
{
  gpio_set_function(gpio, GPIO_FUNC_SIO);
  gpio_set_dir(gpio, GPIO_IN);
  // Explicitly specifying the ISR function removes the global IRS function which prevents buttons from working (don't do it)
  // gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &tachometerISR);
  gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, true);
}

void tachometerISR(uint gpio, uint32_t events)
{
  if (gpio == EXTRACTOR_TACH_GPIO && events & GPIO_IRQ_EDGE_FALL)
  {
    pulseCount++;
  }
}
