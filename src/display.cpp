#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "constants.h"
#include "settings.h"

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

SemaphoreHandle_t displayDataMutex = NULL;
volatile DisplayType currentDisplay = HOME;

static bool flashing = false;

void initDisplay()
{
    display.begin();
    canvas.setMode(CANVAS_MODE_TRANSPARENT);
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
    canvas.clear();
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.setColor(WHITE);
    canvas.printFixed(14, 20, "TARGET TEMP", STYLE_NORMAL);
    canvas.setFreeFont(free_koi12x24);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", (int)currentSettings.targetTemp);
    canvas.setColor(calculateColor(ambientTemp, currentSettings.targetTemp));
    canvas.printFixed(36, 32, buffer, STYLE_NORMAL);
    display.drawCanvas(0, 0, canvas);
}

void renderHome(float chamberTemp, float ambientTemp, float heateTemp)
{
    canvas.clear();

    // Light
    if (lightOn)
    {
        canvas.setColor(WHITE);
    }
    else
    {
        canvas.setColor(RGB_COLOR8(100, 100, 100));
    }
    canvas.drawCircle(5, 5, 1);
    canvas.drawCircle(5, 5, 3);
    canvas.putPixel(2, 2);
    canvas.putPixel(2, 8);
    canvas.putPixel(8, 2);
    canvas.putPixel(8, 8);
    int32_t numberOfPoints = lightIntensity / 10;
    if (numberOfPoints > 0)
    {
        canvas.fillRect(0, 12, numberOfPoints - 1, 13);
    }
    int32_t remainder = lightIntensity % 10;
    if (remainder > 0)
    {
        int32_t intensity = ((lightOn ? 255 : 100) * remainder) / 10;
        canvas.setColor(RGB_COLOR8(intensity, intensity, intensity));
        canvas.putPixel(numberOfPoints, 12);
        canvas.putPixel(numberOfPoints, 13);
    }

    // On indicator
    if (isOn)
    {
        if ((int)chamberTemp == (int)currentSettings.targetTemp)
        {
            canvas.setColor(GREEN);
        }
        else if ((int)chamberTemp > (int)currentSettings.targetTemp)
        {
            canvas.setColor(BLUE);
            int32_t numberOfPoints = fanIntensity / 10;
            if (numberOfPoints > 0)
            {
                canvas.fillRect(86, 12, 86 + numberOfPoints - 1, 13);
            }
            int32_t remainder = fanIntensity % 10;
            if (remainder > 0)
            {
                int32_t intensity = (255 * remainder) / 10;
                canvas.setColor(RGB_COLOR8(0, 0, intensity));
                canvas.putPixel(86 + numberOfPoints, 12);
                canvas.putPixel(86 + numberOfPoints, 13);
            }
        }
        else if ((int)chamberTemp < (int)currentSettings.targetTemp)
        {
            canvas.setColor(RED);
            int32_t numberOfPoints = elementIntensity / 10;
            if (numberOfPoints > 0)
            {
                canvas.fillRect(86, 12, 86 + numberOfPoints - 1, 13);
            }
            int32_t remainder = elementIntensity % 10;
            if (remainder > 0)
            {
                int32_t intensity = (255 * remainder) / 10;
                canvas.setColor(RGB_COLOR8(intensity, 0, 0));
                canvas.putPixel(86 + numberOfPoints, 12);
                canvas.putPixel(86 + numberOfPoints, 13);
            }
        }
    }
    else
    {
        canvas.setColor(RGB_COLOR8(100, 100, 100));
    }
    canvas.drawCircle(91, 5, 4);
    canvas.drawLine(91, 2, 91, 8);

    char buffer[32];
    // Chamber
    canvas.setFreeFont(free_koi12x24);
    if (chamberTemp == -1000)
    {
        snprintf(buffer, sizeof(buffer), "00");
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%d", (int)chamberTemp);
    }
    canvas.setColor(calculateColor(currentSettings.targetTemp, chamberTemp));
    canvas.fillRect(26, 36, 70, 37);
    if (chamberTemp == -1000 || chamberTemp >= 10)
    {
        canvas.printFixed(36, 12, buffer, STYLE_NORMAL);
        canvas.drawCircle(64, 14, 1);
    }
    else
    {
        canvas.printFixed(42, 12, buffer, STYLE_NORMAL);
        canvas.drawCircle(52, 14, 1);
    }

    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.setFontSpacing(2);
    canvas.setColor(WHITE);
    snprintf(buffer, sizeof(buffer), "%d%%", (int)chamberHumidity);
    if (chamberHumidity < 10)
    {
        canvas.printFixed(40, 44, buffer, STYLE_NORMAL);
    }
    else if (chamberHumidity == 100)
    {
        canvas.printFixed(34, 44, buffer, STYLE_NORMAL);
    }
    else
    {
        canvas.printFixed(38, 44, buffer, STYLE_NORMAL);
    }

    if (tempIsHigh)
    {
        if (flashing)
        {
            canvas.setColor(RED);
            canvas.drawCircle(4, 60, 2);
            flashing = false;
        }
        else
        {
            flashing = true;
        }
    }
    else if (tempIsLow)
    {
        if (flashing)
        {
            canvas.setColor(BLUE);
            canvas.drawCircle(4, 22, 2);
            flashing = false;
        }
        else
        {
            flashing = true;
        }
    }

    display.drawCanvas(0, 0, canvas);
}

