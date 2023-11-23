#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

#include "core/data/el_data_storage.hpp"
#include "core/el_config_internal.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"

namespace sscma {

using namespace edgelab;
using namespace edgelab::utility;

using namespace sscma::callback;

namespace extension {

class NetworkSupervisor {
   public:
    NetworkSupervisor()
        : _wireless_network_config_revision(0), _mqtt_server_config_revision(0), _mqtt_pubsub_config_revision(0) {
        _target_network_status = get_target_network_status();

        [[maybe_unused]] auto ret = xTaskCreate(&NetworkSupervisor::c_run,
                                                SSCMA_NETWORK_SUPERVISOR_NAME,
                                                SSCMA_NETWORK_SUPERVISOR_STACK_SIZE,
                                                this,
                                                SSCMA_NETWORK_SUPERVISOR_PRIO,
                                                nullptr);
        EL_ASSERT(ret == pdPASS);  // TODO: handle error
    }

    ~NetworkSupervisor() = default;

   protected:
    static void c_run(void* this_pointer) { static_cast<NetworkSupervisor*>(this_pointer)->run(); }

    void run() {
    Loop:
        backward();
        forward();
        el_sleep(SSCMA_NETWORK_SUPERVISOR_POLL_DELAY);
        goto Loop;
    }

    el_net_sta_t get_target_network_status() {
        auto target_status  = static_resource->network->status();
        auto network_config = wireless_network_config_t{};
        if (static_resource->storage->get(el_make_storage_kv_from_type(network_config))) {
            if (std::strlen(network_config.name)) {
                target_status    = NETWORK_JOINED;
                auto mqtt_config = mqtt_server_config_t{};
                if (static_resource->storage->get(el_make_storage_kv_from_type(mqtt_config))) {
                    if (std::strlen(mqtt_config.address)) target_status = NETWORK_CONNECTED;
                } else
                    return target_status;
            } else
                return target_status;
        }
        return target_status;
    }

    el_net_sta_t try_ensure_network_status_changed(el_net_sta_t old_sta, uint32_t delay_ms, std::size_t n_times) {
        for (std::size_t i = 0; i < n_times; ++i) {
            el_sleep(delay_ms);
            auto sta = static_resource->network->status();
            if (sta != old_sta) [[likely]]
                return sta;
        }
        return old_sta;
    }

    void remove_mqtt_pubsub_topics() {
        if (_sub_topics_set.size()) {
            for (const auto& sub_topic : _sub_topics_set) static_resource->network->unsubscribe(sub_topic.c_str());
            _sub_topics_set.clear();  // clear old topics, we're not care about if unsubscribe success
        }
    }

    bool try_reset_mqtt_pubsub_tpoics() {
        auto config = get_default_mqtt_pubsub_config();
        auto kv     = el_make_storage_kv_from_type(config);

        {  // synchronize pubsub config
            const Guard guard(static_resource->network_config_sync_lock);
            static_resource->storage->get(kv);  // discard return error code
            _mqtt_pubsub_config_revision = static_resource->mqtt_pubsub_config_revision;
        }

        remove_mqtt_pubsub_topics();
        auto ret = static_resource->network->subscribe(config.sub_topic, static_cast<mqtt_qos_t>(config.sub_qos));
        if (ret == EL_OK) [[likely]]
            _sub_topics_set.emplace(config.sub_topic);

        static_resource->transport->set_mqtt_pubsub_config(config);

        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@MQTTPUBSUB\", \"code\": ",
                                      std::to_string(ret),
                                      ", \"data\": ",
                                      mqtt_pubsub_config_2_json_str(config),
                                      "}\n")};
        static_resource->transport->send_bytes(ss.c_str(), ss.size());

        return ret == EL_OK;
    }

