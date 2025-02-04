#include "interaction.h"
#include "display.h"
#include "compressor-status.h"
#include "control.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "queue.h"

SemaphoreHandle_t interactionQueue = NULL;

volatile int32_t encoderPosition = 0;

TimerHandle_t enterLongPressTimer = NULL;
TimerHandle_t compressorLongPressTimer = NULL;
TimerHandle_t extractorLongPressTimer = NULL;

volatile int32_t enterButtonDown = 0;
volatile int32_t compressorButtonDown = 0;
volatile int32_t extractorButtonDown = 0;

static uint32_t enterPressStartTime = 0;
static uint32_t compressorPressStartTime = 0;
static uint32_t extractorPressStartTime = 0;

static bool enterLongPressHandled = false;
static bool compressorLongPressHandled = false;
static bool extractorLongPressHandled = false;

void handleButtonISR(uint gpio, uint32_t events)
{
  Interaction action = NONE;
  if (gpio == ENTER_SW_GPIO)
  {
    if (events & GPIO_IRQ_EDGE_FALL && enterButtonDown == 0)
    {
      printf("ENTER BUTTON DOWN\n");
      enterButtonDown = 1;
      enterPressStartTime = xTaskGetTickCount(); // Get current tick count as the start time
      enterLongPressHandled = false;
      xTimerStartFromISR(enterLongPressTimer, 0);
    }
    else if (events & GPIO_IRQ_EDGE_RISE && enterButtonDown == 1)
    {
      printf("ENTER BUTTON UP\n");
      enterButtonDown = 0;
      xTimerStopFromISR(enterLongPressTimer, 0);
      if (!enterLongPressHandled)
      {
        // Only send ENTER if long press was not handled
        action = ENTER;
      }
    }
  }
  else if (gpio == COMPRESSOR_BUTTON_GPIO)
  {
    if (events & GPIO_IRQ_EDGE_FALL && compressorButtonDown == 0)
    {
      printf("COMPRESSOR BUTTON DOWN\n");
      compressorButtonDown = 1;
      compressorPressStartTime = xTaskGetTickCount(); // Get current tick count as the start time
      compressorLongPressHandled = false;
      xTimerStartFromISR(compressorLongPressTimer, 0);
    }
    else if (events & GPIO_IRQ_EDGE_RISE && compressorButtonDown == 1)
    {
      printf("COMPRESSOR BUTTON UP\n");
      compressorButtonDown = 0;
      xTimerStopFromISR(compressorLongPressTimer, 0);
      if (!compressorLongPressHandled)
      {
        // Only send ENTER if long press was not handled
        action = COMPRESSOR;
      }
    }
  }
  else if (gpio == EXTRACTOR_BUTTON_GPIO)
  {
    if (events & GPIO_IRQ_EDGE_FALL && extractorButtonDown == 0)
    {
      printf("EXTRACTOR BUTTON DOWN\n");
      extractorButtonDown = 1;
      extractorPressStartTime = xTaskGetTickCount(); // Get current tick count as the start time
      extractorLongPressHandled = false;
      xTimerStartFromISR(extractorLongPressTimer, 0);
    }
    else if (events & GPIO_IRQ_EDGE_RISE && extractorButtonDown == 1)
    {
      printf("EXTRACTOR BUTTON UP\n");
      extractorButtonDown = 0;
      xTimerStopFromISR(extractorLongPressTimer, 0);
      if (!extractorLongPressHandled)
      {
        // Only send ENTER if long press was not handled
        action = EXTRACTOR;
      }
    }
  }
  if (action != NONE)
  {
    xQueueSendFromISR(interactionQueue, &action, NULL);
  }
}

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

void sharedISR(uint gpio, uint32_t events)
{
  if (gpio == ENCODER_CLK_GPIO || gpio == ENCODER_DC_GPIO)
  {
    handleEncoderISR(gpio, events);
  }
  else if (gpio == ENTER_SW_GPIO)
  {
    handleButtonISR(gpio, events);
  }
  else if (gpio == COMPRESSOR_BUTTON_GPIO)
  {
    handleButtonISR(gpio, events);
  }
  else if (gpio == EXTRACTOR_BUTTON_GPIO)
  {
    handleButtonISR(gpio, events);
  }
}

