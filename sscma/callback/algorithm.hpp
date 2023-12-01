#pragma once

#include <cstdint>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_algorithms(const std::string& cmd, void* caller) {
    const auto& registered_algorithms = static_resource->algorithm_delegate->get_all_algorithm_info();
    const char* delim                 = "";

    std::string ss{
      concat_strings("\r{\"type\": 0, \"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};
    for (const auto& i : registered_algorithms) {
        ss += concat_strings(delim, algorithm_info_2_json_str(i));
        delim = ", ";
    }
    ss += "]}\n";

    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_algorithm(const std::string&  cmd,
                   el_algorithm_type_t algorithm_type,
                   void*               caller,
                   bool                called_by_event = false) {
    const auto& algorithm_info = static_resource->algorithm_delegate->get_algorithm_info(algorithm_type);
    auto        ret            = algorithm_type == algorithm_info.type ? EL_OK : EL_EINVAL;

    // if algorithm type changed, update current algorithm type
    if (algorithm_info.type != static_resource->current_algorithm_type) [[likely]] {
        static_resource->current_algorithm_type = algorithm_info.type;

        if (!called_by_event)
            ret = static_resource->storage->emplace(el_make_storage_kv_from_type(algorithm_info.type)) ? EL_OK : EL_EIO;
    }

    const std::string& ss{concat_strings("\r{\"type\": ",
                                         called_by_event ? "1" : "0",
                                         ", \"name\": \"",
                                         cmd,
                                         "\", \"code\": ",
                                         std::to_string(ret),
                                         ", \"data\": ",
                                         algorithm_info_2_json_str(&algorithm_info),
                                         "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_algorithm_info(const std::string& cmd, void* caller) {
    const auto& algorithm_info =
      static_resource->algorithm_delegate->get_algorithm_info(static_resource->current_algorithm_type);

    const std::string& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                         cmd,
                                         "\", \"code\": ",
                                         std::to_string(EL_OK),
                                         ", \"data\": ",
                                         algorithm_info_2_json_str(&algorithm_info),
                                         "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
