#include "display.h"
#include "constants.h"
#include "settings.h"
#include "wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "FreeRTOS.h"
#include "timers.h"

// Define the SPI configuration
SPlatformSpiConfig spiConfig = {
    .cs = {DISPLAY_SPI_CS_GPIO}, // Chip Select Pin
    .dc = DISPLAY_SPI_DC_GPIO,   // Data/Command Pin
    .frequency = 10000000,       // SPI Frequency (6 MHz)
    .scl = DISPLAY_SPI_CLK_GPIO, // SCK Pin
    .sda = DISPLAY_SPI_MOSI_GPIO // MOSI Pin
};

const int canvasWidth = 96;  // Width
const int canvasHeight = 64; // Height
uint8_t canvasData[canvasWidth * canvasHeight];

DisplaySSD1331_96x64x8_SPI display(DISPLAY_SPI_RST_GPIO, spiConfig);
NanoCanvas8 canvas(canvasWidth, canvasHeight, canvasData);
const char *settingsMenuItems[] = {
    "Set Pressure Timeout",
    "Set Motor Timeout",
    "Set Release Timeout",
    "Set Fan Speed",
    "Set Lights",
};
static LcdGfxMenu settingsMenu(settingsMenuItems, sizeof(settingsMenuItems) / sizeof(char *), (NanoRect){{0, 0}, {0, 0}});

volatile DisplayType currentDisplay = HOME;

static bool flash = true;

void flashTimerCallback(TimerHandle_t xTimer)
{
  flash = !flash;
}

void initDisplay()
{
  display.begin();
  canvas.setMode(CANVAS_MODE_TRANSPARENT);
  TimerHandle_t xFlashTimer = xTimerCreate(
      "FlashTimer",        // Timer name
      pdMS_TO_TICKS(1000), // Timer period in ticks (1000 ms)
      pdTRUE,              // Auto-reload
      (void *)0,           // Timer ID
      flashTimerCallback   // Callback function
  );

  if (xFlashTimer == NULL)
  {
    printf("COULD NOT CREATE FLASH TIMER");
  }
  else
  {
    if (xTimerStart(xFlashTimer, 0) != pdPASS)
    {
      printf("COULD NOT START FLASH TIMER");
    }
  }
}

void getRgb(uint8_t color, uint8_t &r, uint8_t &g, uint8_t &b)
{
  r = (color & 0xE0);      // Extract red (bits 7-5, 3 bits)
  g = (color & 0x1C) << 3; // Extract green (bits 4-2, shift left to fill 8 bits)
  b = (color & 0x03) << 6; // Extract blue (bits 1-0, shift left to fill 8 bits)
}

// Function to interpolate between two colors
uint8_t interpolateColor(uint32_t colorA, uint32_t colorB, int progress)
{
  // Normalize progress to a value between 0.0 and 1.0
  float normalizedProgress = static_cast<float>(progress) / 100.0f;

  // Extract RGB components from both colors
  uint8_t rA, gA, bA;
  uint8_t rB, gB, bB;
  getRgb(colorA, rA, gA, bA);
  getRgb(colorB, rB, gB, bB);

  printf("RA: %d GA: %d BA: %d\n", rA, gA, bA);
  printf("RB: %d GB: %d BB: %d\n", rB, gB, bB);

  // Interpolate the RGB values
  uint8_t r = static_cast<uint8_t>(rA * (1 - normalizedProgress) + rB * normalizedProgress);
  uint8_t g = static_cast<uint8_t>(gA * (1 - normalizedProgress) + gB * normalizedProgress);
  uint8_t b = static_cast<uint8_t>(bA * (1 - normalizedProgress) + bB * normalizedProgress);

  printf("R: %d G: %d B: %d\n", r, g, b);

  // Return the final interpolated color in RGB_COLOR8 format
  return RGB_COLOR8(r, g, b);
}

