#pragma once

#include <cstdint>
#include <cstring>
#include <list>
#include <string>
#include <unordered_set>
#include <utility>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "porting/el_device.h"
#include "porting/el_misc.h"
#include "porting/el_transport.h"
#include "sscma/definations.hpp"
#include "sscma/interface/wifi.hpp"
#include "sscma/prototypes.hpp"
#include "sscma/types.hpp"
#include "sscma/utility.hpp"

namespace sscma {

using namespace edgelab;

using namespace sscma::types;
using namespace sscma::prototypes;
using namespace sscma::utility;

namespace types {

typedef enum class mqtt_sta_e { UNKNOWN = 0, DISCONNECTED, CONNECTED, BUSY } mqtt_sta_e;

}

namespace transport {

class MQTT final : public Supervisable, public Transport {
   public:
    MQTT(StatefulInterface* interface)
        : _interface(interface),
          _network(Device::get_device()->get_network()),
          _device_lock(),
          _size(SSCMA_CMD_MAX_LENGTH << 1),  // 2 times of the max length of command
          _buffer(new char[_size]{}),
          _head(0),
          _tail(0),
          _mqtt_server_config_lock(),
          _mqtt_server_config_queue(),
          _mqtt_pubsub_config(),
          _sub_topics_set() {
        EL_ASSERT(_interface);
        EL_ASSERT(_network);
        EL_ASSERT(_buffer);
        _mqtt_handler_ptr   = this;
        _mqtt_pubsub_config = get_default_mqtt_pubsub_config(Device::get_device());
        _mqtt_server_config_queue.emplace_back(sync_status_from_driver(), mqtt_server_config_t{});
        this->_is_present = true;
        _interface->add_pre_down_callback(this, [](void* from) {
            auto mqtt = static_cast<MQTT*>(from);
            mqtt->set_down({mqtt_sta_e::DISCONNECTED, mqtt->get_last_mqtt_server_config()});
        });
    }

    ~MQTT() {
        _interface->remove_pre_down_callback(this);

        this->_is_present = false;
        _mqtt_handler_ptr = nullptr;
        _interface        = nullptr;
        _network          = nullptr;

        if (_buffer) [[likely]] {
            delete[] _buffer;
            _buffer = nullptr;
        }
    }

    void poll_from_supervisor() override {
        EL_LOGI("[SSCMA] MQTT::poll_from_supervisor()");
        auto mqtt_server_config        = std::pair<mqtt_sta_e, mqtt_server_config_t>{};
        bool mqtt_server_config_synced = true;
        {
            const Guard guard(_mqtt_server_config_lock);
            mqtt_server_config = _mqtt_server_config_queue.front();

            if (_mqtt_server_config_queue.size() > 1) [[unlikely]] {
                mqtt_server_config        = _mqtt_server_config_queue.back();
                mqtt_server_config_synced = false;
                while (_mqtt_server_config_queue.size() > 1) _mqtt_server_config_queue.pop_front();
            }
        }

        EL_LOGI("[SSCMA] MQTT::poll_from_supervisor() - mqtt_server_config_synced: %d", mqtt_server_config_synced);
        EL_LOGI("[SSCMA] MQTT::poll_from_supervisor() - target_status: %d", mqtt_server_config.first);
        EL_LOGI("[SSCMA] MQTT::poll_from_supervisor() - mqtt address: %s", mqtt_server_config.second.address);

        if (mqtt_server_config_synced) [[likely]] {
            auto current_sta = sync_status_from_driver();
            if (current_sta < mqtt_server_config.first) [[unlikely]]
                bring_up(mqtt_server_config);
        } else {
            set_down(mqtt_server_config);
            bring_up(mqtt_server_config);
        }
    }

    bool set_mqtt_server_config(mqtt_server_config_t mqtt_server_config) {
        // use default port if not specified
        if (mqtt_server_config.port == 0) [[unlikely]] {
            if (mqtt_server_config.use_ssl)
                mqtt_server_config.port = SSCMA_MQTT_DEFAULT_SSL_PORT;
            else
                mqtt_server_config.port = SSCMA_MQTT_DEFAULT_PORT;
        }

        const Guard guard(_mqtt_server_config_lock);
        _mqtt_server_config_queue.emplace_back(
          std::strlen(mqtt_server_config.address) ? mqtt_sta_e::CONNECTED : mqtt_sta_e::DISCONNECTED,
          mqtt_server_config);

        return true;
    }

    mqtt_server_config_t get_last_mqtt_server_config() const {
        const Guard guard(_mqtt_server_config_lock);
        return _mqtt_server_config_queue.front().second;
    }

