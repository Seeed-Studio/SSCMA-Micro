#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

namespace sscma::types {

typedef std::function<void(std::atomic<bool>&)> repl_task_t;

typedef std::function<void(void)>                     branch_cb_t;
typedef std::function<int(void)>                      mutable_cb_t;
typedef std::unordered_map<std::string, mutable_cb_t> mutable_map_t;

}  // namespace sscma::types
