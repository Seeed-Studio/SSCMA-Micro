/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr, Seeed Technology Co.,Ltd
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

#ifndef _EL_VERSION_H_
#define _EL_VERSION_H_

#include <cstdint>

namespace edgelab {

namespace constants {

constexpr static uint16_t YEAR =
  (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');

constexpr static uint16_t MONTH = (__DATE__[0] == 'J')   ? ((__DATE__[1] == 'a') ? 1 : ((__DATE__[2] == 'n') ? 6 : 7))
                                  : (__DATE__[0] == 'F') ? 2
                                  : (__DATE__[0] == 'M') ? ((__DATE__[2] == 'r') ? 3 : 5)
                                  : (__DATE__[0] == 'A') ? ((__DATE__[1] == 'p') ? 4 : 8)
                                  : (__DATE__[0] == 'S') ? 9
                                  : (__DATE__[0] == 'O') ? 10
                                  : (__DATE__[0] == 'N') ? 11
                                  : (__DATE__[0] == 'D') ? 12
                                                         : 0;

constexpr static uint16_t DAY =
  (__DATE__[4] == ' ') ? (__DATE__[5] - '0') : (__DATE__[4] - '0') * 10 + (__DATE__[5] - '0');

constexpr static char VERSION[]{
  YEAR / 1000 + '0',
  (YEAR % 1000) / 100 + '0',
  (YEAR % 100) / 10 + '0',
  YEAR % 10 + '0',
  '.',
  MONTH / 10 + '0',
  MONTH % 10 + '0',
  '.',
  DAY / 10 + '0',
  DAY % 10 + '0',
  '\0',
};

}  // namespace constants

}  // namespace edgelab

constexpr static auto& __EL_VERSION__ = edgelab::constants::VERSION;

#endif
