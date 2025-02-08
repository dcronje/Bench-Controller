#ifndef DISPLAY_H
#define DISPLAY_H

#include "constants.h"
#include <lcdgfx.h>
#include <lcdgfx_gui.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define RED RGB_COLOR8(255, 0, 0)       // Max red, no green, no blue
#define GREEN RGB_COLOR8(0, 255, 0)     // No red, max green, no blue
#define BLUE RGB_COLOR8(0, 0, 255)      // No red, no green, max blue
#define ORANGE RGB_COLOR8(255, 255, 0)  // Max red, some green, no blue
#define PURPLE RGB_COLOR8(255, 0, 255)  // Max red, no green, no blue
#define WHITE RGB_COLOR8(255, 255, 255) // Max red, max green, max blue
#define BLACK RGB_COLOR8(0, 0, 0)       // No red, no green, no blue

void initDisplay();
void displayUp();
void displayDown();
void displayEnter();
void displayBack();
void displayCompressorSettingsMenu();
void displayExtractorSettingsMenu();
void displayLightsSettingsMenu();
void alertCompressor(int flashes);
void alertExtractor(int flashes);
void alertLights(int flashes);

void displayTask(void *params);

enum DisplayType
{
  HOME,
  COMPRESSOR_SETTINGS_MENU,
  EXTRACTOR_SETTINGS_MENU,
  LIGHTS_SETTINGS_MENU,
  SET_PRESSURE_TIMEOUT_DISPLAY,
  SET_MOTOR_TIMEOUT_DISPLAY,
  SET_RELEASE_TIMEOUT_DISPLAY,
  SET_FAN_SPEED_DISPLAY,
  SET_MAX_LIGHTS,
  SET_LIGHT_COOLING,
  COMPRESSOR_STATS,
  EXTRACTOR_STATS,
  LIGHTS_STATS,
};

#endif // DISPLAY_H