    void backward() {
        bool wireless_network_config_changed = false;
        bool mqtt_server_config_changed      = false;
        bool mqtt_pubsub_config_changed      = false;

        {
            const Guard guard(static_resource->network_config_sync_lock);
            // check if config changed
            wireless_network_config_changed =
              _wireless_network_config_revision != static_resource->wireless_network_config_revision;
            mqtt_server_config_changed = _mqtt_server_config_revision != static_resource->mqtt_server_config_revision;
            mqtt_pubsub_config_changed = _mqtt_pubsub_config_revision != static_resource->mqtt_pubsub_config_revision;
            // if no config changed, just return
            if (!wireless_network_config_changed && !mqtt_server_config_changed && !mqtt_pubsub_config_changed) return;
            // else update target_network_status
            _target_network_status = get_target_network_status();
        }

        auto current_status = static_resource->network->status();
        switch (current_status) {
        case NETWORK_CONNECTED: {
            // if mqtt_server_config changed, reset mqtt server
            if (mqtt_pubsub_config_changed) {
                if (!try_reset_mqtt_pubsub_tpoics()) [[unlikely]]
                    return;  // if reset failed, just return
            }

            // if mqtt_server_config changed, reset mqtt server
            if (mqtt_server_config_changed) {
                // mqtt pubsub config should be reloaded later
                remove_mqtt_pubsub_topics();
                // disconnect from mqtt server and reset pubsub config
                auto ret = static_resource->network->disconnect();
                if (ret != EL_OK) [[unlikely]]
                    return;
                if (try_ensure_network_status_changed(
                      current_status, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS, SSCMA_WIRELESS_NETWORK_POLL_RETRY) !=
                    NETWORK_CONNECTED) [[unlikely]]
                    return;
            }
        }
            [[fallthrough]];  // else fallthrough to NETWORK_JOINED

        case NETWORK_JOINED: {
            // if wireless_network_config changed, reset wireless network
            if (wireless_network_config_changed) {
                auto ret = static_resource->network->quit();
                if (ret != EL_OK) [[unlikely]]
                    return;
                if (try_ensure_network_status_changed(current_status,
                                                      SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS,
                                                      SSCMA_WIRELESS_NETWORK_POLL_RETRY) != NETWORK_JOINED) [[unlikely]]
                    return;
            }
        }
            [[fallthrough]];  // else fallthrough to NETWORK_IDLE

        case NETWORK_IDLE:
        case NETWORK_LOST:
        default:
            return;
        }
    }

    void forward() {
        if (_target_network_status != NETWORK_JOINED && _target_network_status != NETWORK_CONNECTED) return;

        auto current_status = static_resource->network->status();
        if (current_status == _target_network_status) return;

        switch (current_status) {
        case NETWORK_LOST: {
            static_resource->network->init();
            printf("network init\n");
        }
            [[fallthrough]];  // else fallthrough to NETWORK_IDLE

        case NETWORK_IDLE: {
            auto config = wireless_network_config_t{};
            auto kv     = el_make_storage_kv_from_type(config);

            {  // synchronize wireless network config
                const Guard guard(static_resource->network_config_sync_lock);
                if (!static_resource->storage->get(kv)) [[unlikely]]
                    return;
                _wireless_network_config_revision = static_resource->wireless_network_config_revision;
            }

            auto ret = static_resource->network->join(config.name, config.passwd);

            const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@WIFI\", \"code\": ",
                                          std::to_string(ret),
                                          ", \"data\": ",
                                          wireless_network_config_2_json_str(config),
                                          "}\n")};
            static_resource->transport->send_bytes(ss.c_str(), ss.size());

            if (ret != EL_OK) [[unlikely]]
                return;
            if (try_ensure_network_status_changed(current_status,
                                                  SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS,
                                                  SSCMA_WIRELESS_NETWORK_POLL_RETRY) != NETWORK_JOINED) [[unlikely]]
                return;
        }
            [[fallthrough]];  // else fallthrough to NETWORK_JOINED

        case NETWORK_JOINED: {
            auto config = mqtt_server_config_t{};
            auto kv     = el_make_storage_kv_from_type(config);

            {
                const Guard guard(static_resource->network_config_sync_lock);
                if (!static_resource->storage->get(kv)) [[unlikely]]
                    return;
                _mqtt_server_config_revision = static_resource->mqtt_server_config_revision;
            }

            auto ret =
              static_resource->network->connect(config.address, config.username, config.password, mqtt_recv_cb);

            const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@MQTTSERVER\", \"code\": ",
                                          std::to_string(ret),
                                          ", \"data\": ",
                                          mqtt_server_config_2_json_str(config),
                                          "}\n")};
            static_resource->transport->send_bytes(ss.c_str(), ss.size());

            if (ret != EL_OK) [[unlikely]]
                return;
            if (try_ensure_network_status_changed(current_status, SSCMA_MQTT_POLL_DELAY_MS, SSCMA_MQTT_POLL_RETRY) !=
                NETWORK_CONNECTED) [[unlikely]]
                return;
        }
            [[fallthrough]];  // else fallthrough to NETWORK_CONNECTED

        case NETWORK_CONNECTED: {
            if (!try_reset_mqtt_pubsub_tpoics()) [[unlikely]]
                return;
        }
            [[fallthrough]];  // else fallthrough to default

        default:
            static_resource->transport->emit_mqtt_discover();  // emit mqtt discover
        }
    }

   private:
    el_net_sta_t _target_network_status;

    std::size_t _wireless_network_config_revision;
    std::size_t _mqtt_server_config_revision;
    std::size_t _mqtt_pubsub_config_revision;

    std::unordered_set<std::string> _sub_topics_set;
};

}  // namespace extension

}  // namespace sscma
