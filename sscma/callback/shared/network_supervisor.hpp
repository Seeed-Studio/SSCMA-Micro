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
#include "sscma/extension/mqtt_transport.hpp"
#include "sscma/static_resource.hpp"

namespace sscma {

using namespace edgelab;
using namespace edgelab::utility;

namespace extension {

void mqtt_recv_cb(char* top, int tlen, char* msg, int mlen) {
    auto config = static_resource->mqtt->get_mqtt_pubsub_config();
    if (tlen ^ std::strlen(config.sub_topic) || std::strncmp(top, config.sub_topic, tlen) || mlen <= 1) return;
    if (!static_resource->mqtt->push_to_buffer(msg, mlen)) [[unlikely]]
        EL_LOGI("MQTT buffer may corrupted");
}

class NetworkSupervisor {
   public:
    ~NetworkSupervisor() {
        _network = nullptr;
        _mqtt    = nullptr;
        _storage = nullptr;
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
        : _network(static_resource->network),
          _mqtt(static_resource->mqtt),
          _storage(static_resource->storage),
          _network_config_sync_lock(),
          _wireless_network_config(),
          _mqtt_server_config(get_default_mqtt_server_config(static_resource->device)),
          _mqtt_pubsub_config(get_default_mqtt_pubsub_config(static_resource->device)),
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

    void ensure_network_status_changed_from(el_net_sta_t old_sta, uint32_t delay_ms) {
        auto sta = _network->status();
        for (; sta == old_sta;) {
            el_sleep(delay_ms);
            sta = _network->status();
        }
    }

    bool try_ensure_network_status_changed_to(el_net_sta_t new_sta, uint32_t delay_ms, std::size_t n_times) {
        auto sta = _network->status();
        for (std::size_t i = 0; (i < n_times) & (sta != new_sta); ++i) {
            el_sleep(delay_ms);
            sta = _network->status();
        }
        return sta == new_sta;
    }

    void remove_mqtt_pubsub_topics() {
        if (_sub_topics_set.size()) {
            for (const auto& sub_topic : _sub_topics_set) _network->unsubscribe(sub_topic.c_str());
            _sub_topics_set.clear();  // clear old topics, we're not care about if unsubscribe success
        }
    }

    bool try_sync_mqtt_pubsub_topics() {
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
        _mqtt->set_mqtt_pubsub_config(config);
        _mqtt->emit_mqtt_discover();  // emit mqtt discover

        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@MQTTPUBSUB\", \"code\": ",
                                      std::to_string(ret),
                                      ", \"data\": ",
                                      mqtt_pubsub_config_2_json_str(config),
                                      "}\n")};
        _mqtt->send_bytes(ss.c_str(), ss.size());
        static_resource->serial->send_bytes(ss.c_str(), ss.size());

        return ret == EL_OK;
    }

    void disconnect_mqtt_server(el_net_sta_t old_sta) {
        remove_mqtt_pubsub_topics();
        auto ret = _network->disconnect();
        if (ret == EL_OK) [[likely]]
            ensure_network_status_changed_from(old_sta, SSCMA_MQTT_POLL_DELAY_MS);
    }

    void quit_wireless_network(el_net_sta_t old_sta) {
        auto ret = _network->quit();
        if (ret == EL_OK) [[likely]]
            ensure_network_status_changed_from(old_sta, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS);
    }

    void deinit_network(el_net_sta_t old_sta) {
        _network->deinit();
        ensure_network_status_changed_from(old_sta, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS);
    }

