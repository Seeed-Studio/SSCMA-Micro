#pragma once

#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void kv_create_or_alter(const std::vector<std::string>& argv, void* caller) {
    auto ret =  EL_OK;

    if (argv[1].size() > CONFIG_EL_STORAGE_KEY_SIZE_MAX || argv[2].size() > (FDB_BLOCK_SIZE - CONFIG_EL_STORAGE_KEY_SIZE_MAX)) {
        ret = EL_EINVAL;
    } else {
        auto s = argv[2].length();
        auto v = new char[s + 1]{};

        std::strncpy(v, argv[2].c_str(), s);

        auto kv = el_storage_kv_t<char*>(argv[1].c_str(), v, s + 1);
        ret = static_resource->storage->emplace(kv) ? EL_OK : EL_EIO;

        delete[] v;
    }

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": ",
                           std::to_string(ret),
                           ", \"data\": {\"key\": ",
                           quoted(argv[1].c_str()),
                           ", \"crc16_maxim\": ",
                           std::to_string(el_crc16_maxim(reinterpret_cast<const uint8_t*>(argv[2].c_str()), argv[2].length())),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void kv_query_all_key(const std::vector<std::string>& argv, void* caller) {
    auto        key_list = std::string{};
    const char* delim    = "";

    for (const auto& key : *static_resource->storage) {
        key_list += concat_strings(delim, quoted(key));
        delim = ", ";
    }

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": ",
                           std::to_string(EL_OK),
                           ", \"data\": [",
                           key_list,
                           "]}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void kv_query(const std::vector<std::string>& argv, void* caller) {
    auto   ret = EL_EINVAL;
    size_t s   = 0;
    char*  v   = nullptr;


    if (static_resource->storage->contains(argv[1].c_str())) {
        s = static_resource->storage->get_value_size(argv[1].c_str());
        if (s > 0) [[likely]] {
            v       = new char[s + 1]{};
            auto kv = el_storage_kv_t<char*>(argv[1].c_str(), v, s);            
            ret     = static_resource->storage->get(kv) ? EL_OK : EL_EIO;
        } 
    } 

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": ",
                           std::to_string(ret),
                           ", \"data\": {\"key\": ",
                           quoted(argv[1].c_str()),
                           ", \"value\": ",
                           quoted(v ? v : ""),
                           ", \"crc16_maxim\": ",
                           std::to_string(v ? el_crc16_maxim(reinterpret_cast<const uint8_t*>(v), std::strlen(v)) : el_crc16_maxim(nullptr, 0)),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());

    if (v) delete[] v;
}

void kv_drop(const std::vector<std::string>& argv, void* caller) {
    auto ret = EL_EINVAL;
    if (static_resource->storage->contains(argv[1].c_str()))
        ret = static_resource->storage->erase(argv[1].c_str()) ? EL_OK : EL_EIO;

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": ",
                           std::to_string(ret),
                           ", \"data\": {\"key\": ",
                           quoted(argv[1].c_str()),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
