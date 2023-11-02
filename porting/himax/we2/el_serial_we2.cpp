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
#include "el_config_porting.h"

namespace edgelab {

namespace container {

class RingBuffer {
   public:
    RingBuffer(size_t size) : _s{size}, _p{0}, _c{0}, _l{0}, _b{new char[size]{}} {}

    ~RingBuffer() {
        if (_b) [[likely]]
            delete[] _b;
    }

    inline void push(char ch) {
        _b[_p++ % _s] = ch;
        ++_l;
    }

    inline bool get(char& ch) {
        if (_l) [[likely]] {
            ch = _b[_c++ % _s];
            --_l;
            return true;
        }
        return false;
    }

   private:
    const size_t    _s;
    volatile size_t _p;
    volatile size_t _c;
    volatile size_t _l;
    char*           _b;
};

}  // namespace container

namespace porting {

static container::RingBuffer* _serial_ring_buffer = nullptr;
static char                   _serial_char_buffer[32]{};
volatile static DEV_UART*     _serial_port_handler = nullptr;

void _serial_uart_dma_recv(void*) {
    _serial_ring_buffer->push(_serial_char_buffer[0]);
    _serial_port_handler->uart_read_udma(_serial_char_buffer, 1, (void*)_serial_uart_dma_recv);
}

}  // namespace porting

using namespace porting;

SerialWE2::~SerialWE2() { deinit(); }

el_err_code_t SerialWE2::init() {
    EL_ASSERT(!this->_is_present);

    this->console_uart = hx_drv_uart_get_dev((USE_DW_UART_E)CONSOLE_UART_ID);
    this->console_uart->uart_open(UART_BAUDRATE_921600);

    if (!_serial_ring_buffer) [[likely]]
        _serial_ring_buffer = new container::RingBuffer{8192};

    EL_ASSERT(_serial_ring_buffer);

    _serial_port_handler = this->console_uart;
    _serial_port_handler->uart_read_udma(_serial_char_buffer, 1, (void*)_serial_uart_dma_recv);

    this->_is_present = this->console_uart != nullptr;

    return this->_is_present ? EL_OK : EL_EIO;
}

el_err_code_t SerialWE2::deinit() {
    this->_is_present = (!hx_drv_uart_deinit((USE_DW_UART_E)CONSOLE_UART_ID) ? false : true);

    return !this->_is_present ? EL_OK : EL_EIO;
}

char SerialWE2::echo(bool only_visible) {
    EL_ASSERT(this->_is_present);

    char c{get_char()};
    if (only_visible && !isprint(c)) return c;

    this->console_uart->uart_write(&c, sizeof(c));

    return c;
}

char SerialWE2::get_char() {
    EL_ASSERT(this->_is_present);

    char c{'\0'};
    porting::_serial_ring_buffer->get(c);
    return c;
}

size_t SerialWE2::get_line(char* buffer, size_t size, const char delim) {
    EL_ASSERT(this->_is_present);

    size_t pos{0};
    char   c{'\0'};
    while (pos < size - 1) {
        if (!porting::_serial_ring_buffer->get(c)) continue;

        if (c == delim || c == 0x00) [[unlikely]] {
            buffer[pos++] = '\0';
            return pos;
        }
        buffer[pos++] = c;
    }
    buffer[pos++] = '\0';
    return pos;
}

el_err_code_t SerialWE2::read_bytes(char* buffer, size_t size) {
    EL_ASSERT(this->_is_present);

    size_t read{0};
    size_t pos_of_bytes{0};

    while (size) {
        size_t bytes_to_read{size < 8 ? size : 8};

        read += this->console_uart->uart_read(buffer + pos_of_bytes, bytes_to_read);
        pos_of_bytes += bytes_to_read;
        size -= bytes_to_read;
    }

    return read > 0 ? EL_OK : EL_AGAIN;
}

el_err_code_t SerialWE2::send_bytes(const char* buffer, size_t size) {
    EL_ASSERT(this->_is_present);

    size_t sent{0};
    size_t pos_of_bytes{0};

    while (size) {
        size_t bytes_to_send{size < 8 ? size : 8};

        sent += this->console_uart->uart_write(buffer + pos_of_bytes, bytes_to_send);
        pos_of_bytes += bytes_to_send;
        size -= bytes_to_send;
    }

    return sent == pos_of_bytes ? EL_OK : EL_AGAIN;
}

}  // namespace edgelab