    bool is_mqtt_server_config_synchornized() const {
        const Guard guard(_mqtt_server_config_lock);
        return _mqtt_server_config_queue.size() <= 1;
    }

    bool is_mqtt_server_connected() const { return sync_status_from_driver() >= mqtt_sta_e::CONNECTED; }

    mqtt_pubsub_config_t get_mqtt_pubsub_config() const { return _mqtt_pubsub_config; }

    el_err_code_t read_bytes(char* buffer, std::size_t size) override {
        if (!buffer | !size) [[unlikely]]
            return EL_EINVAL;

        auto        head = _head;
        auto        tail = _tail;
        std::size_t len  = 0;

        while (tail != head && len < size) {
            buffer[len++] = _buffer[tail];
            tail          = (tail + 1) % _size;
        }
        _tail = tail;

        return EL_OK;
    }

    el_err_code_t send_bytes(const char* buffer, std::size_t size) override {
        return m_send_bytes(_mqtt_pubsub_config.pub_topic, _mqtt_pubsub_config.pub_qos, buffer, size);
    }

    char echo(bool only_visible = true) override {
        auto c = get_char();
        if (only_visible && !std::isprint(c)) return c;
        m_send_bytes(_mqtt_pubsub_config.pub_topic, _mqtt_pubsub_config.pub_qos, &c, 1);
        return c;
    }

    char get_char() override {
        auto head = _head;
        auto tail = _tail;
        if (head == tail) [[unlikely]]
            return '\0';
        return _buffer[_tail = (tail + 1) % _size];
    }

    std::size_t get_line(char* buffer, std::size_t size, const char delim = 0x0d) override {
        if (!buffer | !size) [[unlikely]]
            return 0;

        auto              head    = _head;
        auto              tail    = _tail;
        auto              prev    = tail;
        std::size_t const len_max = size - 1;
        std::size_t       len     = 0;

        for (char c = _buffer[tail]; tail != head; tail = (tail + 1) % _size, ++len) {
            if ((c == delim) | (c == '\0')) {
                tail = (tail + 1) % _size;
                len += c != '\0';
                break;
            }
        }

        if (len) {
            for (std::size_t i = 0; i < len; ++i) buffer[i] = _buffer[(prev + i) % _size];
            _tail = tail;
        }

        return len;
    }

    void emit_mqtt_discover() {
        const auto& ss{concat_strings("\r", mqtt_pubsub_config_2_json_str(_mqtt_pubsub_config), "\n")};
        char        discover_topic[SSCMA_MQTT_TOPIC_LEN]{};
        std::snprintf(
          discover_topic, sizeof(discover_topic) - 1, SSCMA_MQTT_DISCOVER_TOPIC, SSCMA_AT_API_MAJOR_VERSION);

        m_send_bytes(discover_topic, 0, ss.c_str(), ss.size());
    }

   protected:
    static inline void mqtt_subscribe_callback(char* top, int tlen, char* msg, int mlen) {
        if (!_mqtt_handler_ptr) [[unlikely]]
            return;
        auto this_ptr = static_cast<MQTT*>(_mqtt_handler_ptr);

        if (tlen ^ std::strlen(this_ptr->_mqtt_pubsub_config.sub_topic) ||
            std::strncmp(top, this_ptr->_mqtt_pubsub_config.sub_topic, tlen) || mlen <= 1)
            return;

        if (!this_ptr->push_to_buffer(msg, mlen)) [[unlikely]]
            EL_LOGI("[SSCMA] MQTT::mqtt_subscribe_callback() - buffer may corrupted");
    }

    inline bool push_to_buffer(const char* bytes, std::size_t size) {
        if (!bytes | !size) [[unlikely]]
            return true;

        auto head = _head;
        auto tail = _tail;

        for (std::size_t i = 0; i < size; ++i) {
            _buffer[head] = bytes[i];
            head          = (head + 1) % _size;
            if (head == tail) [[unlikely]]
                return false;  // buffer corrupted
        }

        _head = head;

        return true;
    }

    inline el_err_code_t m_send_bytes(const char* topic, uint8_t qos, const char* buffer, std::size_t size) {
        const Guard guard(_device_lock);
        if (sync_status_from_driver() != mqtt_sta_e::CONNECTED) [[unlikely]]
            return EL_EPERM;
        return _network->publish(topic, buffer, size, static_cast<mqtt_qos_t>(qos));
    }

