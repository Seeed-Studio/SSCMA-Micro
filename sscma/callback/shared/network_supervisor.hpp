#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_set>

#include "core/data/el_data_storage.hpp"
#include "core/el_config_internal.h"
#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "porting/el_device.h"
#include "porting/el_network.h"
#include "porting/el_transport.h"
#include "sscma/definations.hpp"
#include "sscma/extension/mux_transport.hpp"
#include "sscma/static_resource.hpp"

namespace sscma {

using namespace edgelab;
using namespace edgelab::utility;

namespace extension {

void mqtt_recv_cb(char* top, int tlen, char* msg, int mlen) {
    auto config = static_resource->transport->get_mqtt_pubsub_config();
    if (tlen ^ std::strlen(config.sub_topic) || std::strncmp(top, config.sub_topic, tlen) || mlen <= 1) return;
    static_resource->instance->exec(std::string(msg, mlen));
}

class NetworkSupervisor {
   public:
    ~NetworkSupervisor() {
        _device    = nullptr;
        _network   = nullptr;
        _transport = nullptr;
        _storage   = nullptr;
    }

    [[nodiscard]] static NetworkSupervisor* get_network_supervisor() {
        static NetworkSupervisor network_supervisor;
        return &network_supervisor;
    }

    void start() {  // call once only
        [[maybe_unused]] auto ret = xTaskCreate(&NetworkSupervisor::c_run,
                                                SSCMA_NETWORK_SUPERVISOR_NAME,
                                                SSCMA_NETWORK_SUPERVISOR_STACK_SIZE,
                                                this,
                                                SSCMA_NETWORK_SUPERVISOR_PRIO,
                                                nullptr);
        EL_ASSERT(ret == pdPASS);  // TODO: handle error
    }

    void update_wireless_network_config(const wireless_network_config_t& config) {
        const Guard guard(_network_config_sync_lock);
        _wireless_network_config         = config;
        _wireless_network_config_updated = false;
        sync_target_network_status();
    }

    void update_mqtt_server_config(const mqtt_server_config_t& config) {
        const Guard guard(_network_config_sync_lock);
        _mqtt_server_config         = config;
        _mqtt_server_config_updated = false;
        sync_target_network_status();
    }

    void update_mqtt_pubsub_config(const mqtt_pubsub_config_t& config) {
        const Guard guard(_network_config_sync_lock);
        _mqtt_pubsub_config         = config;
        _mqtt_pubsub_config_updated = false;
        sync_target_network_status();
    }

    bool is_wireless_network_config_updated() {
        const Guard guard(_network_config_sync_lock);
        return _wireless_network_config_updated;
    }

    bool is_mqtt_server_config_updated() {
        const Guard guard(_network_config_sync_lock);
        return _mqtt_server_config_updated;
    }

    bool is_mqtt_pubsub_config_updated() {
        const Guard guard(_network_config_sync_lock);
        return _mqtt_pubsub_config_updated;
    }

   protected:
    NetworkSupervisor()
        : _device(static_resource->device),
          _network(static_resource->device->get_network()),
          _transport(static_resource->transport),
          _storage(static_resource->storage),
          _network_config_sync_lock(),
          _wireless_network_config(),
          _mqtt_server_config(get_default_mqtt_server_config(_device)),
          _mqtt_pubsub_config(get_default_mqtt_pubsub_config(_device)),
          _wireless_network_config_updated(true),
          _mqtt_server_config_updated(true),
          _mqtt_pubsub_config_updated(true),
          _target_network_status(NETWORK_LOST),
          _sub_topics_set() {}

    void run() {
        load_network_config();
        sync_target_network_status();

    Loop:
        status_backward();
        status_forward();
        el_sleep(SSCMA_NETWORK_SUPERVISOR_POLL_DELAY);
        goto Loop;
    }

    static void c_run(void* this_pointer) { static_cast<NetworkSupervisor*>(this_pointer)->run(); }

    void load_network_config() {
        static_resource->storage->get(el_make_storage_kv_from_type(_wireless_network_config));
        static_resource->storage->get(el_make_storage_kv_from_type(_mqtt_server_config));
        static_resource->storage->get(el_make_storage_kv_from_type(_mqtt_pubsub_config));
    }

    void sync_target_network_status() {
        if (std::strlen(_wireless_network_config.name)) {
            _target_network_status = NETWORK_JOINED;
            if (std::strlen(_mqtt_server_config.address)) _target_network_status = NETWORK_CONNECTED;
        } else
            _target_network_status = NETWORK_LOST;  // or NETWORK_IDLE
    }

