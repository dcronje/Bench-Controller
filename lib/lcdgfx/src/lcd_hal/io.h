/*
    MIT License

    Copyright (c) 2018-2022, Alexey Dynda

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/**
 * @file lcd_hal/io.h SSD1306 HAL IO communication functions
 */

#ifndef _LCD_HAL_IO_H_
#define _LCD_HAL_IO_H_

/**
 * @defgroup SSD1306_HAL_API HAL: ssd1306 library hardware abstraction layer
 * @{
 *
 * @brief i2c/spi ssd1306 library hardware abstraction layer
 *
 * @details ssd1306 library hardware abstraction layer
 */

#include "UserSettings.h"
#include "interface.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include <stdint.h>

#ifndef LCDINT_TYPES_DEFINED
/** Macro informs if lcdint_t type is defined */
#define LCDINT_TYPES_DEFINED
/** internal int type, used by ssd1306 library. Important for uC with low SRAM */
typedef int lcdint_t;
/** internal int type, used by ssd1306 library. Important for uC with low SRAM */
typedef unsigned int lcduint_t;
#endif

/** swaps content of a and b variables of type type */
#define ssd1306_swap_data(a, b, type) \
    {                                 \
        type t = a;                   \
        a = b;                        \
        b = t;                        \
    }

////////////////////////////////////////////////////////////////////////////////
//                   HAL Layer functions
////////////////////////////////////////////////////////////////////////////////

/** Constant corresponds to low level of gpio pin */
#define LCD_LOW 0
/** Constant corresponds to high level of gpio pin */
#define LCD_HIGH 1
/** Constant corresponds to input mode of gpio */
#define LCD_GPIO_INPUT 0
/** Constant corresponds to output mode of gpio */
#define LCD_GPIO_OUTPUT 1
/** Constant corresponds to input mode of gpio with pullup resistor enabled */
#define LCD_GPIO_INPUT_PULLUP 2
/** Constant corresponds to output mode of gpio with pulldown resistor enabled */
#define LCD_GPIO_INPUT_PULLDOWN 3
// LCD_PROMEM is platform specific definition
#ifndef LCD_PROGMEM
/** LCD_PROGMEM constant is used to specify data stored in flash, platform specific */
#define LCD_PROGMEM PROGMEM
#endif

/**
 * Sets gpio pin mode
 * @param pin pin number to change mode of
 * @param mode new gpio mode: LCD_GPIO_INPUT or LCD_GPIO_OUTPUT
 */
void lcd_gpioMode(int pin, int mode);

/**
 * Reads gpio pin value
 * @param pin gpio pin number to read
 * @return LCD_HIGH or LCD_LOW
 */
int lcd_gpioRead(int pin);

/**
 * Writes value to gpio
 * @param pin gpio pin number to change
 * @param level LCD_HIGH or LCD_LOW
 */
void lcd_gpioWrite(int pin, int level);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
void lcd_registerGpioEvent(int pin, void (*on_pin_change)(void *), void *arg);
void lcd_unregisterGpioEvent(int pin);
#endif

/**
 * Read ADC data
 * @param pin adc pin to read (platform-specific)
 * @return integer value corresponding to provided gpio pin.
 *         actual value range depends on platform and ADC mode.
 */
int lcd_adcRead(int pin);

/**
 * returns 32-bit timestamp from system power-up in milliseconds
 */
uint32_t lcd_millis(void);

/**
 * returns 32-bit timestamp from system power-up in microseconds
 */
uint32_t lcd_micros(void);

/**
 * Initializes RND device
 * @param seed unique number to use for initialization
 */
void lcd_randomSeed(int seed);

/**
 * Attaches interrupt handler to pin. Not implemented on many platforms
 * @param pin gpio pin number to attach interrupt handler to
 * @param interrupt interrupt handler
 * @param level gpio state to aim interrupt
 */
void lcd_attachInterrupt(int pin, void (*interrupt)(), int level);

/**
 * Forces current thread to sleeps for specified number of milliseconds
 * @param ms time in milliseconds
 */
void lcd_delay(unsigned long ms);

/**
 * Forces current thread to sleeps for specified number of microseconds
 * @param us time in microseconds
 */
void lcd_delayUs(unsigned long us);

/**
 * Read single data byte directly from flash. This function is valid only
 * for AVR platform. For other platforms, it reads byte, pointed by ptr.
 * @param ptr pointer to data in flash
 * @return returns single byte read.
 */
uint8_t lcd_pgmReadByte(const void *ptr);

/**
 * Reads 16-bit from eeprom
 * @param ptr pointer to eeprom memory to read
 * @return 16-bit number from eeprom
 */
uint16_t lcd_eepromReadWord(const void *ptr);

/**
 * Writes 16-bit to eeprom
 * @param ptr pointer to eeprom memory to write data to
 * @param val 16-bit value to write
 */
void lcd_eepromWriteWord(const void *ptr, uint16_t val);

/** @copydoc lcd_gpioRead */
#define digitalRead lcd_gpioRead
/** @copydoc digitalRead */
#define digitalWrite lcd_gpioWrite
/** @copydoc lcd_gpioMode */
#define pinMode lcd_gpioMode
/** @copydoc lcd_adcRead */
#define analogRead lcd_adcRead

/** @copydoc lcd_pgmReadByte */
#define pgm_read_byte lcd_pgmReadByte
/** @copydoc lcd_eepromReadWord */
#define eeprom_read_word lcd_eepromReadWord
/** @copydoc lcd_eepromWriteWord */
#define eeprom_write_word lcd_eepromWriteWord

/** @copydoc lcd_millis */
#define millis lcd_millis
/** @copydoc lcd_micros */
#define micros lcd_micros
/** @copydoc lcd_delay */
#define delay lcd_delay
/** @copydoc lcd_delayUs */
#define delayMicroseconds lcd_delayUs

/** @copydoc lcd_random */
#define random lcd_random
/** @copydoc lcd_randomSeed */
#define randomSeed lcd_randomSeed

#endif