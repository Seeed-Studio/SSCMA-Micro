#pragma once

#include <cstring>
#include <string>

#include "core/utils/el_hash.h"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace edgelab;
using namespace sscma::utility;

void set_info(const std::vector<std::string>& argv, void* caller) {
    char info[SSCMA_CMD_MAX_LENGTH]{};
    char key[sizeof(SSCMA_STORAGE_KEY_INFO) + 4]{};
    std::strncpy(info, argv[1].c_str(), sizeof(info) - 1);
    snprintf(key, sizeof(key), SSCMA_STORAGE_KEY_INFO "#%d", static_resource->current_model_id);
    el_err_code_t ret = EL_OK;
    uint8_t       i   = 0;
    for (i = 0; i < 5; i++) {
        ret = static_resource->storage->emplace(el_make_storage_kv(key, info)) ? EL_OK : EL_EIO;
        if (ret == EL_OK) break;
    }
    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": ",
                           std::to_string(ret),
                           ", \"data\": {\"crc16_maxim\": ",
                           std::to_string(el_crc16_maxim(reinterpret_cast<uint8_t*>(&info[0]), std::strlen(&info[0]))),
                           ", \"retries\": ",
                           std::to_string(i),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_info(const std::string& cmd, void* caller) {
    char info[SSCMA_CMD_MAX_LENGTH]{};
    char key[sizeof(SSCMA_STORAGE_KEY_INFO) + 4]{};
    auto ret = EL_OK;

    snprintf(key, sizeof(key), SSCMA_STORAGE_KEY_INFO "#%d", static_resource->current_model_id);

    if (static_resource->storage->contains(key))
        ret = static_resource->storage->get(el_make_storage_kv(key, info)) ? EL_OK : EL_EIO;

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           cmd,
                           "\", \"code\": ",
                           std::to_string(ret),
                           ", \"data\": {\"crc16_maxim\": ",
                           std::to_string(el_crc16_maxim(reinterpret_cast<uint8_t*>(&info[0]), std::strlen(&info[0]))),
                           ", \"info\": ",
                           quoted(info),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
