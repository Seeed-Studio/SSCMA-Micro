#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void set_action(const std::vector<std::string>& argv) {
    std::string ss(REPLY_CMD_HEADER);
    char*       action      = new char[CMD_MAX_LENGTH]{};
    uint16_t    crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(argv[1].c_str()), argv[1].size());
    auto        ret         = EL_OK;

    if (argv[1].size()) [[likely]] {
        ret = static_resourse->action_cond->set_condition(argv[1]) ? EL_OK : EL_EINVAL;
        if (ret != EL_OK) [[unlikely]]
            goto ActionReply;

        static_resourse->action_cond->set_exception_cb([=]() {
            std::string ss(REPLY_EVT_HEADER);

            ss += concat_strings("\"name\": \"",
                                 argv[0],
                                 "\", \"code\": ",
                                 std::to_string(EL_ELOG),
                                 ", \"data\": {\"crc16_maxim\": ",
                                 std::to_string(crc16_maxim),
                                 "}}\n");

            static_resourse->transport->send_bytes(ss.c_str(), ss.size());
        });
    }

    if (static_resourse->is_ready.load()) [[likely]]
        ret = static_resourse->storage->emplace(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EIO;

ActionReply:
    ss += concat_strings("\"name\": \"",
                         argv[0],
                         "\", \"code\": ",
                         std::to_string(ret),
                         ", \"data\": {\"crc16_maxim\": ",
                         std::to_string(crc16_maxim),
                         ", \"action\": ",
                         quoted(argv[1]),
                         "}}\n");

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
    delete action;
}

void get_action(const std::string& cmd) {
    std::string ss(REPLY_CMD_HEADER);
    char*       action      = new char[CMD_MAX_LENGTH]{};
    uint16_t    crc16_maxim = 0xffff;
    auto        ret         = EL_OK;

    if (static_resourse->action_cond->has_condition() && static_resourse->storage->contains("edgelab_action")) {
        ret         = static_resourse->storage->get(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EINVAL;
        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(&action[0]), std::strlen(&action[0]));
    }

    ss += concat_strings("\"name\": \"",
                         cmd,
                         "\", \"code\": ",
                         std::to_string(ret),
                         ", \"data\": {\"crc16_maxim\": ",
                         std::to_string(crc16_maxim),
                         ", ",
                         action_str_2_json_str(action),
                         "}}\n");

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
