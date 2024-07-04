#ifndef _MA_MISC_H_
#define _MA_MISC_H_

#include <stddef.h>
#include <stdint.h>

#include "core/ma_config_internal.h"

#ifdef MA_OSAL_PTHREAD
#include "porting/posix/ma_misc_posix.h"
#else
#error "Unsupported OS"
#endif


#endif  // MA_PORTING_H_