    el_net_sta_t try_ensure_network_status_changed(el_net_sta_t old_sta, uint32_t delay_ms, std::size_t n_times) {
        for (std::size_t i = 0; i < n_times; ++i) {
            el_sleep(delay_ms);
            auto sta = _network->status();
            if (sta != old_sta) [[likely]]
                return sta;
        }
        return old_sta;
    }

    void remove_mqtt_pubsub_topics() {
        if (_sub_topics_set.size()) {
            for (const auto& sub_topic : _sub_topics_set) _network->unsubscribe(sub_topic.c_str());
            _sub_topics_set.clear();  // clear old topics, we're not care about if unsubscribe success
        }
    }

    bool try_sync_mqtt_pubsub_tpoics() {
        auto config = mqtt_pubsub_config_t{};
        {  // synchronize pubsub config
            const Guard guard(_network_config_sync_lock);
            config                      = _mqtt_pubsub_config;
            _mqtt_pubsub_config_updated = true;
        }

        remove_mqtt_pubsub_topics();
        auto ret = _network->subscribe(config.sub_topic, static_cast<mqtt_qos_t>(config.sub_qos));
        if (ret == EL_OK) [[likely]]
            _sub_topics_set.emplace(config.sub_topic);
        _transport->set_mqtt_pubsub_config(config);

        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@MQTTPUBSUB\", \"code\": ",
                                      std::to_string(ret),
                                      ", \"data\": ",
                                      mqtt_pubsub_config_2_json_str(config),
                                      "}\n")};
        _transport->send_bytes(ss.c_str(), ss.size());

        return ret == EL_OK;
    }

    void status_backward() {
        auto current_status = _network->status();
        // early return if current status is NETWORK_LOST
        if (current_status == NETWORK_LOST) [[unlikely]]
            return;
        auto target_status                   = current_status;
        bool wireless_network_config_changed = false;
        bool mqtt_server_config_changed      = false;
        bool mqtt_pubsub_config_changed      = false;
        {
            const Guard guard(_network_config_sync_lock);
            // check if config need to be updated
            if (_wireless_network_config_updated & _mqtt_server_config_updated & _mqtt_pubsub_config_updated) [[likely]]
                return;
            // sync local config status
            target_status                   = _target_network_status;
            wireless_network_config_changed = !_wireless_network_config_updated;
            mqtt_server_config_changed      = !_mqtt_server_config_updated;
            mqtt_pubsub_config_changed      = !_mqtt_pubsub_config_updated;
        }

        EL_LOGI("[NetworkSupervisor] config change observed, target status: %d, current status: %d",
                target_status,
                current_status);

        switch (current_status) {
        case NETWORK_CONNECTED: {
            // if mqtt_server_config changed, reset mqtt server
            if (mqtt_pubsub_config_changed) [[unlikely]] {
                if (!try_sync_mqtt_pubsub_tpoics()) [[unlikely]]
                    return;  // if failed to sync mqtt pubsub config, just return
            }

            // if mqtt_server_config changed, reset mqtt server
            if (mqtt_server_config_changed) [[unlikely]] {
                // mqtt pubsub config should be reloaded later
                remove_mqtt_pubsub_topics();
                // disconnect from mqtt server and reset pubsub config
                auto ret = _network->disconnect();
                if (ret == EL_OK) [[likely]]
                    ret = try_ensure_network_status_changed(
                            NETWORK_CONNECTED, SSCMA_MQTT_POLL_DELAY_MS, SSCMA_MQTT_POLL_RETRY) == NETWORK_JOINED
                            ? EL_OK
                            : EL_ETIMOUT;

                EL_LOGI("[NetworkSupervisor] mqtt server disconnected, ret: %d", ret);
                if (ret != EL_OK) [[unlikely]]
                    return;

                const Guard guard(_network_config_sync_lock);
                _mqtt_server_config_updated = true;
            }
        }
            [[fallthrough]];  // else fallthrough to NETWORK_JOINED

        case NETWORK_JOINED: {
            // if wireless_network_config changed, reset wireless network
            if (wireless_network_config_changed) [[unlikely]] {
                auto ret = _network->quit();
                if (ret == EL_OK) [[likely]]
                    ret = try_ensure_network_status_changed(NETWORK_JOINED,
                                                            SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS,
                                                            SSCMA_WIRELESS_NETWORK_POLL_RETRY) == NETWORK_IDLE
                            ? EL_OK
                            : EL_ETIMOUT;

                EL_LOGI("[NetworkSupervisor] wireless network quit, ret: %d", ret);
                if (ret != EL_OK) [[unlikely]]
                    return;

                const Guard guard(_network_config_sync_lock);
                _wireless_network_config_updated = true;
            }
        }
            [[fallthrough]];  // else fallthrough to NETWORK_IDLE

        case NETWORK_IDLE: {
            // when fall to NETWORK_IDLE, if target status is NETWORK_LOST, deinit network
            if (target_status == NETWORK_LOST) [[unlikely]] {
                _network->deinit();
                if (!(try_ensure_network_status_changed(NETWORK_IDLE,
                                                        SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS,
                                                        SSCMA_WIRELESS_NETWORK_POLL_RETRY) == NETWORK_LOST)) [[likely]]
                    return;
            }
        }
            [[fallthrough]];  // else fallthrough to NETWORK_LOST

        case NETWORK_LOST:
        default:
            return;
        }
    }

