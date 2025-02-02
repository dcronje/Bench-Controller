#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

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
const int DISPLAY_SPI_CS_GPIO = 17;   // Chip Select (CS) – GPIO 17
const int DISPLAY_SPI_DC_GPIO = 20;   // Data/Command (DC) – GPIO 14
const int DISPLAY_SPI_RST_GPIO = 16;  // Reset (RST) – GPIO 13

const int ENCODER_CLK_GPIO = 22; // Replace with your actual pin numbers
const int ENCODER_DC_GPIO = 26;
const int ENTER_SW_GPIO = 27;

const int COMPRESSOR_BUTTON_GPIO = 12;
const int EXTRACTOR_BUTTON_GPIO = 12;

const int PRESSURE_SENSOR_GPIO = 26;
const int PRESSURE_SENSOR_ADC_CHANNEL = 0;
const int CURRENT_SENSOR_GPIO = 27;
const int CURRENT_SENSOR_ADC_CHANNEL = 1;

const int RELAY_GPIO = 17;
const int SOLENOID_GPIO = 15;

const int TEMPERATURE_SENSOR_GPIO = 12;

// const int LED_GPIO = CYW43_WL_GPIO_LED_PIN;
const int SOCKET_SERVER_PORT = 3000;

const int SHUT_DOWN_BUTTON_GPIO = 6;
const int FORGET_WIFI_BUTTON_GPIO = 4;

const int WS2812_GPIO = 3;

const uint8_t SHT30_I2C_ADDR = 0x44;               // SHT30 I²C address
const uint16_t SHT30_CMD_MEASURE_HIGHREP = 0x2400; // High repeatability measurement command
const uint8_t SHT30_I2C_SDA_GPIO = 14;             // SDA pin for I²C communication
const uint8_t SHT30_I2C_SCL_GPIO = 15;             // SCL pin for I²C communication
const uint32_t SHT30_I2C_FREQ = 100000;

const uint8_t TSL2561_ADDRESS = 0x39;           // Default I2C address
const uint8_t TSL2561_CMD_BIT = 0x80;           // Command bit
const uint8_t TSL2561_CONTROL = 0x00;           // Control register
const uint8_t TSL2561_TIMING = 0x01;            // Timing register
const uint8_t TSL2561_THRESHOLD_LOWLOW = 0x02;  // Low threshold low byte
const uint8_t TSL2561_INTERRUPT = 0x06;         // Interrupt control
const uint8_t TSL2561_CHANNEL0_LOW = 0x0C;      // Channel 0 lower data register
const uint8_t TSL2561_CHANNEL1_LOW = 0x0E;      // Channel 1 lower data register
const uint8_t TSL2561_INTERRUPT_DISABLE = 0x00; // Value to disable interrupts

const int LIGHTS_A_TEMP_GPIO = 10;
const int LIGHTS_B_TEMP_GPIO = 10;
const int LIGHTS_C_TEMP_GPIO = 10;

#define DISPLAY_SPI_PORT spi0
#define SENSOR_I2C_PORT i2c1 // Change to i2c1 if needed

#endif // CONSTANTS_H