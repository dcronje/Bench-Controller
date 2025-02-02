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
#define WHITE RGB_COLOR8(255, 255, 255) // Max red, max green, max blue
#define BLACK RGB_COLOR8(0, 0, 0)       // No red, no green, no blue

void initDisplay();
void displayUp();
void displayDown();
void displayEnter();
void displayBack();

void displayTask(void *params);

enum DisplayType
{
  HOME,
  SETTINGS_MENU,
  SET_PRESSURE_TIMEOUT_DISPLAY,
  SET_MOTOR_TIMEOUT_DISPLAY,
  SET_RELEASE_TIMEOUT_DISPLAY,
  SET_FAN_SPEED_DISPLAY,
};

extern SemaphoreHandle_t displayDataMutex;

#endif // DISPLAY_H