uint8_t calculateColor(float targetTemperature, float actualTemperature)
{
  const float range = 5.0; // Range for transition (degrees above/below target)
  float difference = actualTemperature - targetTemperature;

  uint8_t red = 0, green = 0, blue = 0;

  if (difference <= -range)
  {
    // Fully blue when 5 or more degrees below target
    red = 0;
    green = 0;
    blue = 255;
  }
  else if (difference >= range)
  {
    // Fully red when 5 or more degrees above target
    red = 255;
    green = 0;
    blue = 0;
  }
  else if (difference < 0)
  {
    // Transition from blue to green (below target)
    float ratio = (range + difference) / range; // Ratio from 0 (blue) to 1 (green)
    red = 0;
    green = static_cast<uint8_t>(255 * ratio);
    blue = static_cast<uint8_t>(255 * (1 - ratio));
  }
  else
  {
    // Transition from green to red (above target)
    float ratio = difference / range; // Ratio from 0 (green) to 1 (red)
    red = static_cast<uint8_t>(255 * ratio);
    green = static_cast<uint8_t>(255 * (1 - ratio));
    blue = 0;
  }

  // Convert to RGB_COLOR8 format (8 bits per color)
  return RGB_COLOR8(red, green, blue);
}

void renderSetPressureTimeout()
{
  // canvas.clear();
  // canvas.setFixedFont(ssd1306xled_font6x8);
  // canvas.setColor(WHITE);
  // canvas.printFixed(14, 20, "TARGET TEMP", STYLE_NORMAL);
  // canvas.setFreeFont(free_koi12x24);
  // char buffer[32];
  // snprintf(buffer, sizeof(buffer), "%d", (int)currentSettings.targetTemp);
  // canvas.setColor(calculateColor(ambientTemp, currentSettings.targetTemp));
  // canvas.printFixed(36, 32, buffer, STYLE_NORMAL);
  // display.drawCanvas(0, 0, canvas);
}

void renderSetMotorTimeout()
{
  // canvas.clear();
  // canvas.setFixedFont(ssd1306xled_font6x8);
  // canvas.setColor(WHITE);
  // canvas.printFixed(14, 20, "TARGET TEMP", STYLE_NORMAL);
  // canvas.setFreeFont(free_koi12x24);
  // char buffer[32];
  // snprintf(buffer, sizeof(buffer), "%d", (int)currentSettings.targetTemp);
  // canvas.setColor(calculateColor(ambientTemp, currentSettings.targetTemp));
  // canvas.printFixed(36, 32, buffer, STYLE_NORMAL);
  // display.drawCanvas(0, 0, canvas);
}

void renderSetReleaseTimeout()
{
  // canvas.clear();
  // canvas.setFixedFont(ssd1306xled_font6x8);
  // canvas.setColor(WHITE);
  // canvas.printFixed(14, 20, "TARGET TEMP", STYLE_NORMAL);
  // canvas.setFreeFont(free_koi12x24);
  // char buffer[32];
  // snprintf(buffer, sizeof(buffer), "%d", (int)currentSettings.targetTemp);
  // canvas.setColor(calculateColor(ambientTemp, currentSettings.targetTemp));
  // canvas.printFixed(36, 32, buffer, STYLE_NORMAL);
  // display.drawCanvas(0, 0, canvas);
}

void renderSetFanSpeed()
{
  // canvas.clear();
  // canvas.setFixedFont(ssd1306xled_font6x8);
  // canvas.setColor(WHITE);
  // canvas.printFixed(14, 20, "TARGET TEMP", STYLE_NORMAL);
  // canvas.setFreeFont(free_koi12x24);
  // char buffer[32];
  // snprintf(buffer, sizeof(buffer), "%d", (int)currentSettings.targetTemp);
  // canvas.setColor(calculateColor(ambientTemp, currentSettings.targetTemp));
  // canvas.printFixed(36, 32, buffer, STYLE_NORMAL);
  // display.drawCanvas(0, 0, canvas);
}

