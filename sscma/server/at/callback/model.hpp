#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma::model;

void get_available_models(const std::string& cmd, Transport& transport, Encoder& encoder) {
    auto& models = static_resource->device->getModels();
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write(models);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void set_model(
  const std::string& cmd, Transport& transport, Encoder& encoder, uint8_t model_id, bool called_by_event = false) {
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

void get_model_info(const std::string& cmd, Transport& transport, Encoder& encoder) {
    ma_err_t ret    = MA_OK;
    auto&    models = static_resource->device->getModels();

    auto it = std::find_if(
      models.begin(), models.end(), [&](const ma_model_t& m) { return m.id == static_resource->current_model_id; });
    if (it == models.end()) {
        ret = MA_ENOENT;
    }

    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    encoder.write("model", it == models.end() ? ma_model_t{} : *it);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
