#ifndef _MA_MISC_H_
#define _MA_MISC_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>
#include <stdint.h>

#include "core/ma_compiler.h"
#include "core/ma_config_internal.h"

#if MA_PORTING_POSIX
#include "porting/posix/ma_misc_posix.h"
#elif MA_PORTING_ESPRESSIF
#include "porting/espressif/ma_misc_espressif.h"
#else
#error "Unsupported platform"
#endif

#ifndef ma_malloc
void* ma_malloc(size_t size);
#endif

#ifndef ma_calloc
void* ma_calloc(size_t nmemb, size_t size);
#endif

#ifndef ma_realloc
void* ma_realloc(void* ptr, size_t size);
#endif

#ifndef ma_free
void ma_free(void* ptr);
#endif

#ifndef ma_abort
void ma_abort(void);
#endif

#ifndef ma_reset
void ma_reset(void);
#endif

#ifndef ma_printf
int ma_printf(const char* fmt, ...);
#endif

#ifndef ma_sleep
void ma_sleep(uint32_t ms);
#endif

#ifndef ma_usleep
void ma_usleep(uint32_t us);
#endif

#ifndef ma_get_time_us
int64_t ma_get_time_us(void);
#endif

#ifndef ma_get_time_ms
int64_t ma_get_time_ms(void);
#endif

#ifdef __cplusplus
}
#endif

#endif  // _MA_MISC_H_