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

#include "el_serial_himax.h"

#include <cctype>

#include "core/el_debug.h"
#include "core/el_types.h"

namespace edgelab {

SerialEsp::SerialEsp() {}

SerialEsp::~SerialEsp() { deinit(); }

el_err_code_t SerialEsp::init() {
    this->console_uart = hx_drv_uart_get_dev((USE_SS_UART_E)CONSOLE_UART_ID);
    this->_is_present  = this->console_uart != nullptr;

    return this->_is_present ? EL_OK : EL_EIO;
}

el_err_code_t SerialEsp::deinit() {
    this->_is_present = (!hx_drv_uart_deinit((USE_SS_UART_E)CONSOLE_UART_ID) ? false : true);

    return !this->_is_present ? EL_OK : EL_EIO;
}

char SerialEsp::echo(bool only_visible) {
    EL_ASSERT(this->_is_present);

    char c{get_char()};
    if (only_visible && !isprint(c)) return c;
    this->console_uart->uart_write(&c, sizeof(c));
    return c;
}

char SerialEsp::get_char() {
    EL_ASSERT(this->_is_present);

    char c{'\0'};
    this->console_uart->uart_read(&c, 1);
    return c;
}

size_t SerialEsp::get_line(char* buffer, size_t size, const char delim) {
    EL_ASSERT(this->_is_present);

    size_t pos{0};
    char   c{'\0'};
    while (pos < size - 1) {
        if (!this->console_uart->uart_read_nonblock(&c, 1)) continue;

        if (c == delim || c == 0x00) [[unlikely]] {
            buffer[pos++] = '\0';
            return pos;
        }
        buffer[pos++] = c;
    }
    buffer[pos++] = '\0';
    return pos;
}

el_err_code_t SerialEsp::read_bytes(char* buffer, size_t size) {
    EL_ASSERT(this->_is_present);

    size_t read{0};
    size_t pos_of_bytes{0};

    while (size) {
        size_t bytes_to_read{size < 8 ? size : 8};

        read += this->console_uart->uart_read_nonblock(buffer + pos_of_bytes, bytes_to_read);
        pos_of_bytes += bytes_to_read;
        size -= bytes_to_read;
    }

    return read > 0 ? EL_OK : EL_AGAIN;
}

el_err_code_t SerialEsp::send_bytes(const char* buffer, size_t size) {
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
