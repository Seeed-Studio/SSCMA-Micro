#ifndef _MA_COMPILER_H_
#define _MA_COMPILER_H_


#define MA_STRINGIFY(s)            #s
#define MA_STRINGIZE(s)            MA_STRINGIFY(s)

#define MA_CONCAT(a, b)            a##b
#define MA_CONCAT3(a, b, c)        a##b##c

#define MA_UNUSED(x)               (void)(x)
#define MA_ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

#define MA_MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MA_MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MA_CLIP(x, a, b)           ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

#define MA_ABS(a)                  ((a) < 0 ? -(a) : (a))
#define MA_ALIGN(x, a)             (((x) + ((a) - 1)) & ~((a) - 1))
#define MA_ALIGN_DOWN(x, a)        ((x) & ~((a) - 1))
#define MA_ALIGN_UP(x, a)          (((x) + ((a) - 1)) & ~((a) - 1))
#define MA_IS_ALIGNED(x, a)        (((x) & ((typeof(x))(a) - 1)) == 0)
#define MA_IS_ALIGNED_DOWN(x, a)   (((x) & ((typeof(x))(a) - 1)) == 0)
#define MA_IS_ALIGNED_UP(x, a)     (((x) & ((typeof(x))(a) - 1)) == 0)

#define MA_BIT(n)                  (1 << (n))
#define MA_BIT_MASK(n)             (MA_BIT(n) - 1)
#define MA_BIT_SET(x, n)           ((x) |= MA_BIT(n))
#define MA_BIT_CLR(x, n)           ((x) &= ~EL_BIT(n))
#define MA_BIT_GET(x, n)           (((x) >> (n)) & 1)
#define MA_BIT_SET_MASK(x, n, m)   ((x) |= ((m) << (n)))
#define MA_BIT_CLR_MASK(x, n, m)   ((x) &= ~((m) << (n)))
#define MA_BIT_GET_MASK(x, n, m)   (((x) >> (n)) & (m))
#define MA_BIT_SET_MASKED(x, n, m) ((x) |= ((m) & (MA_BIT_MASK(n) << (n))))
#define MA_BIT_CLR_MASKED(x, n, m) ((x) &= ~((m) & (MA_BIT_MASK(n) << (n))))
#define MA_BIT_GET_MASKED(x, n, m) (((x) >> (n)) & (m))


#if defined __COUNTER__ && __COUNTER__ != __COUNTER__
#define _MA_COUNTER_ __COUNTER__
#else
#define _MA_COUNTER_ __LINE__
#endif

// Compile-time Assert
#if defined(__cplusplus) && __cplusplus >= 201103L
#define MA_STATIC_ASSERT static_assert
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define MA_STATIC_ASSERT _Static_assert
#elif defined(__CCRX__)
#define MA_STATIC_ASSERT(const_expr, _mess) \
    typedef char MA_CONCAT(_verify_static_, _MA_COUNTER_)[(const_expr) ? 1 : 0];
#else
#define MA_STATIC_ASSERT(const_expr, _mess) \
    enum { MA_CONCAT(_verify_static_, _MA_COUNTER_) = 1 / (!!(const_expr)) }
#endif

#define MA_LITTLE_ENDIAN (0x12u)
#define MA_BIG_ENDIAN    (0x21u)

#if defined(__GNUC__) || defined(__clang__)
#define MA_ATTR_ALIGNED(Bytes)    __attribute__((aligned(Bytes)))
#define MA_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
#define MA_ATTR_PACKED            __attribute__((packed))
#define MA_ATTR_WEAK              __attribute__((weak))
#define MA_ATTR_ALWAYS_INLINE     __attribute__((always_inline))
#define MA_ATTR_DEPRECATED(mess)  __attribute__((deprecated(mess)))
#define MA_ATTR_UNUSED            __attribute__((unused))
#define MA_ATTR_USED              __attribute__((used))

