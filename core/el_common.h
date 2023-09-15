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

#ifndef _EL_COMMON_H_
#define _EL_COMMON_H_

#include "core/el_compiler.h"
#include "core/el_config_internal.h"
#include "core/el_debug.h"
#include "core/el_types.h"
#include "porting/el_misc.h"

#define EL_VERSION                 "2023.09.12"
#define EL_VERSION_LENTH_MAX       32

#define EL_CONCAT(a, b)            a##b
#define EL_STRINGIFY(s)            #s

#define EL_UNUSED(x)               (void)(x)
#define EL_ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

#define EL_MAX(a, b)               ((a) > (b) ? (a) : (b))
#define EL_MIN(a, b)               ((a) < (b) ? (a) : (b))
#define EL_CLIP(x, a, b)           ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

#define EL_ABS(a)                  ((a) < 0 ? -(a) : (a))
#define EL_ALIGN(x, a)             (((x) + ((a)-1)) & ~((a)-1))
#define EL_ALIGN_DOWN(x, a)        ((x) & ~((a)-1))
#define EL_ALIGN_UP(x, a)          (((x) + ((a)-1)) & ~((a)-1))
#define EL_IS_ALIGNED(x, a)        (((x) & ((typeof(x))(a)-1)) == 0)
#define EL_IS_ALIGNED_DOWN(x, a)   (((x) & ((typeof(x))(a)-1)) == 0)
#define EL_IS_ALIGNED_UP(x, a)     (((x) & ((typeof(x))(a)-1)) == 0)

#define EL_BIT(n)                  (1 << (n))
#define EL_BIT_MASK(n)             (EL_BIT(n) - 1)
#define EL_BIT_SET(x, n)           ((x) |= EL_BIT(n))
#define EL_BIT_CLR(x, n)           ((x) &= ~EL_BIT(n))
#define EL_BIT_GET(x, n)           (((x) >> (n)) & 1)
#define EL_BIT_SET_MASK(x, n, m)   ((x) |= ((m) << (n)))
#define EL_BIT_CLR_MASK(x, n, m)   ((x) &= ~((m) << (n)))
#define EL_BIT_GET_MASK(x, n, m)   (((x) >> (n)) & (m))
#define EL_BIT_SET_MASKED(x, n, m) ((x) |= ((m) & (EL_BIT_MASK(n) << (n))))
#define EL_BIT_CLR_MASKED(x, n, m) ((x) &= ~((m) & (EL_BIT_MASK(n) << (n))))
#define EL_BIT_GET_MASKED(x, n, m) (((x) >> (n)) & (m))

#endif
