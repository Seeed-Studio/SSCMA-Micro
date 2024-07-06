#pragma once

#include <atomic>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"

#include "resource.hpp"

namespace ma::server::callback {

using namespace ma;

void get_device_id(const std::string& cmd, Transport& transport, Codec& codec) {
    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd, static_resource->device->id());
    codec.end();
    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
}

void get_device_name(const std::string& cmd, Transport& transport, Codec& codec) {
    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd, static_resource->device->name());
    codec.end();
    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
}

void get_device_status(const std::string& cmd, Transport& transport, Codec& codec) {
    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd);
    codec.write("boot_count", static_resource->boot_count);
    codec.write("is_ready", static_resource->is_ready.load() ? 1 : 0);
    codec.end();
    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
}

void get_version(const std::string& cmd,
                 Transport& transport,
                 Codec& codec,
                 const std::string& version) {
    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd);
    codec.write("at_api", version);
    codec.write("software", MA_VERSION);
    codec.write("hardware", static_resource->device->version());
    codec.end();
    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
}

void break_task(const std::string& cmd, Transport& transport, Codec& codec) {
    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd, static_cast<uint64_t>(ma_get_time_ms()));
    codec.end();
    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
}

void task_status(const std::string& cmd, bool sta, Transport& transport, Codec& codec) {
    codec.begin(MA_REPLY_RESPONSE, MA_OK, cmd, static_cast<uint64_t>(sta));
    codec.end();
    transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
}

}  // namespace ma::server::callback
