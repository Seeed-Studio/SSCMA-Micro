#pragma once

#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_models(const std::string& cmd) {
    const auto& models_info = static_resource->models->get_all_model_info();
    const char* delim       = "";

    std::string ss{
      concat_strings("\r{\"type\": 0, \"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};
    for (const auto& i : models_info) {
        ss += concat_strings(delim, model_info_2_json_str(i));
        delim = ", ";
    }
    ss += "]}\n";

    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void set_model(const std::string& cmd, uint8_t model_id, bool called_by_event = false) {
    const auto& model_info = static_resource->models->get_model_info(model_id);

    // a valid model id should always > 0
    auto ret = model_info.id ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto ModelReply;

    // allocate tensor arena once (memset to 0 every time)
    static auto* tensor_arena = el_aligned_malloc_once(32, CONFIG_SSCMA_TENSOR_ARENA_SIZE);
    std::memset(tensor_arena, 0, CONFIG_SSCMA_TENSOR_ARENA_SIZE);

    // init engine with tensor arena
    ret = static_resource->engine->init(tensor_arena, CONFIG_SSCMA_TENSOR_ARENA_SIZE);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    // load model from flash to tensor arena (memory)
    ret = static_resource->engine->load_model(model_info.addr_memory, model_info.size);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    // if model id changed, update current model id
    if (static_resource->current_model_id != model_id) {
        static_resource->current_model_id = model_id;
        if (!called_by_event)
            ret = static_resource->storage->emplace(
                    el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_MODEL_ID, static_resource->current_model_id))
                    ? EL_OK
                    : EL_EIO;
    }

    // jump since everything is ok
    goto ModelReply;

ModelError:
    static_resource->current_model_id = 0;

ModelReply:
    const auto& ss{concat_strings("\r{\"type\": ",
                                  std::to_string(called_by_event ? 1 : 0),
                                  ", \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"model\": ",
                                  model_info_2_json_str(model_info),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_model_info(const std::string& cmd) {
    const auto& model_info = static_resource->models->get_model_info(static_resource->current_model_id);

    auto ret = model_info.id ? EL_OK : EL_EINVAL;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  model_info_2_json_str(model_info),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
