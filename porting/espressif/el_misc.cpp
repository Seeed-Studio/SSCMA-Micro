/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include "core/el_compiler.h"
#include "porting/el_misc.h"
#include "porting/espressif/el_board_config.h"

namespace edgelab {

EL_ATTR_WEAK void el_sleep(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }

EL_ATTR_WEAK uint64_t el_get_time_ms(void) { return (uint64_t)esp_timer_get_time() / 1000; }

EL_ATTR_WEAK uint64_t el_get_time_us(void) { return (uint64_t)esp_timer_get_time(); }

EL_ATTR_WEAK int el_printf(const char* fmt, ...) {
    va_list args;
    int     n;
    va_start(args, fmt);
    n = vprintf(fmt, args);
    va_end(args);
    return n;
}

EL_ATTR_WEAK int el_putchar(char c) { return putchar(c); }

EL_ATTR_WEAK void* el_malloc(size_t size) { return malloc(size); }

EL_ATTR_WEAK void* el_calloc(size_t nmemb, size_t size) { return calloc(nmemb, size); }

EL_ATTR_WEAK void el_free(void* ptr) { free(ptr); }

EL_ATTR_WEAK void el_reset(void) { exit(0); }

EL_ATTR_WEAK void el_status_led(bool on) {
    gpio_set_direction(LED_0_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_0_PIN, on ? 0 : 1);
}

}  // namespace edgelab
