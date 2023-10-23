#pragma once

#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_models(const std::string& cmd) {
    const auto& models_info = static_resourse->models->get_all_model_info();

    std::string ss{
      concat_strings(REPLY_CMD_HEADER, "\"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};
    DELIM_RESET;
    for (const auto& i : models_info) {
        DELIM_PRINT(ss);
        ss += model_info_2_json_str(i);
    }
    ss += "]}\n";

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void set_model(const std::string& cmd, uint8_t model_id) {
    const auto& model_info = static_resourse->models->get_model_info(model_id);

    auto ret = model_info.id ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto ModelReply;

    // TODO: move heap_caps_malloc to port/el_memory or el_system
    static auto* tensor_arena = el_malloc(TENSOR_ARENA_SIZE);
    memset(tensor_arena, 0, TENSOR_ARENA_SIZE);

    ret = static_resourse->engine->init(tensor_arena, TENSOR_ARENA_SIZE);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    ret = static_resourse->engine->load_model(model_info.addr_memory, model_info.size);
    if (ret != EL_OK) [[unlikely]]
        goto ModelError;

    if (static_resourse->current_model_id != model_id) {
        static_resourse->current_model_id = model_id;
        if (static_resourse->is_ready.load()) [[likely]]
            *static_resourse->storage << el_make_storage_kv("current_model_id", static_resourse->current_model_id);
    }

    goto ModelReply;

ModelError:
    static_resourse->current_model_id = 0;

ModelReply: {
    const auto& ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"model\": ",
                                  model_info_2_json_str(model_info),
                                  "}}\n")};
    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}
}

void get_model_info(const std::string& cmd) {
    const auto& model_info = static_resourse->models->get_model_info(static_resourse->current_model_id);

    auto ret = model_info.id ? EL_OK : EL_EINVAL;

    const auto& ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  model_info_2_json_str(model_info),
                                  "}\n")};
    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
