#ifndef _MA_MISC_H_
#define _MA_MISC_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ma_malloc  malloc
#define ma_calloc  calloc
#define ma_realloc realloc
#define ma_free    free
#define ma_printf  printf
#define ma_abort   abort
#define ma_reset   abort

#endif  // _MA_MISC_H_