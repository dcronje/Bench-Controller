#ifndef SHT30_H
#define SHT30_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

class SHT30
{
public:
  SHT30(i2c_inst_t *i2cPort, uint8_t addr = 0x44);
  void init();
  bool readAll(float *temperature, float *humidity);

private:
  i2c_inst_t *i2cPort;
  uint8_t address;

  bool sendCommand(uint16_t cmd);
  bool readSensorData(uint8_t *data, uint8_t len);
};

#endif // SHT30_H
