/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr (Seeed Technology Inc.)
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

#ifndef _EL_MUTEX_HPP_
#define _EL_MUTEX_HPP_

#include "core/el_config_internal.h"

#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    #include <freertos/FreeRTOS.h>
    #include <freertos/semphr.h>
#endif

namespace edgelab {

class Mutex {
   public:
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    Mutex() noexcept : _lock(xSemaphoreCreateCounting(1, 1)) {}
    ~Mutex() noexcept { vSemaphoreDelete(_lock); }
#else
    Mutex() noexcept  = default;
    ~Mutex() noexcept = default;
#endif

    inline void lock() const {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
        xSemaphoreTake(_lock, portMAX_DELAY);
#endif
    }

    inline void unlock() const {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
        xSemaphoreGive(_lock);
#endif
    }

   private:
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    mutable SemaphoreHandle_t _lock;
#endif
};

}  // namespace edgelab

#endif
