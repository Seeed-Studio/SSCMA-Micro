#pragma once

#include <cstdint>
#include <cstring>

#include "core/el_types.h"
#include "porting/el_misc.h"
#include "porting/el_network.h"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"

namespace sscma::utility {

decltype(auto) get_default_mqtt_pubsub_config() {
    auto default_config = mqtt_pubsub_config_t{};

    std::snprintf(default_config.pub_topic,
                  sizeof(default_config.pub_topic) - 1,
                  SSCMA_MQTT_PUB_FMT,
                  SSCMA_AT_API_MAJOR_VERSION,
                  VENDOR_PREFIX,
                  VENDOR_CHIP_NAME,
                  static_resource->device->get_device_id());
    default_config.pub_qos = 0;

    std::snprintf(default_config.sub_topic,
                  sizeof(default_config.sub_topic) - 1,
                  SSCMA_MQTT_SUB_FMT,
                  SSCMA_AT_API_MAJOR_VERSION,
                  VENDOR_PREFIX,
                  VENDOR_CHIP_NAME,
                  static_resource->device->get_device_id());
    default_config.sub_qos = 0;

    return default_config;
}

decltype(auto) get_default_mqtt_server_config() {
    auto default_config = mqtt_server_config_t{};

    std::snprintf(default_config.client_id,
                  sizeof(default_config.client_id) - 1,
                  SSCMA_MQTT_DEVICE_ID_FMT,
                  VENDOR_PREFIX,
                  VENDOR_CHIP_NAME,
                  static_resource->device->get_device_id());

    return default_config;
}

}  // namespace sscma::utility
