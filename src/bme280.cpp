#include "bme280.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"

BME280::BME280(i2c_inst_t *i2cPort, uint8_t addr) : i2cPort(i2cPort), address(addr) {}

void BME280::init()
{
  this->loadCalibrationData();
  // Increasing oversampling for temperature to x2
  this->writeByte(0xF2, 0x01); // Humidity oversampling x1
  this->writeByte(0xF4, 0x2D); // Temp oversampling x2, Pressure oversampling x1, Normal mode
  this->writeByte(0xF5, 0xA0); // Standby time 1000 ms
}

bool BME280::readAll(float *temperature, float *humidity, float *pressure)
{
  uint8_t data[8];
  if (this->readBytes(0xF7, data, 8) != 8)
  {
    printf("Failed to read data\n");
    return false;
  }

  // Combining data bytes for pressure, temperature, and humidity
  int32_t adc_P = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
  int32_t adc_T = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4);
  int32_t adc_H = ((uint32_t)data[6] << 8) | (uint32_t)data[7];

  // Temperature compensation
  int32_t var1, var2, t_fine;
  var1 = ((((adc_T >> 3) - ((int32_t)calibData.digT1 << 1))) * ((int32_t)calibData.digT2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)calibData.digT1)) * ((adc_T >> 4) - ((int32_t)calibData.digT1))) >> 12) * ((int32_t)calibData.digT3)) >> 14;
  t_fine = var1 + var2;
  *temperature = ((t_fine * 5 + 128) >> 8) / 100.0f;

  // Pressure compensation
  int64_t p;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)calibData.digP6;
  var2 += (var1 * (int64_t)calibData.digP5) << 17;
  var2 += ((int64_t)calibData.digP4) << 35;
  var1 = ((var1 * var1 * (int64_t)calibData.digP3) >> 8) + ((var1 * (int64_t)calibData.digP2) << 12);
  var1 = (((((int64_t)1) << 47) + var1) * (int64_t)calibData.digP1) >> 33;
  if (var1 == 0)
  {
    return false; // Avoid division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)calibData.digP9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)calibData.digP8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)calibData.digP7) << 4);
  *pressure = ((float)p / 25600.0f) - 251.0f; // Convert from Pa to hPa

  // Humidity compensation
  int32_t v_x1_u32r = t_fine - 76800;
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)calibData.digH4) << 20) - (((int32_t)calibData.digH5) * v_x1_u32r)) + 16384) >> 15) *
               (((((((v_x1_u32r * ((int32_t)calibData.digH6)) >> 10) * (((v_x1_u32r * ((int32_t)calibData.digH3)) >> 11) + 32768)) >> 10) + 2097152) *
                     ((int32_t)calibData.digH2) +
                 8192) >>
                14));
  v_x1_u32r -= (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calibData.digH1)) >> 4);
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  *humidity = (float)(v_x1_u32r >> 12) / 1024.0f;

  return true;
}

int BME280::readBytes(uint8_t regAddr, uint8_t *buffer, uint8_t len)
{
  int result = i2c_write_blocking(this->i2cPort, this->address, &regAddr, 1, true);
  if (result != 1)
  {
    return -1; // Error
  }
  result = i2c_read_blocking(this->i2cPort, this->address, buffer, len, false);
  return result;
}

int BME280::writeByte(uint8_t regAddr, uint8_t value)
{
  uint8_t buffer[2] = {regAddr, value};
  return i2c_write_blocking(this->i2cPort, this->address, buffer, 2, false);
}

void BME280::loadCalibrationData()
{
  uint8_t calib[26];
  this->readBytes(0x88, calib, 24);
  calibData.digT1 = (uint16_t)(calib[1] << 8) | calib[0];
  calibData.digT2 = (int16_t)(calib[3] << 8) | calib[2];
  calibData.digT3 = (int16_t)(calib[5] << 8) | calib[4];
  calibData.digP1 = (uint16_t)(calib[7] << 8) | calib[6];
  calibData.digP2 = (int16_t)(calib[9] << 8) | calib[8];
  calibData.digP3 = (int16_t)(calib[11] << 8) | calib[10];
  calibData.digP4 = (int16_t)(calib[13] << 8) | calib[12];
  calibData.digP5 = (int16_t)(calib[15] << 8) | calib[14];
  calibData.digP6 = (int16_t)(calib[17] << 8) | calib[16];
  calibData.digP7 = (int16_t)(calib[19] << 8) | calib[18];
  calibData.digP8 = (int16_t)(calib[21] << 8) | calib[20];
  calibData.digP9 = (int16_t)(calib[23] << 8) | calib[22];
  calibData.digH1 = calib[24];
  this->readBytes(0xE1, calib, 7);
  calibData.digH2 = (int16_t)(calib[1] << 8) | calib[0];
  calibData.digH3 = calib[2];
  int16_t e4 = calib[3];
  int16_t e5 = calib[4];
  calibData.digH4 = (e4 << 4) | (e5 & 0x0F);
  calibData.digH5 = (e5 >> 4) | (calib[5] << 4);
  calibData.digH6 = (int8_t)calib[6];

  // Debug output for calibration data
  printf("Calibration Data:\n");
  printf("T1: %u\n", calibData.digT1);
  printf("T2: %d\n", calibData.digT2);
  printf("T3: %d\n", calibData.digT3);
  printf("P1: %u\n", calibData.digP1);
  printf("P2: %d\n", calibData.digP2);
  printf("P3: %d\n", calibData.digP3);
  printf("P4: %d\n", calibData.digP4);
  printf("P5: %d\n", calibData.digP5);
  printf("P6: %d\n", calibData.digP6);
  printf("P7: %d\n", calibData.digP7);
  printf("P8: %d\n", calibData.digP8);
  printf("P9: %d\n", calibData.digP9);
  printf("H1: %u\n", calibData.digH1);
  printf("H2: %d\n", calibData.digH2);
  printf("H3: %u\n", calibData.digH3);
  printf("H4: %d\n", calibData.digH4);
  printf("H5: %d\n", calibData.digH5);
  printf("H6: %d\n", (int)calibData.digH6);
}