void displayUp()
{

    if (currentDisplay == HOME)
    {
        lightIntensity -= 2;
        if (lightIntensity <= 0)
        {
            lightIntensity = 0;
        }
    }
    else if (currentDisplay == MAIN_MENU_OFF)
    {
        mainMenuOff.up();
        mainMenuOff.show(display);
    }
    else if (currentDisplay == MAIN_MENU_ON)
    {
        mainMenuOn.up();
        mainMenuOn.show(display);
    }
    else if (currentDisplay == SETTINGS_MENU)
    {
        settingsMenu.up();
        settingsMenu.show(display);
    }
    else if (currentDisplay == CALIBRATE_MENU)
    {
        calibrateMenu.up();
        calibrateMenu.show(display);
    }
    else if (currentDisplay == SET_TARGET_TEMP)
    {
        currentSettings.targetTemp--;
        if (currentSettings.targetTemp < minTargetTemp)
        {
            currentSettings.targetTemp = minTargetTemp;
        }
    }
    else if (currentDisplay == SET_HIGH_ALARM)
    {
        currentSettings.highAlarm--;
        if (currentSettings.highAlarm < minHighAlarm)
        {
            currentSettings.highAlarm = minHighAlarm;
        }
    }
    else if (currentDisplay == SET_LOW_ALARM)
    {
        currentSettings.lowAlarm--;
        if (currentSettings.lowAlarm < minLowAlarm)
        {
            currentSettings.lowAlarm = minLowAlarm;
        }
    }
    else if (currentDisplay == SET_HEATING_DIFF)
    {
        currentSettings.heatingDiff--;
        if (currentSettings.heatingDiff < minHeatingDiff)
        {
            currentSettings.heatingDiff = minHeatingDiff;
        }
    }
    else if (currentDisplay == SELECT_CALIBRATION_SENSOR)
    {
        if (selectedSensor == MOSFET)
        {
            selectedSensor = HEATER;
        }
        else if (selectedSensor == HEATER)
        {
            selectedSensor = AMBIENT;
        }
        else if (selectedSensor == AMBIENT)
        {
            selectedSensor = CHAMBER;
        }
        else if (selectedSensor = CHAMBER)
        {
            selectedSensor = MOSFET;
        }
    }
}

