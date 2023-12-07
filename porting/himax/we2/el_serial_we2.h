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

#ifndef _EL_SERIAL_WE2_H_
#define _EL_SERIAL_WE2_H_

extern "C" {
#include <hx_drv_uart.h>
}

#include "porting/el_serial.h"

namespace edgelab {

class SerialWE2 final : public Serial {
   public:
    SerialWE2();
    ~SerialWE2();

    el_err_code_t init() override;
    el_err_code_t deinit() override;

    char        echo(bool only_visible = true) override;
    char        get_char() override;
    std::size_t get_line(char* buffer, size_t size, const char delim = 0x0d) override;

    std::size_t read_bytes(char* buffer, size_t size) override;
    std::size_t send_bytes(const char* buffer, size_t size) override;

   private:
    DEV_UART* _console_uart;
};

}  // namespace edgelab

#endif
