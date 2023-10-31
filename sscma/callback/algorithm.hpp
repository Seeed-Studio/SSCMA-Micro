#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_algorithms(const std::string& cmd) {
    const auto& registered_algorithms = static_resourse->algorithm_delegate->get_all_algorithm_info();
    const char* delim                 = "";

    std::string ss{
      concat_strings(REPLY_CMD_HEADER, "\"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};
    for (const auto& i : registered_algorithms) {
        ss += concat_strings(delim, algorithm_info_2_json_str(i));
        delim = ", ";
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

    const std::string& ss{concat_strings(REPLY_CMD_HEADER,
                                         "\"name\": \"",
                                         cmd,
                                         "\", \"code\": ",
                                         std::to_string(ret),
                                         ", \"data\": ",
                                         algorithm_info_2_json_str(&algorithm_info),
                                         "}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void get_algorithm_info(const std::string& cmd) {
    const auto& algorithm_info =
      static_resourse->algorithm_delegate->get_algorithm_info(static_resourse->current_algorithm_type);

    const std::string& ss{concat_strings(REPLY_CMD_HEADER,
                                         "\"name\": \"",
                                         cmd,
                                         "\", \"code\": ",
                                         std::to_string(EL_OK),
                                         ", \"data\": ",
                                         algorithm_info_2_json_str(&algorithm_info),
                                         "}\n")};

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
