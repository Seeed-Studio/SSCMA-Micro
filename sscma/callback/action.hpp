#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "core/utils/el_hash.h"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace edgelab;

using namespace sscma::utility;

void set_action(const std::vector<std::string>& argv, bool called_by_event = false) {
    // get last condition expr string hash
    auto hash = static_resource->action->get_condition_hash();
    // set current condition expr string
    auto ret = static_resource->action->set_condition(argv[1]) ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto ActionReply;

    // update exception callback (cmd changes)
    static_resource->action->set_exception_cb([cmd = argv[0],
                                               exp = quoted(argv[1]),
                                               ec  = std::to_string(EL_ELOG),
                                               crc = std::to_string(static_resource->action->get_condition_hash())]() {
        const auto& ss = concat_strings("\r{\"type\": 1, \"name\": \"",
                                        cmd,
                                        "\", \"code\": ",
                                        ec,
                                        ", \"data\": {\"crc16_maxim\": ",
                                        crc,
                                        ", \"action\": ",
                                        exp,
                                        "}}\n");
        static_resource->transport->send_bytes(ss.c_str(), ss.size());
    });

    // compare hash to check if condition expr string changed
    if (hash != static_resource->action->get_condition_hash()) [[likely]] {
        hash             = static_resource->action->get_condition_hash();  // update last hash
        auto mutable_map = static_resource->action->get_mutable_map();     // get mutable map
        for (auto& kv : mutable_map) {
            auto argv = tokenize_function_2_argv(kv.first);
            if (!argv.size()) [[unlikely]]  // if argv is empty, skip
                continue;

            // LED action
            if (argv[0] == "led") {
                if (argv.size() == 2) [[likely]] {  // LED action with 1 argument
                    bool enable = std::atoi(argv[1].c_str());
                    kv.second   = [enable]() -> int {
                        el_status_led(enable);
                        return 1;
                    };
                }
            }
        }
        static_resource->action->set_mutable_map(mutable_map);  // update mutable map

        if (!called_by_event) {
            char action[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
            std::strncpy(
              action,
              argv[1].c_str(),
              argv[1].length() < CONFIG_SSCMA_CMD_MAX_LENGTH ? argv[1].length() : CONFIG_SSCMA_CMD_MAX_LENGTH - 1);
            ret =
              static_resource->storage->emplace(el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action)) ? EL_OK : EL_EIO;
        }
    }

ActionReply:
    const auto& ss{concat_strings("\r{\"type\": ",
                                  std::to_string(called_by_event ? 1 : 0),
                                  ", \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"crc16_maxim\": ",
                                  std::to_string(hash),
                                  ", \"action\": ",
                                  quoted(argv[1]),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_action(const std::string& cmd) {
    char     action[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
    uint16_t crc16_maxim = 0xffff;
    auto     ret         = EL_OK;

    // if action is set and a action is stored in flash, get the action from flash
    if (static_resource->action->has_condition() && static_resource->storage->contains(SSCMA_STORAGE_KEY_ACTION)) {
        ret = static_resource->storage->get(el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action)) ? EL_OK : EL_EIO;
        crc16_maxim = el_crc16_maxim(reinterpret_cast<const uint8_t*>(action), std::strlen(action));
    }

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"crc16_maxim\": ",
                                  std::to_string(crc16_maxim),
                                  ", \"action\": ",
                                  quoted(action),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
