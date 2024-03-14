/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
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

#include "el_serial2_we2.h"

extern "C" {
#include <console_io.h>
#include <hx_drv_scu.h>
#include <hx_drv_uart.h>
}

#include <cctype>
#include <cstdint>

#include "el_config_porting.h"

namespace edgelab {

namespace porting {

static lwRingBuffer*              _rb_rx = nullptr;
static lwRingBuffer*              _rb_tx = nullptr;
static char                       _buf_rx[32]{};
static char                       _buf_tx[4096]{};
volatile static bool              _tx_busy  = false;
volatile static SemaphoreHandle_t _mutex_tx = nullptr;
volatile static DEV_UART*         _uart     = nullptr;

static void _uart_dma_recv(void*) {
    _rb_rx->put(_buf_rx[0]);
    _uart->uart_read_udma(_buf_rx, 1, (void*)_uart_dma_recv);
}

static void _uart_dma_send(void*) {
    size_t remain = _rb_tx->size() < 4095 ? _rb_tx->size() : 4095;
    _tx_busy      = remain != 0;
    if (remain != 0) {
        _rb_tx->get(_buf_tx, remain);
        SCB_CleanDCache_by_Addr((volatile void*)_buf_tx, remain);
        _uart->uart_write_udma(_buf_tx, remain, (void*)_uart_dma_send);
    }
}
}  // namespace porting

using namespace porting;

Serial2WE2::Serial2WE2() : _console_uart(nullptr) {
    this->type = EL_TRANSPORT_UART;
}

Serial2WE2::~Serial2WE2() { deinit(); }

el_err_code_t Serial2WE2::init() {
    if (this->_is_present) [[unlikely]]
        return EL_EPERM;

    hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_UART1_RX, 0);
    hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_UART1_TX, 0);
    hx_drv_uart_init(USE_DW_UART_1, HX_UART1_BASE);

    _console_uart = hx_drv_uart_get_dev(USE_DW_UART_1);

    _console_uart->uart_open(UART_BAUDRATE_921600);

    if (!_rb_rx) [[likely]]
        _rb_rx = new lwRingBuffer{8192};

    if (!_rb_tx) [[likely]]
        _rb_tx = new lwRingBuffer{32768};

    _mutex_tx = xSemaphoreCreateMutex();

    EL_ASSERT(_rb_rx);
    EL_ASSERT(_rb_tx);
    EL_ASSERT(_mutex_tx);

    _uart = _console_uart;
    _uart->uart_read_udma(_buf_rx, 1, (void*)_uart_dma_recv);

    this->_is_present = _console_uart != nullptr;
    _tx_busy          = false;

    return this->_is_present ? EL_OK : EL_EIO;
}

el_err_code_t Serial2WE2::deinit() {
    if (!this->_is_present) [[unlikely]]
        return EL_EPERM;

    this->_is_present = !hx_drv_uart_deinit(USE_DW_UART_1) ? false : true;

    delete _rb_rx;
    delete _rb_tx;
    vSemaphoreDelete(_mutex_tx);

    _rb_rx    = nullptr;
    _rb_tx    = nullptr;
    _mutex_tx = nullptr;

    return !this->_is_present ? EL_OK : EL_EIO;
}

char Serial2WE2::echo(bool only_visible) {
    if (!this->_is_present) [[unlikely]]
        return '\0';

    char c{get_char()};
    if (only_visible && !isprint(c)) return c;

    _console_uart->uart_write(&c, sizeof(c));

    return c;
}

char Serial2WE2::get_char() {
    if (!this->_is_present) [[unlikely]]
        return '\0';

    return porting::_rb_rx->get();
}

std::size_t Serial2WE2::get_line(char* buffer, size_t size, const char delim) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    return porting::_rb_rx->extract(delim, buffer, size);
}

std::size_t Serial2WE2::read_bytes(char* buffer, size_t size) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    size_t time_start = el_get_time_ms();
    size_t read{0};
    size_t pos_of_bytes{0};

    while (size) {
        pos_of_bytes = _rb_rx->size();
        if (pos_of_bytes) {
            read += _rb_rx->get(buffer + read, size);
            size -= read;
        }
        if (el_get_time_ms() - time_start > 3000) {
            break;
        }
    }

    return read;
}

std::size_t Serial2WE2::send_bytes(const char* buffer, size_t size) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    size_t time_start = el_get_time_ms();
    size_t bytes_to_send{0};
    size_t sent = 0;
    xSemaphoreTake(_mutex_tx, portMAX_DELAY);

    //el_printf("\nsend: %d %d\n", size, _rb_tx->size());

    while (size) {
        bytes_to_send = _rb_tx->put(buffer + sent, size);
        //el_printf("\nput: %d\n", bytes_to_send);
        size -= bytes_to_send;
        sent += bytes_to_send;
        if (!_tx_busy) {
            _tx_busy      = true;
            bytes_to_send = _rb_tx->size() < 4095 ? _rb_tx->size() : 4095;
            _rb_tx->get(_buf_tx, bytes_to_send);
            SCB_CleanDCache_by_Addr((volatile void*)_buf_tx, bytes_to_send);
            _uart->uart_write_udma(_buf_tx, bytes_to_send, (void*)_uart_dma_send);
        }
        if (el_get_time_ms() - time_start > 3000) {
            el_printf("\ntimeout\n");
            _tx_busy = false;
            break;
        }
    }

    xSemaphoreGive(_mutex_tx);
    return sent;
}

}  // namespace edgelab