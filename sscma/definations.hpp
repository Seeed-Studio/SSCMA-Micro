#pragma once

#include "core/el_config_internal.h"

#ifndef REPL_EXECUTOR_STACK_SIZE
    #define REPL_EXECUTOR_STACK_SIZE (20480)
#endif

#ifndef REPL_EXECUTOR_PRIO
    #define REPL_EXECUTOR_PRIO (5)
#endif

#ifndef REPL_HISTORY_MAX
    #define REPL_HISTORY_MAX (8)
#endif

#ifndef TENSOR_ARENA_SIZE
    #define TENSOR_ARENA_SIZE (1024 * 1024)
#endif

#ifndef CMD_MAX_LENGTH
    #define CMD_MAX_LENGTH (4096)
#endif

#define SSCMA_STORAGE_KEY_VERSION "sscma#core#version"
#define SSCMA_STORAGE_KEY_ACTION  "sscma#action"
#define SSCMA_STORAGE_KEY_INFO    "sscma#info"
