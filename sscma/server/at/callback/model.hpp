#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma::model;

void getAvailableModels(const std::vector<std::string>& args, Transport& transport, Encoder& encoder) {
    MA_ASSERT(args.size() >= 1);
    const auto& cmd = args[0];

    auto& models = static_resource->device->getModels();
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write(models);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void configureModel(const std::vector<std::string>& args,
                    Transport&                      transport,
                    Encoder&                        encoder,
                    bool                            called_by_event = false) {
    MA_ASSERT(args.size() >= 2);
    const auto&  cmd      = args[0];
    const size_t model_id = std::atoi(args[1].c_str());

    ma_err_t ret    = MA_OK;
    auto&    models = static_resource->device->getModels();

    auto it = std::find_if(models.begin(), models.end(), [&](const ma_model_t& m) { return m.id == model_id; });
    if (it == models.end()) {
        ret = MA_ENOENT;
        goto exit;
    }

#if MA_USE_FILESYSTEM_POSIX
    ret = static_resource->engine->load(static_cast<const char*>(it->addr));
#else
    ret = static_resource->engine->load(it->addr, it->size);
#endif
    if (ret != MA_OK) [[unlikely]]
        goto exit;

    if (static_resource->current_model_id != model_id) {
        static_resource->current_model_id = model_id;
        if (!called_by_event) {
            MA_STORAGE_SET_POD(
              ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MODEL_ID, static_resource->current_model_id);
        }
    }

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    if (static_resource->current_model_id) {
        encoder.write("model", it == models.end() ? ma_model_t{} : *it);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void initDefaultModel(Encoder& encoder) {
    if (static_resource->device->getTransports().empty()) {
        MA_LOGD(MA_TAG, "No transport available");
        return;
    }
    auto& transport = static_resource->device->getTransports().front();
    if (!transport || !*transport) {
        MA_LOGD(MA_TAG, "Transport not available");
        return;
    }

    size_t model_id = 0;

    MA_STORAGE_GET_POD(static_resource->device->getStorage(), MA_STORAGE_KEY_MODEL_ID, model_id, 1);

    std::vector<std::string> args{"INIT@MODEL", std::to_string(static_cast<int>(model_id))};
    configureModel(args, *transport, encoder, true);
}

void getModelInfo(const std::vector<std::string>& args, Transport& transport, Encoder& encoder) {
    MA_ASSERT(args.size() >= 1);
    const auto& cmd = args[0];

    ma_err_t ret    = MA_OK;
    auto&    models = static_resource->device->getModels();

    auto it = std::find_if(
      models.begin(), models.end(), [&](const ma_model_t& m) { return m.id == static_resource->current_model_id; });
    if (it == models.end()) {
        ret = MA_ENOENT;
    }

    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    if (it != models.end()) {
        std::vector<ma_model_t> model{*it};
        encoder.write(model);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
