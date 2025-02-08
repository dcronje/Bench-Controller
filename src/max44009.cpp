#include "max44009.h"

MAX44009::MAX44009(i2c_inst_t *i2cPort, uint8_t addr) : i2cPort(i2cPort), address(addr) {}

void MAX44009::init()
{
  // Optional: configure the sensor, if necessary
  this->configureIntegrationTime(400);
}

void MAX44009::configureIntegrationTime(uint16_t time_ms)
{
  uint8_t configValue = 0;

  switch (time_ms)
  {
  case 800:
    configValue = 0x00; // Default value
    break;
  case 400:
    configValue = 0x01;
    break;
  case 200:
    configValue = 0x02;
    break;
  case 100:
    configValue = 0x03;
    break;
  case 50:
    configValue = 0x04;
    break;
  case 25:
    configValue = 0x05;
    break;
  case 12:
    configValue = 0x06;
    break;
  case 6:
    configValue = 0x07;
    break;
  default:
    configValue = 0x00; // Default value
    break;
  }

  // Write configuration value to register 0x02
  this->writeByte(0x02, configValue);
}

float MAX44009::readLux()
{
  uint8_t luxBytes[2];
  if (this->readBytes(0x03, luxBytes, 2) != 2)
  {
    return -1.0; // Error handling if data read fails
  }
  uint16_t exponent = (luxBytes[0] >> 4) & 0x0F;
  uint16_t mantissa = ((luxBytes[0] & 0x0F) << 4) | (luxBytes[1] & 0x0F);
  return mantissa * (1 << exponent) * 0.045; // Correct calculation as per datasheet
}

int MAX44009::writeByte(uint8_t regAddr, uint8_t value)
{
  uint8_t data[2] = {regAddr, value};
  return i2c_write_blocking(this->i2cPort, this->address, data, 2, false);
}

int MAX44009::readBytes(uint8_t regAddr, uint8_t *buffer, uint8_t len)
{
  int result = i2c_write_blocking(this->i2cPort, this->address, &regAddr, 1, true);
  if (result != 1)
  {
    return -1; // Error
  }
  result = i2c_read_blocking(this->i2cPort, this->address, buffer, len, false);
  return result;
}
