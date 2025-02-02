#include "sensors.h"
#include "constants.h"
#include "control.h"

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

One_wire lightsATempSensor(LIGHTS_A_TEMP_GPIO); // Internal sensor 2
One_wire lightsBTempSensor(LIGHTS_B_TEMP_GPIO); // Internal sensor 2
One_wire lightsCTempSensor(LIGHTS_B_TEMP_GPIO); // Internal sensor 2

void initSensors(void)
{
  lightsATempSensor.init();
  lightsBTempSensor.init();
  lightsCTempSensor.init();

  i2c_init(SENSOR_I2C_PORT, SHT30_I2C_FREQ); // Use the frequency constant
  gpio_set_function(SHT30_I2C_SDA_GPIO, GPIO_FUNC_I2C);
  gpio_set_function(SHT30_I2C_SCL_GPIO, GPIO_FUNC_I2C);
  gpio_pull_up(SHT30_I2C_SDA_GPIO);
  gpio_pull_up(SHT30_I2C_SCL_GPIO);

  uint8_t buffer[2];
  buffer[0] = SHT30_CMD_MEASURE_HIGHREP >> 8;
  buffer[1] = SHT30_CMD_MEASURE_HIGHREP & 0xFF;

  int result = i2c_write_blocking(SENSOR_I2C_PORT, SHT30_I2C_ADDR, buffer, 2, false);

  // Wake up the sensor
  buffer[0] = TSL2561_CMD_BIT | TSL2561_CONTROL;
  buffer[1] = 0x03;
  i2c_write_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 2, false);

  // Set integration time and gain
  buffer[0] = TSL2561_CMD_BIT | TSL2561_TIMING;
  buffer[1] = 0x02; // 402ms integration time
  i2c_write_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 2, false);

  // Disable the interrupt
  buffer[0] = TSL2561_CMD_BIT | TSL2561_INTERRUPT;
  buffer[1] = TSL2561_INTERRUPT_DISABLE;
  i2c_write_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 2, false);
}

float calculateLux(uint16_t ch0, uint16_t ch1)
{
  float ratio = (ch1 / (float)ch0);
  float lux = 0.0;

  // Use coefficients to calculate lux based on the ratio of infrared to visible light
  if (ratio <= 0.50)
  {
    lux = (0.0304 * ch0) - (0.062 * ch0 * pow(ratio, 1.4));
  }
  else if (ratio <= 0.61)
  {
    lux = (0.0224 * ch0) - (0.031 * ch1);
  }
  else if (ratio <= 0.80)
  {
    lux = (0.0128 * ch0) - (0.0153 * ch1);
  }
  else if (ratio <= 1.30)
  {
    lux = (0.00146 * ch0) - (0.00112 * ch1);
  }
  else
  {
    lux = 0;
  }

  return lux;
}

uint32_t readBoothLight()
{
  uint8_t buffer[4];
  uint32_t ch0, ch1;

  // Read channel 0
  buffer[0] = TSL2561_CMD_BIT | TSL2561_CHANNEL0_LOW;
  i2c_write_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 1, true);
  i2c_read_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 2, false);
  ch0 = (buffer[1] << 8) | buffer[0];

  // Read channel 1
  buffer[0] = TSL2561_CMD_BIT | TSL2561_CHANNEL1_LOW;
  i2c_write_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 1, true);
  i2c_read_blocking(SENSOR_I2C_PORT, TSL2561_ADDRESS, buffer, 2, false);
  ch1 = (buffer[1] << 8) | buffer[0];

  return (ch0 << 16) | ch1; // Return 32-bit reading (CH0 high and low)
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

bool readBoothTemperature(float *temperature, float *humidity)
{
  uint8_t command[2] = {0x2C, 0x06}; // High repeatability measurement
  int commandResult = i2c_write_blocking(SENSOR_I2C_PORT, SHT30_I2C_ADDR, command, 2, false);
  if (commandResult < 0)
  {
    printf("Failed to send command\n");
    *temperature = -1000;
    *humidity = -100;
    return false;
  }

  uint8_t buffer[6];
  int result = i2c_read_blocking(SENSOR_I2C_PORT, SHT30_I2C_ADDR, buffer, 6, false);
  if (result < 0)
  {
    printf("I2C read failed\n");
    *temperature = -1000;
    *humidity = -100;
    return false;
  }

  // Convert the raw data
  uint16_t tempRaw = (buffer[0] << 8) | buffer[1];
  uint16_t humRaw = (buffer[3] << 8) | buffer[4];

  *temperature = -45.0 + 175.0 * ((float)tempRaw / 65535.0);
  *humidity = 100.0 * ((float)humRaw / 65535.0);

  return true;
}

void sensorTask(void *params)
{
  const TickType_t xDelay = pdMS_TO_TICKS(500); // 1000 ms delay between readings
  while (1)
  {

    lightsATemp = readLightsATemperature();
    lightsBTemp = readLightsBTemperature();
    lightsCTemp = readLightsCTemperature();

    float localBoothTemp = 0.0f;
    float localBoothHumidity = 0.0f;

    readBoothTemperature(&localBoothTemp, &localBoothHumidity);

    if (localBoothTemp != -1000)
    {
      boothTemp = localBoothTemp;
    }

    if (localBoothHumidity != -1000)
    {
      boothHumidity = localBoothHumidity;
    }

    uint32_t lightData = readBoothLight();
    uint16_t ch0 = lightData >> 16;    // Extract the high 16 bits for channel 0
    uint16_t ch1 = lightData & 0xFFFF; // Extract the low 16 bits for channel 1
    boothLux = calculateLux(ch0, ch1);

    printf("Temperatures A: %.2f B: %.2f C: %.2f LUX: %.2f", lightsATemp, lightsBTemp, lightsCTemp, boothLux);
    vTaskDelay(xDelay); // Delay
  }
}