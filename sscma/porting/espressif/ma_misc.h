#ifndef _MA_MISC_ESPRESSIF_
#define _MA_MISC_ESPRESSIF_

#include "core/ma_common.h"


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
#define ma_reset   esp_restart

#endif  // _MA_MISC_ESPRESSIF_