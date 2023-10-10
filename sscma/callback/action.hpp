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
    auto hash = static_resourse->action_cond->get_condition_hash();
    auto ret  = static_resourse->action_cond->set_condition(argv[1]) ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto ActionReply;

    if (hash != static_resourse->action_cond->get_condition_hash()) [[likely]] {
        hash = static_resourse->action_cond->get_condition_hash();
        static_resourse->action_cond->set_exception_cb(
          [cmd = argv[0],
           exp = quoted(argv[1]),
           ec  = std::to_string(EL_ELOG),
           crc = std::to_string(static_resourse->action_cond->get_condition_hash())]() {
              std::string ss{concat_strings(REPLY_EVT_HEADER,
                                            "\"name\": \"",
                                            cmd,
                                            "\", \"code\": ",
                                            ec,
                                            ", \"data\": {\"crc16_maxim\": ",
                                            crc,
                                            ", \"action\": ",
                                            exp,
                                            "}}\n")};

              static_resourse->transport->send_bytes(ss.c_str(), ss.size());
          });
        auto mutable_map = static_resourse->action_cond->get_mutable_map();
        for (auto& kv : mutable_map) {
            const auto& argv = tokenize_function_2_argv(kv.first);
            if (!argv.size()) [[unlikely]]
                continue;

            if (argv[0] == "led") {
                if (argv.size() == 2) {
                    bool enable = std::atoi(argv[1].c_str());
                    kv.second   = [enable]() -> int {
                        el_status_led(enable);
                        return 1;
                    };
                }
            }
        }
        static_resourse->action_cond->set_mutable_map(mutable_map);

        if (static_resourse->is_ready.load()) [[likely]] {
            char action[CMD_MAX_LENGTH]{};
            std::strncpy(
              action, argv[1].c_str(), argv[1].length() < CMD_MAX_LENGTH ? argv[1].length() : CMD_MAX_LENGTH - 1);
            ret = static_resourse->storage->emplace(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EIO;
        }
    }

ActionReply:
    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"crc16_maxim\": ",
                                  std::to_string(hash),
                                  ", \"action\": ",
                                  quoted(argv[1]),
                                  "}}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_action(const std::string& cmd) {
    char     action[CMD_MAX_LENGTH]{};
    uint16_t crc16_maxim = 0xffff;
    auto     ret         = EL_OK;

    if (static_resourse->action_cond->has_condition() && static_resourse->storage->contains("edgelab_action")) {
        ret         = static_resourse->storage->get(el_make_storage_kv("edgelab_action", action)) ? EL_OK : EL_EINVAL;
        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(action), std::strlen(action));
    }

    std::string ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"crc16_maxim\": ",
                                  std::to_string(crc16_maxim),
                                  ", \"action\": ",
                                  quoted(action),
                                  "}}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
