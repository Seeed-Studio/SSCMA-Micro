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

#ifndef _EL_TRANSPORT_H_
#define _EL_TRANSPORT_H_

#include <cstdbool>
#include <cstdint>

#include "core/el_types.h"

namespace edgelab {

// No status transport protocol (framed), TCP alternative should have a server class, a connection could derive from Transport
class Transport {
   public:
    Transport() : _is_present(false) {}
    virtual ~Transport() = default;

    virtual std::size_t read_bytes(char* buffer, size_t size)       = 0;
    virtual std::size_t send_bytes(const char* buffer, size_t size) = 0;

    virtual char        echo(bool only_visible = true)                               = 0;
    virtual char        get_char()                                                   = 0;
    virtual std::size_t get_line(char* buffer, size_t size, const char delim = 0x0d) = 0;

    operator bool() const { return _is_present; }

   protected:
    bool _is_present;
};

}  // namespace edgelab

#endif
