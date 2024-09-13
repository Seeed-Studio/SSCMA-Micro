#pragma once

#include <atomic>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma;

void get_device_id(const std::string& cmd, Transport& transport, Encoder& encoder) {
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd, static_resource->device->id());
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void get_device_name(const std::string& cmd, Transport& transport, Encoder& encoder) {
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd, static_resource->device->name());
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void get_device_status(const std::string& cmd, Transport& transport, Encoder& encoder) {
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write("boot_count", static_cast<int32_t>(static_resource->device->bootCount()));
    encoder.write("is_ready", static_cast<int32_t>(static_resource->is_ready ? 1 : 0));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void get_version(const std::string& cmd, Transport& transport, Encoder& encoder, const std::string& version) {
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write("at_api", version);
    encoder.write("software", MA_VERSION);
    encoder.write("hardware", static_resource->device->version());
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void break_task(const std::string& cmd, Transport& transport, Encoder& encoder) {
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd, static_cast<uint64_t>(ma_get_time_ms()));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void task_status(const std::string& cmd, bool sta, Transport& transport, Encoder& encoder) {
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd, static_cast<uint64_t>(sta));
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
