/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 (Seeed Technology Inc.)
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

extern "C" {
#include <hx_drv_gpio.h>
#include <hx_drv_scu.h>
#include <xprintf.h>
}

#include <cstdarg>
#include <cstdint>
#include <cstdlib>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "el_config_porting.h"
#include "porting/el_misc.h"

EL_ATTR_WEAK void el_sleep(uint32_t ms) {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    vTaskDelay(ms / portTICK_PERIOD_MS);
#else
    board_delay_ms(ms);
#endif
}

EL_ATTR_WEAK uint64_t el_get_time_ms(void) {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#else
    uint32_t tick = 0;
    uint32_t time = 0;
    SystemGetTick(&tick, &time);
    return (uint64_t)time;
#endif
}

EL_ATTR_WEAK uint64_t el_get_time_us(void) {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    return xTaskGetTickCount() * portTICK_PERIOD_MS * 1000;
#else
    return el_get_time_ms() * 1000;
#endif
}

EL_ATTR_WEAK int el_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    xvprintf(fmt, args);
    va_end(args);
    return 0;
}

EL_ATTR_WEAK int el_putchar(char c) { return 0; }

EL_ATTR_WEAK void* el_malloc(size_t size) {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    return pvPortMalloc(size);
#else
    return malloc(size);
#endif
}

EL_ATTR_WEAK void* el_aligned_malloc_once(size_t align, size_t size) {
    constexpr static const size_t elHeapSize = 1108 * 1024;
    static uint8_t                elHeap[elHeapSize]{};
    static uint8_t*               cp      = elHeap;
    size_t                        pv      = reinterpret_cast<size_t>(cp);
    size_t                        of      = align - (pv % align);
    void*                         aligned = cp + of;
    size_t                        rq      = size + of;
    size_t                        rm      = elHeapSize - static_cast<size_t>(cp - elHeap);
    if (rq > rm) [[unlikely]]
        return nullptr;
    cp += rq;
    return aligned;
}

EL_ATTR_WEAK void* el_calloc(size_t nmemb, size_t size) {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    return pvPortMalloc(nmemb * size);
#else
    return calloc(nmemb, size);
#endif
}

EL_ATTR_WEAK void el_free(void* ptr) {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    vPortFree(ptr);
#else
    free(ptr);
#endif
}

EL_ATTR_WEAK void el_reset(void) { __NVIC_SystemReset(); }

EL_ATTR_WEAK void el_status_led(bool on) {
    // maybe unsafe when build with -no-threadsafe-statics
    __attribute__((used)) static auto call_once = []() {
        hx_drv_scu_set_SEN_D2_pinmux(SCU_SEN_D2_PINMUX_GPIO20);
        hx_drv_gpio_set_output(GPIO20, GPIO_OUT_LOW);
        hx_drv_gpio_set_out_value(GPIO20, GPIO_OUT_LOW);
        return 0;
    }();
    hx_drv_gpio_set_out_value(GPIO20, on ? GPIO_OUT_HIGH : GPIO_OUT_LOW);
}

#if CONFIG_EL_HAS_FREERTOS_SUPPORT
EL_ATTR_WEAK void* operator new[](size_t size) { return pvPortMalloc(size); }

EL_ATTR_WEAK void operator delete[](void* p) { vPortFree(p); }

EL_ATTR_WEAK void* operator new(size_t size) { return pvPortMalloc(size); }

EL_ATTR_WEAK void operator delete(void* p) { vPortFree(p); }
#endif
