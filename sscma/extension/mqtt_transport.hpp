#pragma once

#include <cstdint>
#include <string>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "porting/el_device.h"
#include "porting/el_network.h"
#include "porting/el_transport.h"
#include "sscma/definations.hpp"
#include "sscma/types.hpp"
#include "sscma/utility.hpp"

namespace sscma::extension {

using namespace edgelab;

using namespace sscma::types;
using namespace sscma::utility;

class MQTT final : public Transport {
   public:
    MQTT(Network* network)
        : _network(network),
          _size(SSCMA_CMD_MAX_LENGTH << 1),  // 2 times of the max length of command
          _buffer(new char[_size]),
          _head(0),
          _tail(0),
          _dev_lock(),
          _mqtt_pubsub_config() {
        EL_ASSERT(_buffer);
        std::memset(_buffer, 0, _size);
        this->_is_present = this->_network != nullptr;
    }

    ~MQTT() {
        this->_is_present = false;
        _network = nullptr;
        if (_buffer) [[likely]] {
            delete[] _buffer;
            _buffer = nullptr;
        }
    }

    void set_mqtt_pubsub_config(const mqtt_pubsub_config_t& mqtt_pubsub_config) {
        const Guard guard(_dev_lock);
        _mqtt_pubsub_config = mqtt_pubsub_config;
    }

    decltype(auto) get_mqtt_pubsub_config() {
        const Guard guard(_dev_lock);
        return _mqtt_pubsub_config;
    }

    void emit_mqtt_discover() {
        const Guard guard(_dev_lock);
        const auto& ss{concat_strings("\r", mqtt_pubsub_config_2_json_str(_mqtt_pubsub_config), "\n")};
        char        discover_topic[SSCMA_MQTT_TOPIC_LEN]{};
        std::snprintf(
          discover_topic, sizeof(discover_topic) - 1, SSCMA_MQTT_DISCOVER_TOPIC, SSCMA_AT_API_MAJOR_VERSION);

        _network->publish(discover_topic, ss.c_str(), ss.size(), MQTT_QOS_0);
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

    inline el_err_code_t read_bytes(char* buffer, std::size_t size) override {
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

    inline el_err_code_t send_bytes(const char* buffer, std::size_t size) override {
        const Guard guard(_dev_lock);
        return m_send_bytes(buffer, size);
    }

    inline char echo(bool only_visible = true) override {
        auto c = get_char();
        if (only_visible && !std::isprint(c)) return c;
        m_send_bytes(&c, 1);
        return c;
    }

    inline char get_char() override {
        auto head = _head;
        auto tail = _tail;
        if (head == tail) [[unlikely]]
            return '\0';
        return _buffer[_tail = (tail + 1) % _size];
    }

    inline std::size_t get_line(char* buffer, std::size_t size, const char delim = 0x0d) override {
        if (!buffer | !size) [[unlikely]]
            return 0;

        auto        head = _head;
        auto        tail = _tail;
        auto        prev = tail;
        std::size_t len  = 0;

        char c = _buffer[tail];
        while (tail != head) {
            if ((c == delim) | (c == '\0')) {
                ++len;
                tail = (tail + 1) % _size;
                goto GetTheLine;
            }
            ++len;
            tail = (tail + 1) % _size;
            c    = _buffer[tail];
        }

        return 0;

    GetTheLine:
        // here we're not care about the delim or terminator '\0'
        // if len reach the size and the delim is not '\0', the buffer will not be null-terminated
        size = len < size ? len : size;
        for (std::size_t i = 0; i < size; ++i) buffer[i] = _buffer[(prev + i) % _size];
        _tail = tail;

        return size;
    }

   protected:
    inline el_err_code_t m_send_bytes(const char* buffer, std::size_t size) {
        return _network->publish(
          _mqtt_pubsub_config.pub_topic, buffer, size, static_cast<mqtt_qos_t>(_mqtt_pubsub_config.pub_qos));
    }

   private:
    Network* _network;

    const std::size_t    _size;
    char*                _buffer;
    volatile std::size_t _head;
    volatile std::size_t _tail;

    Mutex                _dev_lock;
    mqtt_pubsub_config_t _mqtt_pubsub_config;
};

}  // namespace sscma::extension
