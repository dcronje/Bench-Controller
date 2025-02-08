#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

#define SSID_MAX_LEN 32
#define PASSWORD_MAX_LEN 64
#define WIFI_SSID "Pico-Bench"
#define WIFI_PASSWORD "password"
#define SOCKET_SERVER_IP "192.168.10.44"
#define MDNS_DOMAIN "bench"
#define MDNS_NAME "Bench"
#define MDNS_SERVICE "_bench"

const int DISPLAY_SPI_CLK_GPIO = 18;  // SPI clock (SCK) – GPIO 18
const int DISPLAY_SPI_MOSI_GPIO = 19; // SPI data (MOSI) – GPIO 19
const int DISPLAY_SPI_CS_GPIO = 22;   // Chip Select (CS) – GPIO 17
const int DISPLAY_SPI_DC_GPIO = 21;   // Data/Command (DC) – GPIO 14
const int DISPLAY_SPI_RST_GPIO = 20;  // Reset (RST) – GPIO 13

const int ENCODER_CLK_GPIO = 11; // Replace with your actual pin numbers
const int ENCODER_DC_GPIO = 12;
const int ENTER_SW_GPIO = 13;

const int COMPRESSOR_BUTTON_GPIO = 10;
const int EXTRACTOR_BUTTON_GPIO = 9;

// const int LED_GPIO = CYW43_WL_GPIO_LED_PIN;
const int SOCKET_SERVER_PORT = 3000;

const int SHUT_DOWN_BUTTON_GPIO = 6;
const int FORGET_WIFI_BUTTON_GPIO = 4;

const uint8_t SENSORS_I2C_SDA_GPIO = 14; // SDA pin for I²C communication
const uint8_t SENSORS_I2C_SCL_GPIO = 15; // SCL pin for I²C communication
const uint32_t SENSORS_I2C_FREQ = 100000;

const int LIGHTS_A_TEMP_GPIO = 3;
const int LIGHTS_B_TEMP_GPIO = 5;
const int LIGHTS_C_TEMP_GPIO = 7;

const uint8_t BME280_I2C_ADDR = 0x76;   // Default I2C address for BME280
const uint8_t MAX44009_I2C_ADDR = 0x4A; // Default I2C address for MAX44009
const uint8_t SHT30_I2C_ADDR = 0x44;

const float TEMP_HIGH = 25.0f;
const float TEMP_LOW = 17.0f;

const float HUMIDITY_HIGH = 60.0f;
const float HUMIDITY_LOW = 40.0f;

const float LUX_LOW = 600.0f;
const float LUX_MEDIUM = 1000.0f;

const int EXTRACTOR_PWM_GPIO = 16;
const int EXTRACTOR_TACH_GPIO = 17;

#define DISPLAY_SPI_PORT spi0
#define SENSOR_I2C_PORT i2c1 // Assuming using i2c1 for sensors

#endif // CONSTANTS_H
