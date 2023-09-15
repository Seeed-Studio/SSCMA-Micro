#pragma once

#include <functional>
#include <sstream>

#define REPL_EXECUTOR_STACK_SIZE (40960)
#define REPL_EXECUTOR_PRIO       (5)
#define REPL_HISTORY_MAX         (8)

#define TENSOR_ARENA_SIZE        (1024 * 1024)

#define CMD_MAX_LENGTH           (4096)

#define REPLY_CMD_HEADER         "\r{\"type\": 0, "
#define REPLY_EVT_HEADER         "\r{\"type\": 1, "
#define REPLY_LOG_HEADER         "\r{\"type\": 2, "

namespace sscma::definations {

using delim_f_t                = std::function<void(std::ostringstream& os)>;
static delim_f_t delim_f       = [](std::ostringstream& os) {};
static delim_f_t print_delim_f = [](std::ostringstream& os) { os << ", "; };
static delim_f_t print_void_f  = [](std::ostringstream& os) { delim_f = print_delim_f; };

#define DELIM_RESET \
    { sscma::definations::delim_f = sscma::definations::print_void_f; }
#define DELIM_PRINT(OS) \
    { sscma::definations::delim_f(OS); }

}  // namespace sscma::definations