void renderCompressor()
{
  switch (networkStatus)
  {
  case NetworkStatus::STARTUP:
    canvas.setColor(RED);
    canvas.drawRect(0, 0, 30, 32);
    canvas.setColor(WHITE);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(3, 8, "WIFI", STYLE_NORMAL);
    canvas.setFixedFont(ssd1306xled_font5x7);
    canvas.printFixed(5, 19, "INIT", STYLE_NORMAL);
    break;
  case NetworkStatus::ERROR:
    canvas.setColor(RED);
    canvas.drawRect(0, 0, 30, 32);
    canvas.setColor(WHITE);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(3, 8, "WIFI", STYLE_NORMAL);
    if (flash)
    {
      canvas.setColor(RED);
      canvas.setFixedFont(ssd1306xled_font5x7);
      canvas.printFixed(2, 19, "ERROR", STYLE_NORMAL);
    }
    break;
  case NetworkStatus::AP_MODE:
    canvas.setColor(RED);
    canvas.drawRect(0, 0, 30, 32);
    canvas.setColor(WHITE);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(3, 8, "WIFI", STYLE_NORMAL);
    if (flash)
    {
      canvas.setColor(BLUE);
      canvas.setFixedFont(ssd1306xled_font5x7);
      canvas.printFixed(2, 19, "CREDS", STYLE_NORMAL);
    }

    break;
  case NetworkStatus::CLIENT_CONNECTED:
    // TODO: Compressor
    break;
  case NetworkStatus::SOCKET_RUNNING:
    canvas.setColor(RED);
    canvas.drawRect(0, 0, 30, 32);
    canvas.setColor(WHITE);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(3, 8, "SOCT", STYLE_NORMAL);
    if (flash)
    {
      canvas.setColor(GREEN);
      canvas.setFixedFont(ssd1306xled_font5x7);
      canvas.printFixed(3, 19, "WAIT", STYLE_NORMAL);
    }

    break;
  case NetworkStatus::WIFI_CONNECTED:
    canvas.setColor(RED);
    canvas.drawRect(0, 0, 30, 32);
    canvas.setColor(WHITE);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(3, 8, "SOCT", STYLE_NORMAL);
    canvas.setColor(ORANGE);
    canvas.setFixedFont(ssd1306xled_font5x7);
    canvas.printFixed(2, 19, "START", STYLE_NORMAL);
    break;
  default:
    break;
  }
}

void renderExtractor()
{
  canvas.setColor(ORANGE);
  canvas.drawRect(33, 0, 63, 32);
}

void renderLights()
{
  canvas.setColor(GREEN);
  canvas.drawRect(65, 0, 95, 32);
}

