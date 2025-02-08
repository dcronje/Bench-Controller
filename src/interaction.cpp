#include "interaction.h"
#include "display.h"
#include "compressor-status.h"
#include "control.h"
#include "isr-handlers.h"
#include "extractor.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "queue.h"

#define DEBOUNCE_TIME_MS 50
#define LONG_PRESS_TIME_MS 500
#define QUEUE_WAIT_TIME_MS 100

SemaphoreHandle_t interactionQueue = NULL;
volatile int32_t encoderPosition = 0;

TimerHandle_t debounceTimers[3];
TimerHandle_t longPressTimers[3];
volatile bool buttonStates[3] = {0};         // 0 for up, 1 for down
volatile bool longPressHandled[3] = {false}; // Track if long press has been handled

void debounceTimerCallback(TimerHandle_t xTimer);
void longPressTimerCallback(TimerHandle_t xTimer);

void initInteraction()
{
  interactionQueue = xQueueCreate(10, sizeof(Interaction));
  gpio_init(ENCODER_CLK_GPIO);
  gpio_init(ENCODER_DC_GPIO);
  gpio_set_dir(ENCODER_CLK_GPIO, GPIO_IN);
  gpio_set_dir(ENCODER_DC_GPIO, GPIO_IN);

  gpio_set_irq_enabled(ENCODER_CLK_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
  gpio_set_irq_enabled(ENCODER_DC_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

  const uint gpio_buttons[3] = {ENTER_SW_GPIO, COMPRESSOR_BUTTON_GPIO, EXTRACTOR_BUTTON_GPIO};
  for (int i = 0; i < 3; i++)
  {
    debounceTimers[i] = xTimerCreate("DebounceTimer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdFALSE, (void *)(uintptr_t)i, debounceTimerCallback);
    longPressTimers[i] = xTimerCreate("LongPressTimer", pdMS_TO_TICKS(LONG_PRESS_TIME_MS), pdFALSE, (void *)(uintptr_t)i, longPressTimerCallback);
    gpio_init(gpio_buttons[i]);
    gpio_set_dir(gpio_buttons[i], GPIO_IN);
    gpio_pull_up(gpio_buttons[i]); // Assuming active low buttons
    gpio_set_irq_enabled(gpio_buttons[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
  }
}

void handleButtonISR(uint gpio, uint32_t events)
{
  int index = (gpio == ENTER_SW_GPIO ? 0 : (gpio == COMPRESSOR_BUTTON_GPIO ? 1 : 2));
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  // Start or reset the debounce timer
  xTimerResetFromISR(debounceTimers[index], &higherPriorityTaskWoken);
}

void debounceTimerCallback(TimerHandle_t xTimer)
{
  int index = (int)(uintptr_t)pvTimerGetTimerID(xTimer);
  uint gpio = (index == 0 ? ENTER_SW_GPIO : (index == 1 ? COMPRESSOR_BUTTON_GPIO : EXTRACTOR_BUTTON_GPIO));

  // Check the stable state of the button
  bool currentLevel = gpio_get(gpio) == 0; // Assuming active low
  if (currentLevel != buttonStates[index])
  {
    buttonStates[index] = currentLevel;
    if (currentLevel)
    {
      // Button press detected
      xTimerStart(longPressTimers[index], 0); // Start long press timer
      longPressHandled[index] = false;
    }
    else
    {
      // Button release detected
      xTimerStop(longPressTimers[index], 0);
      if (!longPressHandled[index])
      {
        // Short press detected
        Interaction action = (index == 0 ? ENTER : (index == 1 ? COMPRESSOR : EXTRACTOR));
        xQueueSendFromISR(interactionQueue, &action, NULL);
      }
    }
  }
}

void longPressTimerCallback(TimerHandle_t xTimer)
{
  int index = (int)(uintptr_t)pvTimerGetTimerID(xTimer);
  longPressHandled[index] = true;
  Interaction action = (index == 0 ? BACK : (index == 1 ? COMPRESSOR_LONG_PRESS : EXTRACTOR_LONG_PRESS));
  xQueueSendFromISR(interactionQueue, &action, NULL);
}

// Ensure the remaining part of your program initializes and uses these structures properly

void handleEncoderISR(uint gpio, uint32_t events)
{
  static int32_t last_level_a = -1;
  static int32_t lastEncoderPosition = 0;
  if (gpio == ENCODER_CLK_GPIO)
  {
    int level_a = gpio_get(ENCODER_CLK_GPIO);
    int level_b = gpio_get(ENCODER_DC_GPIO);

    if (level_a != last_level_a)
    {
      last_level_a = level_a;

      // Rising edge of A determines the action using B's state
      if (level_a == 1)
      {
        encoderPosition += (level_b == 1 ? 1 : -1);
      }
      // Falling edge of A can also determine the action if needed
      else
      {
        encoderPosition += (level_b == 0 ? 1 : -1);
      }
      Interaction action = NONE;
      if (encoderPosition > lastEncoderPosition)
      {
        action = UP;
      }
      else if (encoderPosition < lastEncoderPosition)
      {
        action = DOWN;
      }
      lastEncoderPosition = encoderPosition;
      if (action != NONE)
      {
        xQueueSendFromISR(interactionQueue, &action, NULL);
      }
      printf("Encoder Position: %d\n", encoderPosition);
    }

    // Log the position for testing purposes
  }
}

void handleInteraction(Interaction interaction)
{
  switch (interaction)
  {
  case ENTER:
    printf("ENTER COMMAND\n");
    displayEnter();
    break;
  case BACK:
    printf("BACK COMMAND\n");
    displayBack();
    break;
  case UP:
    printf("UP COMMAND\n");
    displayUp();
    break;
  case DOWN:
    printf("DOWN COMMAND\n");
    displayDown();
    break;
  case COMPRESSOR:
    printf("COMPRESSOR COMMAND\n");
    if (g_compressorStatus.compressorOn)
    {
      sendOffCommand();
    }
    else
    {
      sendOnCommand();
    }
    break;
  case COMPRESSOR_LONG_PRESS:
    printf("COMPRESSOR MENU COMMAND\n");
    displayCompressorSettingsMenu();
    break;
  case EXTRACTOR:
    printf("EXTRACTOR COMMAND\n");
    alertExtractor(3);
    if (extractorOn)
    {
      stopExtractor();
    }
    else
    {
      startExtractor();
    }
    // TODO: turn extractor on off
    break;
  case EXTRACTOR_LONG_PRESS:
    printf("EXTRACTOR MENU COMMAND\n");
    displayExtractorSettingsMenu();
    break;
  default:
    break;
  }
}

void interactionTask(void *pvParameters)
{
  Interaction interaction = NONE;
  while (1)
  {
    if (xQueueReceive(interactionQueue, &interaction, pdMS_TO_TICKS(QUEUE_WAIT_TIME_MS)) == pdPASS)
    {
      // Process interaction commands
      handleInteraction(interaction);
    }
    else
    {
      vPortYield();
    }
  }
}
