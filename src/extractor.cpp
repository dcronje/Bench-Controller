#include "extractor.h"
#include "constants.h"
#include "isr-handlers.h"
#include "settings.h"
#include "pwm.pio.h" // Make sure this includes the SDK's PWM PIO program

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"

#define PWM_FREQUENCY 25000 // 25 kHz suitable for PC fans

volatile int currentFanSpeed = 0;
volatile uint64_t extractorRPM = 0;
volatile bool extractorOn = false;
volatile int targetFanSpeed = 0;
volatile uint64_t pulseCount = 0;

TimerHandle_t rpmTimer;

void rpmTimerCallback(TimerHandle_t xTimer)
{
  static uint64_t lastTime = 0;
  uint64_t localPulseCount = pulseCount;
  uint64_t currentTime = to_us_since_boot(get_absolute_time());
  uint64_t deltaTime = currentTime - lastTime; // Calculate the elapsed time in microseconds

  if (lastTime != 0 && deltaTime > 0 && localPulseCount > 0)
  {
    uint64_t revolutions = localPulseCount / 2;           // Convert pulses to full revolutions
    uint64_t rpm = revolutions * 60000000ULL / deltaTime; // Convert to RPM
    printf("RPM: %llu\n", rpm);
  }
  else
  {
    printf("RPM: 0\n");
  }

  lastTime = currentTime; // Update the last time stamp for the next calculation
  pulseCount = 0;         // Reset pulse count for the next interval
}

void pwmProgramInit(PIO pio, uint sm, uint offset, uint pin)
{
  uint32_t sys_clk_freq = clock_get_hz(clk_sys);                 // Get the system clock frequency
  float divider = (float)sys_clk_freq / (PWM_FREQUENCY * 65536); // Calculate divider for a full 16-bit duty cycle range at 25kHz

  pio_gpio_init(pio, pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

  pio_sm_config c = pwm_program_get_default_config(offset);
  sm_config_set_sideset_pins(&c, pin);
  sm_config_set_clkdiv(&c, divider); // Set the clock divider to achieve the desired frequency
  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_enabled(pio, sm, true);
}

void configurePWMPIO(uint gpio)
{
  PIO pio = pio0;
  uint sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &pwm_program);

  pwmProgramInit(pio, sm, offset, gpio); // Initialize the PIO with the custom function

  // Initialize PWM duty cycle to 0 (fan off)
  pio_sm_put_blocking(pio, sm, 0);
}

void updateFanSpeedPIO(PIO pio, uint sm, int speed)
{
  uint duty_cycle = (speed * 65535) / 100;  // Convert speed percentage to duty cycle
  pio_sm_put_blocking(pio, sm, duty_cycle); // Update the PIO with the new duty cycle
}

void initExtractor(void)
{
  configurePWMPIO(EXTRACTOR_PWM_GPIO);
  configureTachometer(EXTRACTOR_TACH_GPIO);

  rpmTimer = xTimerCreate("RPM Timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, rpmTimerCallback);
  if (rpmTimer == NULL)
  {
    printf("Failed to create RPM timer\n");
  }
  else
  {
    xTimerStart(rpmTimer, 0);
  }
}

void startExtractor(void)
{
  targetFanSpeed = 100; // Assume max speed for example
  extractorOn = true;
}

void stopExtractor(void)
{
  targetFanSpeed = 0;
  extractorOn = false;
}

void updateExtractorSpeed(void)
{
  if (extractorOn)
  {
    targetFanSpeed = currentSettings.fanSpeed; // Update target speed based on settings
  }
}

void extractorTask(void *params)
{
  const TickType_t xDelay = pdMS_TO_TICKS(200);
  PIO pio = pio0; // Assuming PIO0 is used
  uint sm = 0;    // Assuming state machine 0 is used

  while (1)
  {
    if (currentFanSpeed != targetFanSpeed)
    {
      currentFanSpeed = targetFanSpeed; // Update speed directly for simplicity
    }
    updateFanSpeedPIO(pio, sm, currentFanSpeed);
    printf("FAN SPEED: %d RPM: %d\n", currentFanSpeed, extractorRPM);
    vTaskDelay(xDelay);
  }
}

void configureTachometer(uint gpio)
{
  gpio_set_dir(gpio, GPIO_IN);
  gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, true); // Listen for falling edges on the tachometer pin
}

void tachometerISR(uint gpio, uint32_t events)
{
  pulseCount++; // Increment pulse count on each falling edge
}
