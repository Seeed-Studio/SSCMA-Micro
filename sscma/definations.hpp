#pragma once

#include "core/el_config_internal.h"

#ifndef CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE
    #define CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE (20480)
#endif

#ifndef CONFIG_SSCMA_REPL_EXECUTOR_PRIO
    #define CONFIG_SSCMA_REPL_EXECUTOR_PRIO (5)
#endif

#ifndef CONFIG_SSCMA_REPL_HISTORY_MAX
    #define CONFIG_SSCMA_REPL_HISTORY_MAX (8)
#endif

#ifndef CONFIG_SSCMA_TENSOR_ARENA_SIZE
    #define CONFIG_SSCMA_TENSOR_ARENA_SIZE (1024 * 1024)
#endif

#ifndef CONFIG_SSCMA_CMD_MAX_LENGTH
    #define CONFIG_SSCMA_CMD_MAX_LENGTH (4096)
#endif

#define SSCMA_STORAGE_KEY_VERSION "sscma#core#version"
#define SSCMA_STORAGE_KEY_ACTION  "sscma#action"
#define SSCMA_STORAGE_KEY_INFO    "sscma#info"
