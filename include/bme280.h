#ifndef BME280_H
#define BME280_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

class BME280
{
public:
  BME280(i2c_inst_t *i2cPort, uint8_t addr = 0x76);
  void init();
  bool readAll(float *temperature, float *humidity, float *pressure);

private:
  i2c_inst_t *i2cPort;
  uint8_t address;
  struct CalibrationData
  {
    uint16_t digT1;
    int16_t digT2;
    int16_t digT3;
    uint16_t digP1;
    int16_t digP2;
    int16_t digP3;
    int16_t digP4;
    int16_t digP5;
    int16_t digP6;
    int16_t digP7;
    int16_t digP8;
    int16_t digP9;
    uint8_t digH1;
    int16_t digH2;
    uint8_t digH3;
    int16_t digH4;
    int16_t digH5;
    int8_t digH6;
  } calibData;

  int readBytes(uint8_t regAddr, uint8_t *buffer, uint8_t len);
  int writeByte(uint8_t regAddr, uint8_t value);
  void loadCalibrationData();
};

#endif
