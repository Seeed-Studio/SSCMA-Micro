#pragma once

#define REPL_EXECUTOR_STACK_SIZE (40960)
#define REPL_EXECUTOR_PRIO       (5)
#define REPL_HISTORY_MAX         (8)

#define TENSOR_ARENA_SIZE        (1024 * 1024)

#define CMD_MAX_LENGTH           (4096)

#define REPLY_CMD_HEADER         "\r{\"type\": 0, "
#define REPLY_EVT_HEADER         "\r{\"type\": 1, "
#define REPLY_LOG_HEADER         "\r{\"type\": 2, "