    void status_forward() {
        auto current_status = _network->status();
        auto target_status  = current_status;
        {
            const Guard guard(_network_config_sync_lock);
            target_status = _target_network_status;
            // if target status is not (NETWORK_JOINED or NETWORK_CONNECTED), just return
            if (target_status != NETWORK_JOINED && target_status != NETWORK_CONNECTED) return;
            // if target status is same as current status, just return
            if (current_status == target_status) [[unlikely]]
                return;
        }

        // target status is (NETWORK_JOINED or NETWORK_CONNECTED)
        // current status != target status
        switch (current_status) {
        case NETWORK_LOST: {
            _network->init();
            if (!(try_ensure_network_status_changed(NETWORK_LOST,
                                                    SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS,
                                                    SSCMA_WIRELESS_NETWORK_POLL_RETRY) == NETWORK_IDLE)) [[unlikely]]
                return;
            // skip verify if target_status is NETWORK_IDLE
        }
            [[fallthrough]];  // else fallthrough to NETWORK_IDLE

        case NETWORK_IDLE: {
            auto config = wireless_network_config_t{};
            {  // synchronize wireless network config
                const Guard guard(_network_config_sync_lock);
                config                           = _wireless_network_config;
                _wireless_network_config_updated = true;
            }

            auto ret = _network->join(config.name, config.passwd);
            if (ret == EL_OK) [[likely]]
                ret = try_ensure_network_status_changed(NETWORK_IDLE,
                                                        SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS,
                                                        SSCMA_WIRELESS_NETWORK_POLL_RETRY) == NETWORK_JOINED
                        ? EL_OK
                        : EL_ETIMOUT;

            const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@WIFI\", \"code\": ",
                                          std::to_string(ret),
                                          ", \"data\": ",
                                          wireless_network_config_2_json_str(config),
                                          "}\n")};
            _transport->send_bytes(ss.c_str(), ss.size());

            if (ret != EL_OK) [[unlikely]]
                return;
            // skip verify if target_status is NETWORK_JOINED
        }
            [[fallthrough]];  // else fallthrough to NETWORK_JOINED

        case NETWORK_JOINED: {
            auto config = mqtt_server_config_t{};
            {
                const Guard guard(_network_config_sync_lock);
                config                      = _mqtt_server_config;
                _mqtt_server_config_updated = true;
            }

            auto ret = _network->connect(config.address, config.username, config.password, mqtt_recv_cb);
            if (ret == EL_OK) [[likely]]
                ret = try_ensure_network_status_changed(
                        NETWORK_JOINED, SSCMA_MQTT_POLL_DELAY_MS, SSCMA_MQTT_POLL_RETRY) == NETWORK_CONNECTED
                        ? EL_OK
                        : EL_ETIMOUT;

            const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@MQTTSERVER\", \"code\": ",
                                          std::to_string(ret),
                                          ", \"data\": ",
                                          mqtt_server_config_2_json_str(config),
                                          "}\n")};
            _transport->send_bytes(ss.c_str(), ss.size());

            // if failed or target status is NETWORK_JOINED, skip fallthrough
            if ((ret != EL_OK) | (target_status == NETWORK_JOINED)) return;
        }
            [[fallthrough]];  // else fallthrough to NETWORK_CONNECTED

        case NETWORK_CONNECTED: {
            // if failed or target status is NETWORK_CONNECTED, skip fallthrough
            if ((!try_sync_mqtt_pubsub_tpoics()) | (target_status == NETWORK_CONNECTED)) return;
        }
            [[fallthrough]];  // else fallthrough to default

        default:
            _transport->emit_mqtt_discover();  // emit mqtt discover
        }
    }

   private:
    Device*       _device;
    Network*      _network;
    MuxTransport* _transport;
    Storage*      _storage;

    Mutex _network_config_sync_lock;

    wireless_network_config_t _wireless_network_config;
    mqtt_server_config_t      _mqtt_server_config;
    mqtt_pubsub_config_t      _mqtt_pubsub_config;

    bool _wireless_network_config_updated;
    bool _mqtt_server_config_updated;
    bool _mqtt_pubsub_config_updated;

    el_net_sta_t _target_network_status;

    std::unordered_set<std::string> _sub_topics_set;
};

}  // namespace extension

}  // namespace sscma