void displayDown()
{
    if (currentDisplay == HOME)
    {
        lightIntensity += 2;
        if (lightIntensity > 100)
        {
            lightIntensity = 100;
        }
    }
    else if (currentDisplay == MAIN_MENU_OFF)
    {
        mainMenuOff.down();
        mainMenuOff.show(display);
    }
    else if (currentDisplay == MAIN_MENU_ON)
    {
        mainMenuOn.down();
        mainMenuOn.show(display);
    }
    else if (currentDisplay == SETTINGS_MENU)
    {
        settingsMenu.down();
        settingsMenu.show(display);
    }
    else if (currentDisplay == CALIBRATE_MENU)
    {
        calibrateMenu.down();
        calibrateMenu.show(display);
    }
    else if (currentDisplay == SET_TARGET_TEMP)
    {
        currentSettings.targetTemp++;
        if (currentSettings.targetTemp > maxTargetTemp)
        {
            currentSettings.targetTemp = maxTargetTemp;
        }
    }
    else if (currentDisplay == SET_HIGH_ALARM)
    {
        currentSettings.highAlarm++;
        if (currentSettings.highAlarm > maxHighAlarm)
        {
            currentSettings.highAlarm = maxHighAlarm;
        }
    }
    else if (currentDisplay == SET_LOW_ALARM)
    {
        currentSettings.lowAlarm++;
        if (currentSettings.lowAlarm > maxLowAlarm)
        {
            currentSettings.lowAlarm = maxLowAlarm;
        }
    }
    else if (currentDisplay == SET_HEATING_DIFF)
    {
        currentSettings.heatingDiff++;
        if (currentSettings.heatingDiff > maxHeatingDiff)
        {
            currentSettings.heatingDiff = maxHeatingDiff;
        }
    }
    else if (currentDisplay == SELECT_CALIBRATION_SENSOR)
    {
        if (selectedSensor == CHAMBER)
        {
            selectedSensor = AMBIENT;
        }
        else if (selectedSensor == AMBIENT)
        {
            selectedSensor = HEATER;
        }
        else if (selectedSensor == HEATER)
        {
            selectedSensor = MOSFET;
        }
        else if (selectedSensor = MOSFET)
        {
            selectedSensor = CHAMBER;
        }
    }
}

void displayHome()
{
    currentDisplay = HOME;
    renderHome(ambientTemp, chamberTemp, 40.0f);
}

void displaySettingsMenu()
{
    currentDisplay = SETTINGS_MENU;
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    settingsMenu.show(display);
}

void displayCalibrateMenu()
{
    currentDisplay = CALIBRATE_MENU;
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    calibrateMenu.show(display);
}

void displayMainMenuOn()
{
    currentDisplay = MAIN_MENU_ON;
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    mainMenuOn.show(display);
}

void displayMainMenuOff()
{
    currentDisplay = MAIN_MENU_OFF;
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    mainMenuOff.show(display);
}

void displayMainMenu()
{
    if (isOn)
    {
        displayMainMenuOn();
    }
    else
    {
        displayMainMenuOn();
    }
}

void displaySensorCalibration()
{
    currentDisplay = CALIBRATE_SENSORS;
    sensorCalibrationProgress = 0;
    renderCalibrateSensors();
    initiateCalibration(selectedSensor);
}

void displayEnter()
{
    if ((tempIsHigh || tempIsLow) && !alarmAcknowledged)
    {
        alarmAcknowledged = true;
    }
    else if (currentDisplay == HOME)
    {
        lightOn = !lightOn;
    }
    else if (currentDisplay == MAIN_MENU_ON)
    {
        uint8_t selection = mainMenuOn.selection();
        if (selection == 0)
        {
            isOn = !isOn;
            displayHome();
        }
        else if (selection == 1)
        {
            displaySettingsMenu();
        }
        else if (selection == 2)
        {
            // TODO: info
        }
        else if (selection == 3)
        {
            displayCalibrateMenu();
        }
        else
        {
            displayBack();
        }
    }
    else if (currentDisplay == MAIN_MENU_OFF)
    {
        uint8_t selection = mainMenuOff.selection();
        if (selection == 0)
        {
            isOn = !isOn;
            displayHome();
        }
        else if (selection == 1)
        {
            displaySettingsMenu();
        }
        else if (selection == 2)
        {
            // TODO: info
        }
        else if (selection == 3)
        {
            displayCalibrateMenu();
        }
        else
        {
            displayBack();
        }
    }
    else if (currentDisplay == SETTINGS_MENU)
    {
        uint8_t selection = settingsMenu.selection();
        if (selection == 0)
        {
            currentDisplay = SET_TARGET_TEMP;
        }
        else if (selection == 1)
        {
            currentDisplay = SET_HIGH_ALARM;
        }
        else if (selection == 2)
        {
            currentDisplay = SET_LOW_ALARM;
        }
        else if (selection == 3)
        {
            currentDisplay = SET_HEATING_DIFF;
        }
        else
        {
            displayBack();
        }
    }
    else if (currentDisplay == CALIBRATE_MENU)
    {
        uint8_t selection = calibrateMenu.selection();
        if (selection == 0)
        {
            currentDisplay = SELECT_CALIBRATION_SENSOR;
        }
        else if (selection == 1)
        {
            currentDisplay = CALIBRATE_HEATER;
        }
        else
        {
            displayBack();
        }
    }
    else if (currentDisplay == SELECT_CALIBRATION_SENSOR)
    {
        displaySensorCalibration();
    }
    else if (currentDisplay == SET_TARGET_TEMP || currentDisplay == SET_HIGH_ALARM || currentDisplay == SET_LOW_ALARM || currentDisplay == SET_HEATING_DIFF)
    {
        displayBack();
        checkForSettingsChange();
    }
}

void displayBack()
{
    printf("BACK!");
    if (currentDisplay == HOME)
    {
        displayMainMenu();
    }
    else if (currentDisplay == MAIN_MENU_OFF || currentDisplay == MAIN_MENU_ON)
    {
        displayHome();
    }
    else if (currentDisplay == SETTINGS_MENU)
    {
        displayMainMenu();
    }
    else if (currentDisplay == CALIBRATE_MENU)
    {
        displayMainMenu();
    }
    else if (currentDisplay == SELECT_CALIBRATION_SENSOR)
    {
        displayCalibrateMenu();
    }
    else if (currentDisplay == CALIBRATE_SENSORS)
    {
        displayHome();
    }
    else if (currentDisplay == CALIBRATE_HEATER)
    {
        // TODO:
    }
    else if (currentDisplay == SET_TARGET_TEMP || currentDisplay == SET_HIGH_ALARM || currentDisplay == SET_LOW_ALARM || currentDisplay == SET_HEATING_DIFF)
    {
        displaySettingsMenu();
        checkForSettingsChange();
    }
}

void displayTask(void *params)
{
    while (true)
    {
        if (currentDisplay == HOME)
        {
            static float localChamberTemp, localAmbientTemp;

            if (xSemaphoreTake(displayDataMutex, portMAX_DELAY))
            {
                localChamberTemp = chamberTemp;
                localAmbientTemp = ambientTemp;
                xSemaphoreGive(displayDataMutex);
            }
            if (currentDisplay == HOME)
            {
                renderHome(localChamberTemp, localAmbientTemp, 40.0f);
            }
        }
        else if (currentDisplay == SET_TARGET_TEMP)
        {
            renderTargetTemp();
        }
        else if (currentDisplay == SET_HIGH_ALARM)
        {
            renderHighAlarm();
        }
        else if (currentDisplay == SET_LOW_ALARM)
        {
            renderLowAlarm();
        }
        else if (currentDisplay == SET_HEATING_DIFF)
        {
            renderHeatingDiff();
        }
        else if (currentDisplay == SELECT_CALIBRATION_SENSOR)
        {
            renderSelectSensor();
        }
        else if (currentDisplay == CALIBRATE_SENSORS)
        {
            renderCalibrateSensors();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}