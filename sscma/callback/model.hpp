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

void set_model(const std::string& cmd, uint8_t model_id) {
    const auto& model_info = static_resource->models->get_model_info(model_id);

    auto ret = model_info.id ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto ModelReply;

    // TODO: allow custom align using marco
    static auto* tensor_arena = el_aligned_malloc_once(32, TENSOR_ARENA_SIZE);
    memset(tensor_arena, 0, TENSOR_ARENA_SIZE);

    ret = static_resource->engine->init(tensor_arena, TENSOR_ARENA_SIZE);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    ret = static_resource->engine->load_model(model_info.addr_memory, model_info.size);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    if (static_resource->current_model_id != model_id) {
        static_resource->current_model_id = model_id;
        if (static_resource->is_ready.load()) [[likely]]
            *static_resource->storage << el_make_storage_kv("current_model_id", static_resource->current_model_id);
    }

    goto ModelReply;

ModelError:
    static_resource->current_model_id = 0;

ModelReply: {
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"model\": ",
                                  model_info_2_json_str(model_info),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}
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
