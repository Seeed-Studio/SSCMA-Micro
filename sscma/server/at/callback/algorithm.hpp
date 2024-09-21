#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

#define MA_STORAGE_KEY_ALGORITHM_ID "ma#algorithm_id"

namespace ma::server::callback {

void getAvailableAlgorithms(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    // Need to refactor the algorithms
    // - A registry of algorithms is needed and the registry should be able to provide the list of available algorithms
    // - Each algorithm in the registry should be able to provide the information about itself
}

void configureAlgorithm(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    MA_ASSERT(argv.size() >= 2);
    const auto& cmd          = argv[0];
    const auto  algorithm_id = std::atoi(argv[1].c_str());

    ma_err_t ret = MA_OK;
    if (algorithm_id != static_resource->current_algorithm_id) {
        static_resource->current_algorithm_id = algorithm_id;
        MA_STORAGE_SET_POD(ret,
                           static_resource->device->getStorage(),
                           MA_STORAGE_KEY_ALGORITHM_ID,
                           static_resource->current_algorithm_id);
    }

    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    encoder.write("algorithm", static_cast<uint32_t>(static_resource->current_algorithm_id));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getAlgorithmInfo(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    MA_ASSERT(argv.size() >= 1);
    const auto& cmd = argv[0];

    ma_err_t ret = MA_OK;
    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    encoder.write("algorithm", static_cast<uint32_t>(static_resource->current_algorithm_id));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