void enterLongPressCallback(TimerHandle_t xTimer)
{
  Interaction action = BACK;
  xQueueSendFromISR(interactionQueue, &action, NULL);
  enterLongPressHandled = true;
}

void compressorLongPressCallback(TimerHandle_t xTimer)
{
  Interaction action = COMPRESSOR_LONG_PRESS;
  xQueueSendFromISR(interactionQueue, &action, NULL);
  compressorLongPressHandled = true;
}

void extractorLongPressCallback(TimerHandle_t xTimer)
{
  printf("EXTRACTOR LONG PRESS CALLBACK\n");
  Interaction action = EXTRACTOR_LONG_PRESS;
  xQueueSendFromISR(interactionQueue, &action, NULL);
  extractorLongPressHandled = true;
}

void initInteraction()
{
  gpio_init(ENCODER_CLK_GPIO);
  gpio_init(ENCODER_DC_GPIO);
  gpio_set_dir(ENCODER_CLK_GPIO, GPIO_IN);
  gpio_set_dir(ENCODER_DC_GPIO, GPIO_IN);

  gpio_init(ENTER_SW_GPIO);
  gpio_set_dir(ENTER_SW_GPIO, GPIO_IN);

  gpio_init(COMPRESSOR_BUTTON_GPIO);
  gpio_set_dir(COMPRESSOR_BUTTON_GPIO, GPIO_IN);

  gpio_init(EXTRACTOR_BUTTON_GPIO);
  gpio_set_dir(EXTRACTOR_BUTTON_GPIO, GPIO_IN);

  gpio_set_irq_enabled_with_callback(ENCODER_CLK_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &sharedISR);
  gpio_set_irq_enabled(ENCODER_DC_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
  gpio_set_irq_enabled(ENTER_SW_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
  gpio_set_irq_enabled(COMPRESSOR_BUTTON_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
  gpio_set_irq_enabled(EXTRACTOR_BUTTON_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

  interactionQueue = xQueueCreate(50, sizeof(Interaction));

  enterLongPressTimer = xTimerCreate("EnterLongPressTimer", pdMS_TO_TICKS(LONG_PRESS_THRESHOLD), pdFALSE, 0, enterLongPressCallback);
  compressorLongPressTimer = xTimerCreate("CompressorLongPressTimer", pdMS_TO_TICKS(LONG_PRESS_THRESHOLD), pdFALSE, 0, compressorLongPressCallback);
  extractorLongPressTimer = xTimerCreate("ExtractorLongPressTimer", pdMS_TO_TICKS(LONG_PRESS_THRESHOLD), pdFALSE, 0, extractorLongPressCallback);
}

void interactionTask(void *pvParameters)
{
  Interaction interaction = NONE;
  while (1)
  {
    // Wait for data from ISR (blocking until data is available)
    if (xQueueReceive(interactionQueue, &interaction, portMAX_DELAY))
    {
      if (interaction == ENTER)
      {
        printf("ENTER COMMAND\n");
        displayEnter();
      }
      else if (interaction == BACK)
      {
        printf("BACK COMMAND\n");
        displayBack();
      }
      else if (interaction == UP)
      {
        printf("UP COMMAND\n");
        displayUp();
      }
      else if (interaction == DOWN)
      {
        printf("DOWN COMMAND\n");
        displayDown();
      }
      else if (interaction == COMPRESSOR)
      {
        printf("COMPRESSOR COMMAND\n");
        if (g_compressorStatus.compressorOn)
        {
          sendOffCommand();
        }
        else
        {
          sendOnCommand();
        }
      }
      else if (interaction == COMPRESSOR_LONG_PRESS)
      {
        printf("COMPRESSOR MENU COMMAND\n");
        // TODO: compressor settings
      }
      else if (interaction == EXTRACTOR)
      {
        printf("EXTRACTOR COMMAND\n");
        // TODO: turn extractor on off
      }
      else if (interaction == EXTRACTOR_LONG_PRESS)
      {
        printf("EXTRACTOR MENU COMMAND\n");
        // TODO: extractor settings
      }
    }
  }
}