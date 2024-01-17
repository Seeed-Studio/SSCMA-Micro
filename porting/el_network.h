/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _EL_NETWORK_H_
#define _EL_NETWORK_H_

#include "core/el_common.h"
#include "core/el_types.h"

#include "core/utils/el_ringbuffer.hpp"
#include "sscma/definations.hpp"
#include "sscma/types.hpp"

typedef enum {
    NETWORK_LOST = 0,
    NETWORK_IDLE,
    NETWORK_JOINED,
    NETWORK_CONNECTED,
} el_net_sta_t;

typedef enum {
    MQTT_QOS_0 = 0,
    MQTT_QOS_1,
    MQTT_QOS_2
} mqtt_qos_t;

#define MDNS_ITEM_PROTOCAL "protocol"
#define MDNS_ITEM_SERVER   "server"
#define MDNS_ITEM_PORT     "port"
#define MDNS_ITEM_DEST     "dest"
#define MDNS_ITEM_AUTH     "auth"
typedef struct mdns_record {
    char host_name[SSCMA_MDNS_HOST_NAME_LEN];
    // properties: server, port, protocol, destination
    char server[SSCMA_MDNS_SERV_NAME_LEN];
    char destination[SSCMA_MDNS_DEST_NAME_LEN];
    char protocol[8];
    char authentication[8];
    uint16_t port;
} mdns_record_t;

typedef void (*status_cb_t)(el_net_sta_t sta);
typedef void (*topic_cb_t)(char* top, int tlen, char* msg, int mlen);

using namespace sscma::types;

namespace edgelab {

// WIFI-STA for MQTT
class Network {
public:
    Network() : _ip({0}), _is_present(false), network_status(NETWORK_LOST) {}
    virtual ~Network() = default;

    virtual void init(status_cb_t cb) = 0;
    void init() { 
        this->init(nullptr); 
    };
    virtual void deinit() = 0;
    el_net_sta_t status() {
        return this->network_status;
    };
    void set_status(el_net_sta_t status) { 
        this->network_status = status; 
        if (this->status_cb) {
            this->status_cb(this->network_status);
        }
    }
    el_err_code_t get_ip(in4_info_t &ip) { 
        if (this->network_status == NETWORK_LOST || 
            this->network_status == NETWORK_IDLE) {
            return EL_EPERM;
        }
        ip = this->_ip;
        return EL_OK;
    }

    /* WIFI station */
    virtual el_err_code_t join(const char* ssid, const char *pwd) = 0;
    virtual el_err_code_t quit() = 0;
    virtual el_err_code_t set_mdns(mdns_record_t record) = 0;

    /* MQTT client */
    virtual el_err_code_t connect(mqtt_server_config_t mqtt_cfg, topic_cb_t cb) = 0;
    virtual el_err_code_t disconnect() = 0;
    virtual el_err_code_t subscribe(const char* topic, mqtt_qos_t qos) = 0;
    virtual el_err_code_t unsubscribe(const char* topic) = 0;
    virtual el_err_code_t publish(const char* topic, const char* dat, uint32_t len, mqtt_qos_t qos) = 0;

    /* TODO: Add HTTP client */
    // virtual el_err_code_t request(const char* url, const char* method, const char* payload);
    // virtual el_err_code_t get(const char* url, const char* payload);
    // virtual el_err_code_t post(const char* url, const char* payload);

    operator bool() const { return _is_present; }

    status_cb_t status_cb = nullptr;
    topic_cb_t topic_cb = nullptr;
    in4_info_t _ip;

protected:
    bool _is_present;
    el_net_sta_t network_status;
};

}  // namespace edgelab

#endif