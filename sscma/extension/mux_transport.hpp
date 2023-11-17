#pragma once

#include <cstdint>
#include <string>

#include "core/el_types.h"
#include "core/synchronize/el_mutex.hpp"
#include "porting/el_device.h"
#include "porting/el_misc.h"
#include "porting/el_network.h"
#include "porting/el_serial.h"
#include "porting/el_transport.h"
#include "sscma/definations.hpp"
#include "sscma/types.hpp"
#include "sscma/utility.hpp"

namespace sscma::extension {

using namespace edgelab;

using namespace sscma::types;
using namespace sscma::utility;

class MuxTransport final : public Transport {
   public:
    static MuxTransport* get_transport() {
        static MuxTransport transport;
        return &transport;
    }

    ~MuxTransport() { deinit(); }

    el_err_code_t init() override {
        EL_ASSERT(this->_is_present != true);

        auto device = Device::get_device();
        _serial     = device->get_serial();
        _network    = device->get_network();

        this->_is_present = true;

        return (_serial && _network) ? EL_OK : EL_EIO;
    }

    el_err_code_t deinit() override {
        _serial  = nullptr;
        _network = nullptr;

        this->_is_present = false;

        return EL_OK;
    }

    void set_mqtt_config(mqtt_pubsub_config_t mqtt_pubsub_config) {
        const Guard guard(_config_lock);
        _mqtt_pubsub_config = mqtt_pubsub_config;
    }

    mqtt_pubsub_config_t get_mqtt_config() {
        const Guard guard(_config_lock);
        return _mqtt_pubsub_config;
    }

    void emit_mqtt_discover() {
        const Guard guard(_config_lock);
        const auto& ss{concat_strings("\r", mqtt_pubsub_config_2_json_str(_mqtt_pubsub_config), "\n")};
        char        discover_topic[SSCMA_MQTT_TOPIC_LEN]{};
        std::snprintf(
          discover_topic, sizeof(discover_topic) - 1, SSCMA_MQTT_DISCOVER_TOPIC, SSCMA_AT_API_MAJOR_VERSION);

        _network->publish(discover_topic, ss.c_str(), ss.size(), MQTT_QOS_0);
    }

    inline el_err_code_t read_bytes(char* buffer, size_t size) override { return _serial->read_bytes(buffer, size); }

    inline el_err_code_t send_bytes(const char* buffer, size_t size) override {
        const Guard guard(_config_lock);
        return m_send_bytes(buffer, size);
    }

    inline char echo(bool only_visible = true) override { return _serial->echo(only_visible); }

    inline char get_char() override { return _serial->get_char(); }

    inline size_t get_line(char* buffer, size_t size, const char delim = 0x0d) override {
        return _serial->get_line(buffer, size, delim);
    }

   protected:
    MuxTransport() = default;

    inline el_err_code_t m_send_bytes(const char* buffer, size_t size) {
        auto serial_ret = _serial->send_bytes(buffer, size);
        auto network_ret = _network->publish(
          _mqtt_pubsub_config.pub_topic, buffer, size, static_cast<mqtt_qos_t>(_mqtt_pubsub_config.pub_qos));

        return ((serial_ret ^ EL_OK) | (network_ret ^ EL_OK)) ? EL_EIO : EL_OK;  // require both ok
    }

   private:
    Serial*  _serial;
    Network* _network;

    Mutex                _config_lock;
    mqtt_pubsub_config_t _mqtt_pubsub_config;
};

}  // namespace sscma::extension
