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

#include "el_serial_we2.h"

extern "C" {
#include <console_io.h>
#include <hx_drv_uart.h>
}

#include <cctype>
#include <cstdint>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/utils/el_ringbuffer.hpp"
#include "el_config_porting.h"
namespace edgelab {

namespace porting {

static lwRingBuffer*      _rb_rx = nullptr;
static lwRingBuffer*      _rb_tx = nullptr;
static char               _buf_rx[32]{};
static char               _buf_tx[1024]{};
volatile static bool      _tx_busy = false;
volatile static DEV_UART* _uart    = nullptr;

void _uart_dma_recv(void*) {
    _rb_rx->put(_buf_rx[0]);
    _uart->uart_read_udma(_buf_rx, 1, (void*)_uart_dma_recv);
}

void _uart_dma_send(void*) {
    size_t remind = _rb_tx->size() < 1024 ? _rb_tx->size() : 1024;
    if (remind == 0) {
        _tx_busy = false;
        return;
    }
    _rb_tx->get(_buf_tx, remind);
    SCB_CleanDCache_by_Addr((volatile void*)_buf_tx, remind);
    _uart->uart_write_udma(_buf_tx, remind, (void*)_uart_dma_send);
}
}  // namespace porting

using namespace porting;

SerialWE2::SerialWE2() : _console_uart(nullptr) {}

SerialWE2::~SerialWE2() { deinit(); }

el_err_code_t SerialWE2::init() {
    if (this->_is_present) [[unlikely]]
        return EL_EPERM;

    _console_uart = hx_drv_uart_get_dev((USE_DW_UART_E)CONSOLE_UART_ID);
    _console_uart->uart_open(UART_BAUDRATE_921600);

    if (!_rb_rx) [[likely]]
        _rb_rx = new lwRingBuffer{8192};

    if (!_rb_tx) [[likely]]
        _rb_tx = new lwRingBuffer{16384};

    EL_ASSERT(_rb_rx);
    EL_ASSERT(_rb_tx);

    _uart = _console_uart;
    _uart->uart_read_udma(_buf_rx, 1, (void*)_uart_dma_recv);

    this->_is_present = _console_uart != nullptr;
    _tx_busy          = false;

    return this->_is_present ? EL_OK : EL_EIO;
}

el_err_code_t SerialWE2::deinit() {
    if (!this->_is_present) [[unlikely]]
        return EL_EPERM;

    this->_is_present = !hx_drv_uart_deinit((USE_DW_UART_E)CONSOLE_UART_ID) ? false : true;

    return !this->_is_present ? EL_OK : EL_EIO;
}

char SerialWE2::echo(bool only_visible) {
    if (!this->_is_present) [[unlikely]]
        return '\0';

    char c{get_char()};
    if (only_visible && !isprint(c)) return c;

    _console_uart->uart_write(&c, sizeof(c));

    return c;
}

char SerialWE2::get_char() {
    if (!this->_is_present) [[unlikely]]
        return '\0';

    return porting::_rb_rx->get();
}

std::size_t SerialWE2::get_line(char* buffer, size_t size, const char delim) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    return porting::_rb_rx->extract(delim, buffer, size);
}

std::size_t SerialWE2::read_bytes(char* buffer, size_t size) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    size_t read{0};
    size_t pos_of_bytes{0};

    while (size) {
        size_t bytes_to_read{size < 8 ? size : 8};

        read += _console_uart->uart_read(buffer + pos_of_bytes, bytes_to_read);
        pos_of_bytes += bytes_to_read;
        size -= bytes_to_read;
    }

    return read;
}

std::size_t SerialWE2::send_bytes(const char* buffer, size_t size) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    size_t bytes_to_send{0};
    size_t sent = 0;


    if (_rb_tx->size() == 0) {
        while (_tx_busy) {
        };
        _tx_busy = true;
        _rb_tx->put(buffer, size);
        bytes_to_send = size < 1024 ? size : 1024;
        _rb_tx->get(_buf_tx, bytes_to_send);
        SCB_CleanDCache_by_Addr((volatile void*)_buf_tx, bytes_to_send);
        _uart->uart_write_udma(_buf_tx, bytes_to_send, (void*)_uart_dma_send);
    } else {
        while (size) {
            bytes_to_send = _rb_tx->put(buffer + sent, size);
            size -= bytes_to_send;
            sent += bytes_to_send;
        }
    }

    return sent;
}

}  // namespace edgelab