    void status_backward() {
        bool wireless_network_config_changed = false;
        bool mqtt_server_config_changed      = false;
        bool mqtt_pubsub_config_changed      = false;
        {
            const Guard guard(_network_config_sync_lock);
            wireless_network_config_changed = !_wireless_network_config_updated;
            mqtt_server_config_changed      = !_mqtt_server_config_updated;
            mqtt_pubsub_config_changed      = !_mqtt_pubsub_config_updated;
        }

    ResetNetworkStatus:
        auto current_status = _network->status();
        switch (current_status) {
        case NETWORK_CONNECTED:
            if (mqtt_pubsub_config_changed | mqtt_server_config_changed | wireless_network_config_changed)
              [[unlikely]] {
                disconnect_mqtt_server(current_status);
                goto ResetNetworkStatus;
            }
            break;

        case NETWORK_JOINED:
            if (wireless_network_config_changed) [[unlikely]] {
                quit_wireless_network(current_status);
                goto ResetNetworkStatus;
            }
            break;

        case NETWORK_IDLE:
            if (wireless_network_config_changed) [[unlikely]] {
                deinit_network(current_status);
                goto ResetNetworkStatus;
            }
            break;

        case NETWORK_LOST:
        default:
            return;
        }
    }

    bool init_network() {
        _network->init();
        return try_ensure_network_status_changed_to(
          NETWORK_IDLE, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS, SSCMA_WIRELESS_NETWORK_POLL_RETRY);
    }

    bool try_join_wireless_network() {
        auto config = wireless_network_config_t{};
        {
            const Guard guard(_network_config_sync_lock);
            config                           = _wireless_network_config;
            _wireless_network_config_updated = true;
        }

        auto ret = _network->join(config.name, config.passwd);
        if (ret == EL_OK) [[likely]]
            ret = try_ensure_network_status_changed_to(
                    NETWORK_JOINED, SSCMA_WIRELESS_NETWORK_POLL_DELAY_MS, SSCMA_WIRELESS_NETWORK_POLL_RETRY)
                    ? EL_OK
                    : EL_ETIMOUT;

        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@WIFI\", \"code\": ",
                                      std::to_string(ret),
                                      ", \"data\": ",
                                      wireless_network_config_2_json_str(config),
                                      "}\n")};
        _mqtt->send_bytes(ss.c_str(), ss.size());
        static_resource->serial->send_bytes(ss.c_str(), ss.size());

        return ret == EL_OK;
    }

    bool try_connect_mqtt_server() {
        auto config = mqtt_server_config_t{};
        {
            const Guard guard(_network_config_sync_lock);
            config                      = _mqtt_server_config;
            _mqtt_server_config_updated = true;
        }

        auto ret = _network->connect(config, mqtt_recv_cb);
        if (ret == EL_OK) [[likely]]
            ret =
              try_ensure_network_status_changed_to(NETWORK_CONNECTED, SSCMA_MQTT_POLL_DELAY_MS, SSCMA_MQTT_POLL_RETRY)
                ? EL_OK
                : EL_ETIMOUT;

        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"SUPERVISOR@MQTTSERVER\", \"code\": ",
                                      std::to_string(ret),
                                      ", \"data\": ",
                                      mqtt_server_config_2_json_str(config),
                                      "}\n")};
        _mqtt->send_bytes(ss.c_str(), ss.size());
        static_resource->serial->send_bytes(ss.c_str(), ss.size());

        return ret == EL_OK;
    }

    void status_forward() {
        auto current_status = _network->status();
        auto target_status  = current_status;
        {
            const Guard guard(_network_config_sync_lock);
            target_status = _target_network_status;
            if (current_status == target_status) [[unlikely]]
                return;
        }

        switch (current_status) {
        case NETWORK_LOST:
            if (target_status <= NETWORK_LOST) [[unlikely]]
                return;
            if (!init_network()) [[unlikely]]
                return;
            [[fallthrough]];

        case NETWORK_IDLE:
            if (target_status <= NETWORK_IDLE) [[unlikely]]
                return;
            if (!try_join_wireless_network()) [[unlikely]]
                return;
            [[fallthrough]];

        case NETWORK_JOINED:
            if (target_status <= NETWORK_JOINED) [[unlikely]]
                return;
            if (!try_connect_mqtt_server()) [[unlikely]]
                return;
            [[fallthrough]];

        case NETWORK_CONNECTED:
            if (!try_sync_mqtt_pubsub_topics()) [[unlikely]]
                return;
            [[fallthrough]];

        default:
            return;
        }
    }

   private:
    Network* _network;
    MQTT*    _mqtt;
    Storage* _storage;

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
