#pragma once

#include <functional>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"


namespace ma::server::callback {

using namespace ma;

void storeRC(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    MA_STORAGE_SET_STR(ret, static_resource->device->getStorage(), MA_AT_CMD_RC, argv[1]);

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("rc", argv.size() < 2 ? "" : argv[1]);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void readRC(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t    ret = MA_OK;
    std::string value;

    if (argv.size() < 1) {
        ret = MA_EINVAL;
        goto exit;
    }

    MA_STORAGE_GET_STR(static_resource->device->getStorage(), MA_AT_CMD_RC, value, "");

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("rc", value);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void initDefaultRC(Transport* transport, Encoder& encoder, std::function<void(const std::string&, Transport&)> execute) {
    if (!transport) {
        MA_LOGD(MA_TAG, "Transport not available");
        return;
    }
    if (!execute) {
        MA_LOGD(MA_TAG, "Execute function not available");
        return;
    }

    std::string rc;
    MA_STORAGE_GET_STR(static_resource->device->getStorage(), MA_AT_CMD_RC, rc, "");
    // AT+...;AT+...;AT+...;

    size_t start = 0;
    size_t end   = rc.find(";");
    while (end != std::string::npos) {
        std::string cmd = rc.substr(start, end - start);
        execute(cmd, *transport);
        start = end + 1;
        end   = rc.find(";", start);
    }

    if (start < rc.size()) {
        std::string cmd = rc.substr(start);
        execute(cmd, *transport);
    }
}

}
