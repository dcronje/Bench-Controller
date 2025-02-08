#include "sensors.h"
#include "constants.h"
#include "control.h"
#include "max44009.h"
#include "sht30.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "one_wire.h"

volatile float lightsATemp = 0.0f;
volatile float lightsBTemp = 0.0f;
volatile float lightsCTemp = 0.0f;

volatile float boothTemp = 0.0f;
volatile float boothHumidity = 0.0f;
volatile float boothLux = 0.0f;

One_wire lightsATempSensor(LIGHTS_A_TEMP_GPIO); // Internal sensor 1
One_wire lightsBTempSensor(LIGHTS_B_TEMP_GPIO); // Internal sensor 2
One_wire lightsCTempSensor(LIGHTS_C_TEMP_GPIO); // Internal sensor 3

MAX44009 max44009(SENSOR_I2C_PORT, MAX44009_I2C_ADDR);
SHT30 sht30(SENSOR_I2C_PORT, SHT30_I2C_ADDR);

void initSensors(void)
{
  i2c_init(SENSOR_I2C_PORT, SENSORS_I2C_FREQ);
  gpio_set_function(SENSORS_I2C_SDA_GPIO, GPIO_FUNC_I2C);
  gpio_set_function(SENSORS_I2C_SCL_GPIO, GPIO_FUNC_I2C);
  gpio_pull_up(SENSORS_I2C_SDA_GPIO); // Uncomment if needed
  gpio_pull_up(SENSORS_I2C_SCL_GPIO); // Uncomment if needed
}

float readLightsATemperature()
{
  rom_address_t address{};
  lightsATempSensor.single_device_read_rom(address);
  lightsATempSensor.convert_temperature(address, true, false);
  return lightsATempSensor.temperature(address);
}

float readLightsBTemperature()
{
  rom_address_t address{};
  lightsBTempSensor.single_device_read_rom(address);
  lightsBTempSensor.convert_temperature(address, true, false);
  return lightsBTempSensor.temperature(address);
}

float readLightsCTemperature()
{
  rom_address_t address{};
  lightsCTempSensor.single_device_read_rom(address);
  lightsCTempSensor.convert_temperature(address, true, false);
  return lightsCTempSensor.temperature(address);
}

void tempSensorTask(void *params)
{
  sht30.init();
  lightsATempSensor.init();
  lightsBTempSensor.init();
  lightsCTempSensor.init();

  while (1)
  {
    float localLightsATemp = readLightsATemperature();
    float localLightsBTemp = readLightsBTemperature();
    float localLightsCTemp = readLightsCTemperature();
    if (localLightsATemp != -1000)
    {
      lightsATemp = localLightsATemp;
    }
    if (localLightsBTemp != -1000)
    {
      lightsBTemp = localLightsBTemp;
    }
    if (localLightsCTemp != -1000)
    {
      lightsCTemp = localLightsCTemp;
    }

    float localBoothTemp = 0.0f;
    float localBoothHumidity = 0.0f;

    if (sht30.readAll(&localBoothTemp, &localBoothHumidity))
    {
      boothTemp = localBoothTemp;
      boothHumidity = localBoothHumidity;
    }

    printf("Temperatures A: %.2f B: %.2f C: %.2f, Temp: %.2f, Humidity: %.2f\n", lightsATemp, lightsBTemp, lightsCTemp, boothTemp, boothHumidity);

    vTaskDelay(5000); // Delay
  }
}

void lightSensorTask(void *params)
{
  max44009.init();
  while (1)
  {
    boothLux = max44009.readLux();

    // printf("Lux: %.2f\n", boothLux);
    vTaskDelay(500); // Delay
  }
}