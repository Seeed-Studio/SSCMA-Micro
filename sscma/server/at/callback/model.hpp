#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"

#include "resource.hpp"

namespace ma::server::callback {

using namespace ma::model;

void get_available_models(const std::string& cmd, Transport& transport, Encoder& encoder) {
    auto& models = Engine::getModels();
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write(models);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}
void set_model(const std::string& cmd,
               Transport& transport,
               Encoder& encoder,
               uint8_t model_id,
               bool called_by_event = false) {

    ma_err_t ret = MA_OK;
    auto& models = Engine::getModels();

    if (model_id >= models.size()) {
        ret = MA_EINVAL;
        goto exit;
    }
    // init engine with tensor arena
    ret = static_resource->engine->init();
    if (ret != MA_OK) [[unlikely]]
        goto exit;

    // load model from flash to tensor arena (memory)
    ret = static_resource->engine->load(models[model_id + 1].addr);
    if (ret != MA_OK) [[unlikely]]
        goto exit;

    // if model id changed, update current model id
    if (static_resource->cur_model_id != model_id + 1) {
        static_resource->cur_model_id = model_id + 1;
        if (!called_by_event)
            ret = static_resource->storage->set(MA_STORAGE_KEY_MODEL_ID,
                                                static_resource->cur_model_id);
    }

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    if (static_resource->cur_model_id) {
        encoder.write("model", models[static_resource->cur_model_id]);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void get_model_info(const std::string& cmd, Transport& transport, Encoder& encoder) {

    auto& models = Engine::getModels();

    encoder.begin(MA_MSG_TYPE_RESP, static_resource->cur_model_id ? MA_OK : MA_ENOENT, cmd);
    if (static_resource->cur_model_id) {
        encoder.write("model", models[static_resource->cur_model_id]);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
