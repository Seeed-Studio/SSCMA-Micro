#ifndef _MA_DEBUG_H_
#define _MA_DEBUG_H_

#include "ma_config_internal.h"
#include "../porting/ma_misc.h"

#ifndef MA_TAG
    #define MA_TAG_FILE __FILE__ ":" MA_STRINGIZE(__LINE__)
    #define MA_TAG      (strrchr(MA_TAG_FILE, '/') ? strrchr(MA_TAG_FILE, '/') + 1 : MA_TAG_FILE)
#endif

#if MA_DEBUG_LEVEL

#define MA_DEBUG_COLOR_RED     "\033[31m"
#define MA_DEBUG_COLOR_GREEN   "\033[32m"
#define MA_DEBUG_COLOR_YELLOW  "\033[33m"
#define MA_DEBUG_COLOR_BLUE    "\033[34m"
#define MA_DEBUG_COLOR_MAGENTA "\033[35m"
#define MA_DEBUG_COLOR_CYAN    "\033[36m"
#define MA_DEBUG_COLOR_RESET   "\033[0m"

#if MA_USE_DEBUG_MORE_INFO
#define MA_DEBUG_MORE_INFO(TAG) ma_printf("%s[%d] (%ld): ", TAG, __LINE__, ma_get_time_ms());
#else
#define MA_DEBUG_MORE_INFO(TAG)
#endif

#if MA_DEBUG_LEVEL >= 1
#ifndef MA_LOGE
#define MA_LOGE(TAG, ...)                       \
    do {                                        \
        ma_printf(MA_DEBUG_COLOR_RED "E ");     \
        MA_DEBUG_MORE_INFO(TAG);                \
        ma_printf(__VA_ARGS__);                 \
        ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
    } while (0)
#endif
#else
#ifdef MA_LOGE
#undef MA_LOGE
#endif
#define MA_LOGE(TAG, ...)
#endif

#if MA_DEBUG_LEVEL >= 2
#ifndef MA_LOGW
#define MA_LOGW(TAG, ...)                       \
    do {                                        \
        ma_printf(MA_DEBUG_COLOR_YELLOW "W ");  \
        MA_DEBUG_MORE_INFO(TAG);                \
        ma_printf(__VA_ARGS__);                 \
        ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
    } while (0)
#endif
#else
#ifdef MA_LOGW
#undef MA_LOGW
#endif
#define MA_LOGW(TAG, ...)
#endif
#if MA_DEBUG_LEVEL >= 3
#ifndef MA_LOGI
#define MA_LOGI(TAG, ...)                       \
    do {                                        \
        ma_printf(MA_DEBUG_COLOR_GREEN "I ");   \
        MA_DEBUG_MORE_INFO(TAG);                \
        ma_printf(__VA_ARGS__);                 \
        ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
    } while (0)
#endif
#else
#ifdef MA_LOGI
#undef MA_LOGI
#endif
#define MA_LOGI(TAG, ...)
#endif
#if MA_DEBUG_LEVEL >= 4
#ifndef MA_LOGD
#define MA_LOGD(TAG, ...)                       \
    do {                                        \
        ma_printf(MA_DEBUG_COLOR_BLUE "D ");    \
        MA_DEBUG_MORE_INFO(TAG);                \
        ma_printf(__VA_ARGS__);                 \
        ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
    } while (0)
#endif
#else
#ifdef MA_LOGD
#undef MA_LOGD
#endif
#define MA_LOGD(TAG, ...)
#endif
#if MA_DEBUG_LEVEL >= 5
#ifndef MA_LOGV
#define MA_LOGV(TAG, ...)                       \
    do {                                        \
        ma_printf(MA_DEBUG_COLOR_MAGENTA "V "); \
        MA_DEBUG_MORE_INFO(TAG);                \
        ma_printf(__VA_ARGS__);                 \
        ma_printf(MA_DEBUG_COLOR_RESET "\r\n"); \
    } while (0)
#endif
#else
#ifdef MA_LOGV
#undef MA_LOGV
#endif
#define MA_LOGV(TAG, ...)
#endif
#else
#define MA_ELOG(TAG, ...)
#define MA_LOGW(TAG, ...)
#define MA_LOGI(TAG, ...)
#define MA_LOGD(TAG, ...)
#define MA_LOGV(TAG, ...)
#endif

#if MA_USE_ASSERT
#ifndef MA_ASSERT
#define MA_ASSERT(expr)                                \
    do {                                               \
        if (!(expr)) {                                 \
            ma_printf(MA_DEBUG_COLOR_MAGENTA "E ");    \
            MA_DEBUG_MORE_INFO("ASSERT");              \
            ma_printf("Failed assertion '%s'", #expr); \
            ma_printf(MA_DEBUG_COLOR_RESET "\r\n");    \
            while (1) {                                \
                ma_abort();                            \
            }                                          \
        }                                              \
    } while (0)
#endif
#else
#ifdef MA_ASSERT
#undef MA_ASSERT
#endif
#define MA_ASSERT(expr)
#endif

#ifndef MA_USE_STATIC_ASSERT
#ifdef MA_STATIC_ASSERT
#undef MA_STATIC_ASSERT
#define MA_STATIC_ASSERT(expr, msg)
#endif
#endif

#endif  // _MA_DEBUG_H_