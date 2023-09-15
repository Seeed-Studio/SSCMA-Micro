#pragma once

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;


void print_help(std::forward_list<repl_cmd_t> cmd_list) {
    auto os = std::ostringstream(std::ios_base::ate);

    for (const auto& cmd : cmd_list) {
        os << "  AT+" << cmd.cmd;
        if (cmd.args.size()) os << "=<" << cmd.args << ">";
        os << "\n    " << cmd.desc << "\n";
    }

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void get_device_id(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": \""
       << std::uppercase << std::hex << static_resourse->device->get_device_id() << "\"}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void get_device_name(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK) << ", \"data\": \""
       << static_resourse->device->get_device_name() << "\"}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void get_device_status(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": {\"boot_count\": " << static_cast<unsigned>(static_resourse->boot_count)
       << ", \"is_ready\": " << (static_resourse->is_ready.load() ? 1 : 0) << "}}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void get_version(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": {\"software\": \"" << EL_VERSION << "\", \"hardware\": \""
       << static_cast<unsigned>(static_resourse->device->get_chip_revision_id()) << "\"}}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void break_task(const std::string& cmd) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": " << el_get_time_ms() << "}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void task_status(const std::string& cmd, const std::atomic<bool>& sta) {
    auto os = std::ostringstream(std::ios_base::ate);

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(EL_OK)
       << ", \"data\": " << (sta.load() ? 1 : 0) << "}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

}  // namespace sscma::callback
