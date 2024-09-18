#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma;

void storeInfo(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t    ret = MA_OK;
    std::string key = MA_AT_CMD_INFO;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    key += '#';
    key += std::to_string(static_cast<int>(static_resource->current_model_id));

    MA_STORAGE_SET_STR(ret, static_resource->device->getStorage(), key, argv[1]);

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("info", argv.size() < 2 ? "" : argv[1]);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void readInfo(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t    ret = MA_OK;
    std::string key = MA_AT_CMD_INFO;
    std::string value;

    if (argv.size() < 1) {
        ret = MA_EINVAL;
        goto exit;
    }

    key += '#';
    key += std::to_string(static_cast<int>(static_resource->current_model_id));

    MA_STORAGE_GET_STR(static_resource->device->getStorage(), key, value, "");

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("info", value);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback