/*
    MIT License

    Copyright 2016-2022 (C) Alexey Dynda

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
 * @file lcdgfx.h LCDGFX main header
 */

#pragma once

#include "nano_gfx_types.h"
#include "canvas/fonts/fonts.h"

#ifdef __cplusplus
#include "canvas/canvas.h"
#include "nano_engine_v2.h"

#include "v2/lcd/ssd1331/lcd_ssd1331.h"

extern "C"
{
#endif

    /**
     * @defgroup LCD_GENERIC_API Generic API functions, common for all displays and all display modes.
     * @{
     * @brief Generic API functions, common for all displays and all display modes.
     *
     * @details Generic API functions, common for all displays and all display modes.
     */

    /**
     * Enables utf8 support for all text-functions.
     * @note Unicode-16 only supported in text decoding functions.
     */
    void ssd1306_enableUtf8Mode2(void);

    /**
     * Enables ascii mode for all text-functions. No any decoding will be performed
     */
    void ssd1306_enableAsciiMode2(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