#if __has_attribute(__fallthrough__)
#define MA_ATTR_FALLTHROUGH __attribute__((fallthrough))
#else
#define MA_ATTR_FALLTHROUGH \
    do {                    \
    } while (0) /* fallthrough */
#endif

// Endian conversion use well-known host to network (big endian) naming
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MA_BYTE_ORDER MA_LITTLE_ENDIAN
#else
#define MA_BYTE_ORDER MA_BIG_ENDIAN
#endif

#define MA_BSWAP16(u16) (__builtin_bswap16(u16))
#define MA_BSWAP32(u32) (__builtin_bswap32(u32))

#elif defined(__ICCARM__) || defined(__IAR_SYSTEMS_ICC__) || defined(__CC_ARM)
#include <intrinsics.h>

#define MA_ATTR_ALIGNED(Bytes)    __attribute__((aligned(Bytes)))
#define MA_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
#define MA_ATTR_PACKED            __attribute__((packed))
#define MA_ATTR_WEAK              __attribute__((weak))
#define MA_ATTR_ALWAYS_INLINE     __attribute__((always_inline))
#define MA_ATTR_DEPRECATED(mess) \
    __attribute__((deprecated(mess)))                // warn if function with this attribute is used
#define MA_ATTR_UNUSED      __attribute__((unused))  // Function/Variable is meant to be possibly unused
#define MA_ATTR_USED        __attribute__((used))  // Function/Variable is meant to be used
#define MA_ATTR_FALLTHROUGH __attribute__((fallthrough))

// Endian conversion use well-known host to network (big endian) naming
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MA_BYTE_ORDER MA_LITTLE_ENDIAN
#else
#define MA_BYTE_ORDER MA_BIG_ENDIAN
#endif

#define MA_BSWAP16(u16) (__iar_builtin_REV16(u16))
#define MA_BSWAP32(u32) (__iar_builtin_REV(u32))

#else
#error "Compiler attribute porting is required"
#endif

#if (MA_BYTE_ORDER == MA_LITTLE_ENDIAN)

#define ma_htons(u16)   (MA_BSWAP16(u16))
#define ma_ntohs(u16)   (MA_BSWAP16(u16))

#define ma_htonl(u32)   (MA_BSWAP32(u32))
#define ma_ntohl(u32)   (MA_BSWAP32(u32))

#define ma_htole16(u16) (u16)
#define ma_le16toh(u16) (u16)

#define ma_htole32(u32) (u32)
#define ma_le32toh(u32) (u32)

#elif (MA_BYTE_ORDER == MA_BIG_ENDIAN)

#define ma_htons(u16)   (u16)
#define ma_ntohs(u16)   (u16)

#define ma_htonl(u32)   (u32)
#define ma_ntohl(u32)   (u32)

#define ma_htole16(u16) (MA_BSWAP16(u16))
#define ma_le16toh(u16) (MA_BSWAP16(u16))

#define ma_htole32(u32) (MA_BSWAP32(u32))
#define ma_le32toh(u32) (MA_BSWAP32(u32))
#else
#error "Byte order is undefined"
#endif

#ifdef SWIG
#define MA_EXPORT
#elif defined(MA_STATIC_LIBRARY_BUILD)
#define MA_EXPORT
#else  // not definded MA_STATIC_LIBRARY_BUILD
#if defined(_WIN32)
#ifdef MA_COMPILE_LIBRARY
#define MA_EXPORT __declspec(dllexport)
#else
#define MA_EXPORT __declspec(dllimport)
#endif  // MA_COMPILE_LIBRARY
#else
#define MA_EXPORT __attribute__((visibility("default")))
#endif  // _WIN32
#endif  // SWIG


#if defined(__x86_64__) || defined(__ppc64__) || defined(_LP64)
#define MA_ARCH_64 1
#else
#define MA_ARCH_32 1
#endif

#endif  // _MA_COMPILER_H_