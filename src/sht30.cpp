#include "sht30.h"

#include <stdio.h>
#include <math.h>

SHT30::SHT30(i2c_inst_t *i2cPort, uint8_t addr) : i2cPort(i2cPort), address(addr) {}

void SHT30::init()
{
  this->sendCommand(0x2C06); // High repeatability measurement command
}

bool SHT30::readAll(float *temperature, float *humidity)
{
  uint8_t buffer[6];
  if (!this->sendCommand(0x2C06))
  { // Command to start a high repeatability measurement
    printf("Failed to start measurement\n");
    return false;
  }

  sleep_ms(500); // Delay for measurement to complete

  if (!this->readSensorData(buffer, 6))
  {
    printf("Failed to read data\n");
    return false;
  }

  // Process the raw data from the sensor
  uint16_t tempRaw = (buffer[0] << 8) | buffer[1];
  uint16_t humRaw = (buffer[3] << 8) | buffer[4];

  *temperature = -45.0 + 175.0 * ((float)tempRaw / 65535.0); // Convert to Celsius
  *humidity = 100.0 * ((float)humRaw / 65535.0);             // Convert to percentage

  return true;
}

bool SHT30::sendCommand(uint16_t cmd)
{
  uint8_t buffer[2] = {static_cast<uint8_t>(cmd >> 8), static_cast<uint8_t>(cmd & 0xFF)};
  return i2c_write_blocking(this->i2cPort, this->address, buffer, 2, false) == 2;
}

bool SHT30::readSensorData(uint8_t *data, uint8_t len)
{
  return i2c_read_blocking(this->i2cPort, this->address, data, len, false) == len;
}