void renderHome()
{
  canvas.clear();
  renderCompressor();
  renderExtractor();
  renderLights();
  // Light
  // if (lightOn)
  // {
  //     canvas.setColor(WHITE);
  // }
  // else
  // {
  //     canvas.setColor(RGB_COLOR8(100, 100, 100));
  // }
  // canvas.drawCircle(5, 5, 1);
  // canvas.drawCircle(5, 5, 3);
  // canvas.putPixel(2, 2);
  // canvas.putPixel(2, 8);
  // canvas.putPixel(8, 2);
  // canvas.putPixel(8, 8);
  // int32_t numberOfPoints = lightIntensity / 10;
  // if (numberOfPoints > 0)
  // {
  //     canvas.fillRect(0, 12, numberOfPoints - 1, 13);
  // }
  // int32_t remainder = lightIntensity % 10;
  // if (remainder > 0)
  // {
  //     int32_t intensity = ((lightOn ? 255 : 100) * remainder) / 10;
  //     canvas.setColor(RGB_COLOR8(intensity, intensity, intensity));
  //     canvas.putPixel(numberOfPoints, 12);
  //     canvas.putPixel(numberOfPoints, 13);
  // }

  // // On indicator
  // if (isOn)
  // {
  //     if ((int)boothTemp == (int)currentSettings.targetTemp)
  //     {
  //         canvas.setColor(GREEN);
  //     }
  //     else if ((int)boothTemp > (int)currentSettings.targetTemp)
  //     {
  //         canvas.setColor(BLUE);
  //         int32_t numberOfPoints = fanIntensity / 10;
  //         if (numberOfPoints > 0)
  //         {
  //             canvas.fillRect(86, 12, 86 + numberOfPoints - 1, 13);
  //         }
  //         int32_t remainder = fanIntensity % 10;
  //         if (remainder > 0)
  //         {
  //             int32_t intensity = (255 * remainder) / 10;
  //             canvas.setColor(RGB_COLOR8(0, 0, intensity));
  //             canvas.putPixel(86 + numberOfPoints, 12);
  //             canvas.putPixel(86 + numberOfPoints, 13);
  //         }
  //     }
  //     else if ((int)boothTemp < (int)currentSettings.targetTemp)
  //     {
  //         canvas.setColor(RED);
  //         int32_t numberOfPoints = elementIntensity / 10;
  //         if (numberOfPoints > 0)
  //         {
  //             canvas.fillRect(86, 12, 86 + numberOfPoints - 1, 13);
  //         }
  //         int32_t remainder = elementIntensity % 10;
  //         if (remainder > 0)
  //         {
  //             int32_t intensity = (255 * remainder) / 10;
  //             canvas.setColor(RGB_COLOR8(intensity, 0, 0));
  //             canvas.putPixel(86 + numberOfPoints, 12);
  //             canvas.putPixel(86 + numberOfPoints, 13);
  //         }
  //     }
  // }
  // else
  // {
  //     canvas.setColor(RGB_COLOR8(100, 100, 100));
  // }
  // canvas.drawCircle(91, 5, 4);
  // canvas.drawLine(91, 2, 91, 8);

  // char buffer[32];
  // // Booth
  // canvas.setFreeFont(free_koi12x24);
  // if (boothTemp == -1000)
  // {
  //     snprintf(buffer, sizeof(buffer), "00");
  // }
  // else
  // {
  //     snprintf(buffer, sizeof(buffer), "%d", (int)boothTemp);
  // }
  // canvas.setColor(calculateColor(currentSettings.targetTemp, boothTemp));
  // canvas.fillRect(26, 36, 70, 37);
  // if (boothTemp == -1000 || boothTemp >= 10)
  // {
  //     canvas.printFixed(36, 12, buffer, STYLE_NORMAL);
  //     canvas.drawCircle(64, 14, 1);
  // }
  // else
  // {
  //     canvas.printFixed(42, 12, buffer, STYLE_NORMAL);
  //     canvas.drawCircle(52, 14, 1);
  // }

  // canvas.setFixedFont(ssd1306xled_font6x8);
  // canvas.setFontSpacing(2);
  // canvas.setColor(WHITE);
  // snprintf(buffer, sizeof(buffer), "%d%%", (int)boothHumidity);
  // if (boothHumidity < 10)
  // {
  //     canvas.printFixed(40, 44, buffer, STYLE_NORMAL);
  // }
  // else if (boothHumidity == 100)
  // {
  //     canvas.printFixed(34, 44, buffer, STYLE_NORMAL);
  // }
  // else
  // {
  //     canvas.printFixed(38, 44, buffer, STYLE_NORMAL);
  // }

  // if (tempIsHigh)
  // {
  //     if (flashing)
  //     {
  //         canvas.setColor(RED);
  //         canvas.drawCircle(4, 60, 2);
  //         flashing = false;
  //     }
  //     else
  //     {
  //         flashing = true;
  //     }
  // }
  // else if (tempIsLow)
  // {
  //     if (flashing)
  //     {
  //         canvas.setColor(BLUE);
  //         canvas.drawCircle(4, 22, 2);
  //         flashing = false;
  //     }
  //     else
  //     {
  //         flashing = true;
  //     }
  // }

  display.drawCanvas(0, 0, canvas);
}

void displayUp()
{

  if (currentDisplay == HOME)
  {
    // lightIntensity -= 2;
    // if (lightIntensity <= 0)
    // {
    //     lightIntensity = 0;
    // }
  }
  if (currentDisplay == SETTINGS_MENU)
  {
    settingsMenu.up();
    settingsMenu.show(display);
  }
  else if (currentDisplay == SET_PRESSURE_TIMEOUT_DISPLAY)
  {
    // currentSettings.targetTemp--;
    // if (currentSettings.targetTemp < minTargetTemp)
    // {
    //     currentSettings.targetTemp = minTargetTemp;
    // }
  }
  else if (currentDisplay == SET_MOTOR_TIMEOUT_DISPLAY)
  {
    // currentSettings.targetTemp--;
    // if (currentSettings.targetTemp < minTargetTemp)
    // {
    //     currentSettings.targetTemp = minTargetTemp;
    // }
  }
  else if (currentDisplay == SET_RELEASE_TIMEOUT_DISPLAY)
  {
    // currentSettings.targetTemp--;
    // if (currentSettings.targetTemp < minTargetTemp)
    // {
    //     currentSettings.targetTemp = minTargetTemp;
    // }
  }
  else if (currentDisplay == SET_FAN_SPEED_DISPLAY)
  {
    // currentSettings.targetTemp--;
    // if (currentSettings.targetTemp < minTargetTemp)
    // {
    //     currentSettings.targetTemp = minTargetTemp;
    // }
  }
}

