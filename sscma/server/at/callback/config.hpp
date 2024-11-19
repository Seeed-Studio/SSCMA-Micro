#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma;

void setScoreThreshold(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    {
        int score = std::atoi(argv[1].c_str());
        if (score <= 0 || score > 100) {
            ret = MA_EINVAL;
            goto exit;
        }

        MA_STORAGE_SET_POD(ret, static_resource->device->getStorage(), "ma#score_threshold", score);
        if (ret != MA_OK) {
            goto exit;
        }

        static_resource->shared_threshold_score = score / 100.0;
    }

exit:
    auto value = static_cast<uint64_t>(std::round(static_resource->shared_threshold_score * 100));
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0], value);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void setNMSThreshold(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    {
        int nms = std::atoi(argv[1].c_str());
        if (nms <= 0 || nms > 100) {
            ret = MA_EINVAL;
            goto exit;
        }

        MA_STORAGE_SET_POD(ret, static_resource->device->getStorage(), "ma#nms_threshold", nms);
        if (ret != MA_OK) {
            goto exit;
        }

        static_resource->shared_threshold_nms = nms / 100.0;
    }

exit:
    auto value = static_cast<uint64_t>(std::round(static_resource->shared_threshold_nms * 100));
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0], value);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getScoreThreshold(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    int score = std::round(static_resource->shared_threshold_score * 100);
    MA_STORAGE_GET_POD(static_resource->device->getStorage(), "ma#score_threshold", score, score);

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0], static_cast<uint64_t>(score));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getNMSThreshold(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    int nms = std::round(static_resource->shared_threshold_nms * 100);
    MA_STORAGE_GET_POD(static_resource->device->getStorage(), "ma#nms_threshold", nms, nms);

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0], static_cast<uint64_t>(nms));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
