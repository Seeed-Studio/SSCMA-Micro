#ifndef _MA_CONFIG_BOARD_H_
#define _MA_CONFIG_BOARD_H_

#include <stdio.h>
#include <stdlib.h>

#define MA_PORTING_SG200X               1
#define MA_BOARD_RECAMERA               1
#define MA_BOARD_NAME                   "recamera"
#define MA_CONFIG_FILE                  "/etc/sscma.conf"
#define MA_MODEL_DIR                    "/userdata/MODEL/"
#define MA_OSAL_PTHREAD                 1
#define MA_USE_FILESYSTEM               1
#define MA_USE_FILESYSTEM_POSIX         1
#define MA_USE_ENGINE_CVI               1
#define MA_USE_ENGINE_TENSOR_NAME       1
#define MA_USE_TRANSPORT_MQTT           1
#define MA_SEVER_AT_EXECUTOR_STACK_SIZE 80 * 1024
#define MA_SEVER_AT_EXECUTOR_TASK_PRIO  2

#define ma_malloc                       malloc
#define ma_calloc                       calloc
#define ma_realloc                      realloc
#define ma_free                         free
#define ma_printf                       printf
#define ma_abort                        abort
#define ma_reset                        abort

#endif  // _MA_CONFIG_BOARD_H_