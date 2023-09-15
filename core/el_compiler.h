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

#ifndef _EL_COMPILER_H_
#define _EL_COMPILER_H_

#define EL_LITTLE_ENDIAN (0x12u)
#define EL_BIG_ENDIAN (0x21u)

// TODO: refactor since __attribute__ is supported across many compiler
#if defined(__GNUC__)
    #define EL_ATTR_ALIGNED(Bytes) __attribute__((aligned(Bytes)))
    #define EL_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
    #define EL_ATTR_PACKED __attribute__((packed))
    #define EL_ATTR_WEAK __attribute__((weak))
    #define EL_ATTR_ALWAYS_INLINE __attribute__((always_inline))
    #define EL_ATTR_DEPRECATED(mess) __attribute__((deprecated(mess)))  // warn if function with this attribute is used
    #define EL_ATTR_UNUSED __attribute__((unused))  // Function/Variable is meant to be possibly unused
    #define EL_ATTR_USED __attribute__((used))      // Function/Variable is meant to be used

    #define EL_ATTR_PACKED_BEGIN
    #define EL_ATTR_PACKED_END
    #define EL_ATTR_BIT_FIELD_ORDER_BEGIN
    #define EL_ATTR_BIT_FIELD_ORDER_END

    #if __has_attribute(__fallthrough__)
        #define EL_ATTR_FALLTHROUGH __attribute__((fallthrough))
    #else
        #define EL_ATTR_FALLTHROUGH \
            do {                    \
            } while (0) /* fallthrough */
    #endif

    // Endian conversion use well-known host to network (big endian) naming
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define EL_BYTE_ORDER EL_LITTLE_ENDIAN
    #else
        #define EL_BYTE_ORDER EL_BIG_ENDIAN
    #endif

    #define EL_BSWAP16(u16) (__builtin_bswap16(u16))
    #define EL_BSWAP32(u32) (__builtin_bswap32(u32))

    #ifndef __ARMCC_VERSION
        // List of obsolete callback function that is renamed and should not be defined.
        // Put it here since only gcc support this pragma
        #pragma GCC poison tud_vendor_control_request_cb
    #endif

#elif defined(__TI_COMPILER_VERSION__)
    #define EL_ATTR_ALIGNED(Bytes) __attribute__((aligned(Bytes)))
    #define EL_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
    #define EL_ATTR_PACKED __attribute__((packed))
    #define EL_ATTR_WEAK __attribute__((weak))
    #define EL_ATTR_ALWAYS_INLINE __attribute__((always_inline))
    #define EL_ATTR_DEPRECATED(mess) __attribute__((deprecated(mess)))  // warn if function with this attribute is used
    #define EL_ATTR_UNUSED __attribute__((unused))  // Function/Variable is meant to be possibly unused
    #define EL_ATTR_USED __attribute__((used))
    #define EL_ATTR_FALLTHROUGH __attribute__((fallthrough))

    #define EL_ATTR_PACKED_BEGIN
    #define EL_ATTR_PACKED_END
    #define EL_ATTR_BIT_FIELD_ORDER_BEGIN
    #define EL_ATTR_BIT_FIELD_ORDER_END

    // __BYTE_ORDER is defined in the TI ARM compiler, but not MSP430 (which is
    // little endian)
    #if ((__BYTE_ORDER__) == (__ORDER_LITTLE_ENDIAN__)) || defined(__MSP430__)
        #define EL_BYTE_ORDER EL_LITTLE_ENDIAN
    #else
        #define EL_BYTE_ORDER EL_BIG_ENDIAN
    #endif

    #define EL_BSWAP16(u16) (__builtin_bswap16(u16))
    #define EL_BSWAP32(u32) (__builtin_bswap32(u32))

#elif defined(__ICCARM__)
    #include <intrinsics.h>

    #define EL_ATTR_ALIGNED(Bytes) __attribute__((aligned(Bytes)))
    #define EL_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
    #define EL_ATTR_PACKED __attribute__((packed))
    #define EL_ATTR_WEAK __attribute__((weak))
    #define EL_ATTR_ALWAYS_INLINE __attribute__((always_inline))
    #define EL_ATTR_DEPRECATED(mess) __attribute__((deprecated(mess)))  // warn if function with this attribute is used
    #define EL_ATTR_UNUSED __attribute__((unused))  // Function/Variable is meant to be possibly unused
    #define EL_ATTR_USED __attribute__((used))      // Function/Variable is meant to be used
    #define EL_ATTR_FALLTHROUGH __attribute__((fallthrough))

    #define EL_ATTR_PACKED_BEGIN
    #define EL_ATTR_PACKED_END
    #define EL_ATTR_BIT_FIELD_ORDER_BEGIN
    #define EL_ATTR_BIT_FIELD_ORDER_END

    // Endian conversion use well-known host to network (big endian) naming
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define EL_BYTE_ORDER EL_LITTLE_ENDIAN
    #else
        #define EL_BYTE_ORDER EL_BIG_ENDIAN
    #endif

    #define EL_BSWAP16(u16) (__iar_builtin_REV16(u16))
    #define EL_BSWAP32(u32) (__iar_builtin_REV(u32))

#elif defined(__CCRX__)
    #define EL_ATTR_ALIGNED(Bytes)
    #define EL_ATTR_SECTION(sec_name)
    #define EL_ATTR_PACKED
    #define EL_ATTR_WEAK
    #define EL_ATTR_ALWAYS_INLINE
    #define EL_ATTR_DEPRECATED(mess)
    #define EL_ATTR_UNUSED
    #define EL_ATTR_USED
    #define EL_ATTR_FALLTHROUGH \
        do {                    \
        } while (0) /* fallthrough */

    #define EL_ATTR_PACKED_BEGIN _Pragma("pack")
    #define EL_ATTR_PACKED_END _Pragma("packoption")
    #define EL_ATTR_BIT_FIELD_ORDER_BEGIN _Pragma("bit_order right")
    #define EL_ATTR_BIT_FIELD_ORDER_END _Pragma("bit_order")

    // Endian conversion use well-known host to network (big endian) naming
    #if defined(__LIT)
        #define EL_BYTE_ORDER EL_LITTLE_ENDIAN
    #else
        #define EL_BYTE_ORDER EL_BIG_ENDIAN
    #endif

    #define EL_BSWAP16(u16) ((unsigned short)_builtin_revw((unsigned long)u16))
    #define EL_BSWAP32(u32) (_builtin_revl(u32))

#else
    #error "Compiler attribute porting is required"
#endif

#if (EL_BYTE_ORDER == EL_LITTLE_ENDIAN)

    #define el_htons(u16) (EL_BSWAP16(u16))
    #define el_ntohs(u16) (EL_BSWAP16(u16))

    #define el_htonl(u32) (EL_BSWAP32(u32))
    #define el_ntohl(u32) (EL_BSWAP32(u32))

    #define el_htole16(u16) (u16)
    #define el_le16toh(u16) (u16)

    #define el_htole32(u32) (u32)
    #define el_le32toh(u32) (u32)

#elif (EL_BYTE_ORDER == EL_BIG_ENDIAN)

    #define el_htons(u16) (u16)
    #define el_ntohs(u16) (u16)

    #define el_htonl(u32) (u32)
    #define el_ntohl(u32) (u32)

    #define el_htole16(u16) (EL_BSWAP16(u16))
    #define el_le16toh(u16) (EL_BSWAP16(u16))

    #define el_htole32(u32) (EL_BSWAP32(u32))
    #define el_le32toh(u32) (EL_BSWAP32(u32))

#else
    #error Byte order is undefined
#endif

#endif