    void bring_up(const std::pair<mqtt_sta_e, mqtt_server_config_t>& config) {
        EL_LOGI("[SSCMA] MQTT::bring_up() checking interface status");
        if (!_interface->is_interface_up()) [[unlikely]]
            return;

        auto current_sta = sync_status_from_driver();

        EL_LOGI("[SSCMA] MQTT::bring_up() current status: %d, target status: %d", current_sta, config.first);

        if (current_sta == mqtt_sta_e::DISCONNECTED) {
            if (current_sta >= config.first) return;
            // TODO: driver change topic callback API
            EL_LOGI("[SSCMA] MQTT::bring_up() driver connect: %s:%d", config.second.address, config.second.port);
            auto ret = _network->connect(config.second, mqtt_subscribe_callback);  // driver connect
            if (ret != EL_OK) [[unlikely]]
                return;
            current_sta =
              try_ensure_wifi_status_changed_from(current_sta, SSCMA_MQTT_POLL_DELAY_MS, SSCMA_MQTT_POLL_RETRY);
        }

        if (current_sta == mqtt_sta_e::CONNECTED) {
            EL_LOGI("[SSCMA] MQTT::bring_up() driver subscribe: %s", _mqtt_pubsub_config.sub_topic);
            auto ret = _network->subscribe(_mqtt_pubsub_config.sub_topic,
                                           static_cast<mqtt_qos_t>(_mqtt_pubsub_config.sub_qos));  // driver subscribe
            if (ret != EL_OK) [[unlikely]]
                return;
            _sub_topics_set.emplace(_mqtt_pubsub_config.sub_topic);
            EL_LOGI("[SSCMA] MQTT::bring_up() emit_mqtt_discover()");
            emit_mqtt_discover();
        }
    }

    void set_down(const std::pair<mqtt_sta_e, mqtt_server_config_t>& config) {
        const Guard guard(_device_lock);

        auto current_sta = sync_status_from_driver();

        EL_LOGI("[SSCMA] MQTT::set_down() current status: %d, target status: %d", current_sta, config.first);

        if (current_sta == mqtt_sta_e::CONNECTED) {
            if (current_sta < config.first) return;

            EL_LOGI("[SSCMA] MQTT::set_down() driver unsubscribe topic count: %d", _sub_topics_set.size());
            if (_sub_topics_set.size()) [[likely]] {
                for (const auto& sub_topic : _sub_topics_set) _network->unsubscribe(sub_topic.c_str());
                _sub_topics_set.clear();  // clear old topics
            }

            EL_LOGI("[SSCMA] MQTT::set_down() - driver disconnect()");
            auto ret = _network->disconnect();  // driver disconnect
            if (ret != EL_OK) [[unlikely]]
                return;
            current_sta = ensure_status_changed_from(current_sta, SSCMA_MQTT_POLL_DELAY_MS);
        }
    }

    mqtt_sta_e try_ensure_wifi_status_changed_from(mqtt_sta_e old_sta, uint32_t delay_ms, std::size_t n_times) {
        auto sta = sync_status_from_driver();
        for (std::size_t i = 0; (i < n_times) & (sta != old_sta); ++i) {
            el_sleep(delay_ms);
            sta = sync_status_from_driver();
        }
        return sta;
    }

    mqtt_sta_e ensure_status_changed_from(mqtt_sta_e old_sta, uint32_t delay_ms) {
        auto sta = sync_status_from_driver();
        for (; sta == old_sta;) {
            el_sleep(delay_ms);
            sta = sync_status_from_driver();
        }
        return sta;
    }

    mqtt_sta_e sync_status_from_driver() const {
        auto driver_sta = _network->status();
        switch (driver_sta) {
        case NETWORK_LOST:
            return mqtt_sta_e::DISCONNECTED;
        case NETWORK_IDLE:
            return mqtt_sta_e::DISCONNECTED;
        case NETWORK_JOINED:
            return mqtt_sta_e::DISCONNECTED;
        case NETWORK_CONNECTED:
            return mqtt_sta_e::CONNECTED;
        default:
            return mqtt_sta_e::UNKNOWN;
        }
    }

   private:
    StatefulInterface* _interface;
    Network*           _network;

    Mutex _device_lock;

    static MQTT*         _mqtt_handler_ptr;
    const std::size_t    _size;
    char*                _buffer;
    volatile std::size_t _head;
    volatile std::size_t _tail;

    Mutex                                                  _mqtt_server_config_lock;
    std::list<std::pair<mqtt_sta_e, mqtt_server_config_t>> _mqtt_server_config_queue;
    mqtt_pubsub_config_t                                   _mqtt_pubsub_config;
    std::unordered_set<std::string>                        _sub_topics_set;
};

// Important: due to driver's API, the MQTT handler must be a singleton currently
MQTT* MQTT::_mqtt_handler_ptr = nullptr;

}  // namespace transport

}  // namespace sscma
