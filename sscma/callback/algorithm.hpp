#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "sscma/callback/internal/algorithm_config_helper.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_algorithms(const std::string& cmd) {
    const auto& registered_algorithms = static_resourse->algorithm_delegate->get_all_algorithm_info();

    std::string ss{
      concat_strings(REPLY_CMD_HEADER, "\"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};
    DELIM_RESET;
    for (const auto& i : registered_algorithms) {
        DELIM_PRINT(ss);
        ss += algorithm_info_2_json_str(i);
    }
    ss += "]}\n";

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void set_algorithm(const std::string& cmd, el_algorithm_type_t algorithm_type) {
    const auto& algorithm_info = static_resourse->algorithm_delegate->get_algorithm_info(algorithm_type);
    auto        ret            = algorithm_type == algorithm_info.type ? EL_OK : EL_EINVAL;

    if (algorithm_info.type != static_resourse->current_algorithm_type) [[likely]] {
        static_resourse->current_algorithm_type = algorithm_info.type;
        if (static_resourse->is_ready.load()) [[likely]]
            *static_resourse->storage << el_make_storage_kv("current_algorithm_type",
                                                            static_resourse->current_algorithm_type);
    }

    std::string ss{
      concat_strings(REPLY_CMD_HEADER, "\"name\": \"", cmd, "\", \"code\": ", std::to_string(ret), ", \"data\": ")};
    switch (algorithm_info.type) {
    case EL_ALGO_TYPE_FOMO: {
        el_algorithm_fomo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    case EL_ALGO_TYPE_IMCLS: {
        el_algorithm_imcls_config_t info_and_conf{};
        auto                        kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    case EL_ALGO_TYPE_PFLD: {
        el_algorithm_pfld_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    case EL_ALGO_TYPE_YOLO: {
        el_algorithm_yolo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    default:
        ss += concat_strings("{\"type\": ",
                             std::to_string(algorithm_info.type),
                             ", \"categroy\": ",
                             std::to_string(algorithm_info.categroy),
                             ", \"input_from\": ",
                             std::to_string(algorithm_info.input_from),
                             ", \"config\": {}}");
    }
    ss += "}\n";

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_algorithm_info(const std::string& cmd) {
    const auto& algorithm_info =
      static_resourse->algorithm_delegate->get_algorithm_info(static_resourse->current_algorithm_type);

    std::string ss{
      concat_strings(REPLY_CMD_HEADER, "\"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": ")};

    switch (algorithm_info.type) {
    case EL_ALGO_TYPE_FOMO: {
        el_algorithm_fomo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    case EL_ALGO_TYPE_IMCLS: {
        el_algorithm_imcls_config_t info_and_conf{};
        auto                        kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    case EL_ALGO_TYPE_PFLD: {
        el_algorithm_pfld_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    case EL_ALGO_TYPE_YOLO: {
        el_algorithm_yolo_config_t info_and_conf{};
        auto                       kv{el_make_storage_kv_from_type(info_and_conf)};
        if (static_resourse->storage->contains(kv.key))
            *static_resourse->storage >> el_make_storage_kv_from_type(info_and_conf);
        ss += algorithm_info_and_conf_2_json_str(info_and_conf);
    } break;

    default:
        ss += concat_strings("{\"type\": ",
                             std::to_string(algorithm_info.type),
                             ", \"categroy\": ",
                             std::to_string(algorithm_info.categroy),
                             ", \"input_from\": ",
                             std::to_string(algorithm_info.input_from),
                             ", \"config\": {}}");
    }
    ss += "}\n";

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
