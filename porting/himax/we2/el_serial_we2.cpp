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

static lwRingBuffer*      _serial_ring_buffer = nullptr;
static char               _serial_char_buffer[32]{};
volatile static DEV_UART* _serial_port_handler = nullptr;

void _serial_uart_dma_recv(void*) {
    _serial_ring_buffer->put(_serial_char_buffer[0]);
    _serial_port_handler->uart_read_udma(_serial_char_buffer, 1, (void*)_serial_uart_dma_recv);
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

    if (!_serial_ring_buffer) [[likely]]
        _serial_ring_buffer = new lwRingBuffer{8192};

    EL_ASSERT(_serial_ring_buffer);

    _serial_port_handler = _console_uart;
    _serial_port_handler->uart_read_udma(_serial_char_buffer, 1, (void*)_serial_uart_dma_recv);

    this->_is_present = _console_uart != nullptr;

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

    return porting::_serial_ring_buffer->get();
}

std::size_t SerialWE2::get_line(char* buffer, size_t size, const char delim) {
    if (!this->_is_present) [[unlikely]]
        return 0;

    return porting::_serial_ring_buffer->extract(delim, buffer, size);
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

    size_t sent{0};
    size_t pos_of_bytes{0};

    while (size) {
        size_t bytes_to_send{size < 8 ? size : 8};

        sent += _console_uart->uart_write(buffer + pos_of_bytes, bytes_to_send);
        pos_of_bytes += bytes_to_send;
        size -= bytes_to_send;
    }

    return sent;
}

}  // namespace edgelab
