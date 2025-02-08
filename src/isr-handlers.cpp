#include "isr-handlers.h"
#include "constants.h"
#include "interaction.h"
#include "extractor.h"

#include "FreeRTOS.h"

void sharedISR(uint gpio, uint32_t events)
{
  if (gpio == ENCODER_CLK_GPIO || gpio == ENCODER_DC_GPIO)
  {
    handleEncoderISR(gpio, events);
  }
  else if (gpio == ENTER_SW_GPIO || gpio == COMPRESSOR_BUTTON_GPIO || gpio == EXTRACTOR_BUTTON_GPIO)
  {
    handleButtonISR(gpio, events);
  }
  else if (gpio == EXTRACTOR_TACH_GPIO)
  {
    tachometerISR(gpio, events);
  }
}
