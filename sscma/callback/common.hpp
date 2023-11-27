#pragma once

#include <atomic>
#include <string>

#include "core/el_version.h"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace edgelab;

using namespace sscma::utility;

void print_help(const std::forward_list<repl_cmd_t>& cmd_list, void* caller) {
    std::string ss;

    for (const auto& cmd : cmd_list) {
        ss += concat_strings("  AT+", cmd.cmd);
        if (cmd.args.size()) [[likely]]
            ss += concat_strings("=<", cmd.args, ">");
        ss += concat_strings("\n    ", cmd.desc, "\n");
    }

    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_device_id(const std::string& cmd, void* caller) {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  std::to_string(static_resource->device->get_device_id()),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_device_name(const std::string& cmd, void* caller) {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  quoted(static_resource->device->get_device_name()),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_device_status(const std::string& cmd, void* caller) {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": {\"boot_count\": ",
                                  std::to_string(static_resource->boot_count),
                                  ", \"is_ready\": ",
                                  std::to_string(static_resource->is_ready.load() ? 1 : 0),
                                  "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_version(const std::string& cmd, void* caller) {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": {\"software\": \"",
                                  EL_VERSION,
                                  "\", \"hardware\": \"",
                                  std::to_string(static_resource->device->get_chip_revision_id()),
                                  "\"}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void break_task(const std::string& cmd, void* caller) {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  std::to_string(el_get_time_ms()),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void task_status(const std::string& cmd, bool sta, void* caller) {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  std::to_string(sta ? 1 : 0),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
