#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void set_action(const std::vector<std::string>& argv) {
    auto        os                     = std::ostringstream(std::ios_base::ate);
    std::string cmd                    = {};
    char        action[CMD_MAX_LENGTH] = {};
    uint16_t    crc16_maxim            = 0xffff;
    auto        ret                    = EL_OK;

    if (argv[1].size()) [[likely]] {
        ret = static_resourse->action_cond->set_condition(argv[1]) ? EL_OK : EL_EINVAL;
        if (ret != EL_OK) [[unlikely]]
            goto ActionReply;
    }
    if (argv[2].size()) [[likely]] {
        cmd = argv[2];
        cmd.insert(0, "AT+");
        static_resourse->action_cond->set_true_cb([=]() {
            auto ret = static_resourse->instance->exec_non_lock(cmd);
            auto os  = std::ostringstream(std::ios_base::ate);
            os << REPLY_EVT_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
               << ", \"data\": {\"true\": " << quoted_stringify(argv[2]) << "}}\n";
            auto str{os.str()};
            static_resourse->transport->send_bytes(str.c_str(), str.size());
        });
    }
    if (argv[3].size()) [[likely]] {
        cmd = argv[3];
        cmd.insert(0, "AT+");
        static_resourse->action_cond->set_false_cb([=]() { static_resourse->instance->exec_non_lock(cmd); });
    }
    if (argv[4].size()) [[likely]] {
        cmd = argv[4];
        cmd.insert(0, "AT+");
        static_resourse->action_cond->set_exception_cb([=]() { static_resourse->instance->exec_non_lock(cmd); });
    }

    if (static_resourse->is_ready.load()) [[likely]] {
        auto builder = std::ostringstream(std::ios_base::ate);
        builder << argv[1] << '\t' << argv[2] << '\t' << argv[3] << '\t' << argv[4];
        cmd = builder.str();

        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.size());
        std::strncpy(action, cmd.c_str(), sizeof(action) - 1);
        ret = static_resourse->storage->emplace(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EIO;
    }

ActionReply:
    os << REPLY_CMD_HEADER << "\"name\": \"" << argv[0] << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"crc16_maxim\": " << static_cast<unsigned>(crc16_maxim)
       << ", \"cond\": " << quoted_stringify(argv[1]) << ", \"true\": " << quoted_stringify(argv[2])
       << ", \"false\": " << quoted_stringify(argv[3]) << ", \"execption\": " << quoted_stringify(argv[4]) << "}}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

void get_action(const std::string& cmd) {
    auto     os                     = std::ostringstream(std::ios_base::ate);
    char     action[CMD_MAX_LENGTH] = {};
    uint16_t crc16_maxim            = 0xffff;
    auto     ret                    = EL_OK;

    if (static_resourse->action_cond->has_condition() && static_resourse->storage->contains("edgelab_action")) {
        ret         = static_resourse->storage->get(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EINVAL;
        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(&action[0]), std::strlen(&action[0]));
    }

    os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
       << ", \"data\": {\"crc16_maxim\": " << static_cast<unsigned>(crc16_maxim) << ", " << action_str_2_json_str(action)
       << "}}\n";

    const auto& str{os.str()};
    static_resourse->transport->send_bytes(str.c_str(), str.size());
}

}  // namespace sscma::callback
