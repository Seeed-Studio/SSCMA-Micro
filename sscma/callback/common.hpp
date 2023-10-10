#pragma once

#include <atomic>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void print_help(std::forward_list<repl_cmd_t> cmd_list) {
    std::string ss;

    for (const auto& cmd : cmd_list) {
        ss += concat_strings("  AT+", cmd.cmd);
        if (cmd.args.size()) [[likely]]
            ss += concat_strings("=<", cmd.args, ">");
        ss += concat_strings("\n    ", cmd.desc, "\n");
    }

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_device_id(const std::string& cmd) {
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  std::to_string(static_resourse->device->get_device_id()),
                                  "}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_device_name(const std::string& cmd) {
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  quoted(static_resourse->device->get_device_name()),
                                  "}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_device_status(const std::string& cmd) {
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": {\"boot_count\": ",
                                  std::to_string(static_resourse->boot_count),
                                  ", \"is_ready\": ",
                                  std::to_string(static_resourse->is_ready.load() ? 1 : 0),
                                  "}}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_version(const std::string& cmd) {
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": {\"software\": \"",
                                  EL_VERSION,
                                  "\", \"hardware\": \"",
                                  std::to_string(static_resourse->device->get_chip_revision_id()),
                                  "\"}}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void break_task(const std::string& cmd) {
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  std::to_string(el_get_time_ms()),
                                  "}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void task_status(const std::string& cmd, const std::atomic<bool>& sta) {
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  std::to_string(sta.load() ? 1 : 0),
                                  "}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
