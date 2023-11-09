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

#ifndef _EL_MISC_H_
#define _EL_MISC_H_

#ifdef __cplusplus
    #include <cstdbool>
    #include <cstddef>
    #include <cstdint>
#else
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void el_sleep(uint32_t ms);

uint64_t el_get_time_ms(void);

uint64_t el_get_time_us(void);

void* el_malloc(size_t size);

void* el_aligned_malloc_once(size_t align, size_t size);

void* el_calloc(size_t nmemb, size_t size);

void el_free(void* ptr);

int el_printf(const char* format, ...);

int el_putchar(char c);

void el_reset(void);

void el_status_led(bool on);

#ifdef __cplusplus
}
#endif

#endif
