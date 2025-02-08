#ifndef MAX44009_H
#define MAX44009_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

class MAX44009
{
public:
  MAX44009(i2c_inst_t *i2cPort, uint8_t addr = 0x4A);
  void init();
  void configureIntegrationTime(uint16_t time_ms);
  float readLux();

private:
  i2c_inst_t *i2cPort;
  uint8_t address;

  int readBytes(uint8_t regAddr, uint8_t *buffer, uint8_t len);
  int writeByte(uint8_t regAddr, uint8_t value);
};

#endif