void displayDown()
{
  if (currentDisplay == HOME)
  {
    // lightIntensity += 2;
    // if (lightIntensity > 100)
    // {
    //     lightIntensity = 100;
    // }
  }
  else if (currentDisplay == SETTINGS_MENU)
  {
    settingsMenu.down();
    settingsMenu.show(display);
  }
  else if (currentDisplay == SET_PRESSURE_TIMEOUT_DISPLAY)
  {
    // currentSettings.targetTemp++;
    // if (currentSettings.targetTemp > maxTargetTemp)
    // {
    //     currentSettings.targetTemp = maxTargetTemp;
    // }
  }
  else if (currentDisplay == SET_MOTOR_TIMEOUT_DISPLAY)
  {
    // currentSettings.targetTemp++;
    // if (currentSettings.targetTemp > maxTargetTemp)
    // {
    //     currentSettings.targetTemp = maxTargetTemp;
    // }
  }
  else if (currentDisplay == SET_RELEASE_TIMEOUT_DISPLAY)
  {
    // currentSettings.targetTemp++;
    // if (currentSettings.targetTemp > maxTargetTemp)
    // {
    //     currentSettings.targetTemp = maxTargetTemp;
    // }
  }
  else if (currentDisplay == SET_FAN_SPEED_DISPLAY)
  {
    // currentSettings.targetTemp++;
    // if (currentSettings.targetTemp > maxTargetTemp)
    // {
    //     currentSettings.targetTemp = maxTargetTemp;
    // }
  }
}

void displayHome()
{
  currentDisplay = HOME;
  renderHome();
}

void displaySettingsMenu()
{
  currentDisplay = SETTINGS_MENU;
  display.setFixedFont(ssd1306xled_font6x8);
  display.clear();
  settingsMenu.show(display);
}

void displayEnter()
{
  if (currentDisplay == HOME)
  {
    // lightOn = !lightOn;
  }
  else if (currentDisplay == SETTINGS_MENU)
  {
    uint8_t selection = settingsMenu.selection();
    if (selection == 0)
    {
      currentDisplay = SET_PRESSURE_TIMEOUT_DISPLAY;
    }
    else if (selection == 1)
    {
      currentDisplay = SET_MOTOR_TIMEOUT_DISPLAY;
    }
    else if (selection == 2)
    {
      currentDisplay = SET_RELEASE_TIMEOUT_DISPLAY;
    }
    else if (selection == 3)
    {
      currentDisplay = SET_FAN_SPEED_DISPLAY;
    }
    else
    {
      displayBack();
    }
  }
  else if (currentDisplay == SET_PRESSURE_TIMEOUT_DISPLAY || currentDisplay == SET_MOTOR_TIMEOUT_DISPLAY || currentDisplay == SET_RELEASE_TIMEOUT_DISPLAY || currentDisplay == SET_FAN_SPEED_DISPLAY)
  {
    displayBack();
    // checkForSettingsChange();
  }
}

void displayBack()
{
  printf("BACK!");
  if (currentDisplay == HOME)
  {
    displaySettingsMenu();
  }
  else if (currentDisplay == SETTINGS_MENU)
  {
    displayHome();
  }
  else if (currentDisplay == SET_PRESSURE_TIMEOUT_DISPLAY || currentDisplay == SET_MOTOR_TIMEOUT_DISPLAY || currentDisplay == SET_RELEASE_TIMEOUT_DISPLAY || currentDisplay == SET_FAN_SPEED_DISPLAY)
  {
    displaySettingsMenu();
    // checkForSettingsChange();
  }
}

void displayTask(void *params)
{
  while (true)
  {
    if (currentDisplay == HOME)
    {
      if (currentDisplay == HOME)
      {
        renderHome();
      }
    }
    else if (currentDisplay == SET_PRESSURE_TIMEOUT_DISPLAY)
    {
      renderSetPressureTimeout();
    }
    else if (currentDisplay == SET_MOTOR_TIMEOUT_DISPLAY)
    {
      renderSetMotorTimeout();
    }
    else if (currentDisplay == SET_RELEASE_TIMEOUT_DISPLAY)
    {
      renderSetReleaseTimeout();
    }
    else if (currentDisplay == SET_FAN_SPEED_DISPLAY)
    {
      renderSetFanSpeed();
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}