/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
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

#ifndef _EL_NETWORK_ESP_H_
#define _EL_NETWORK_ESP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#include "core/el_types.h"
#include "porting/el_network.h"
#include "porting/espressif/el_config_porting.h"

namespace edgelab {
class NetworkEsp : public Network {
public:
    NetworkEsp()  = default;
    ~NetworkEsp() = default;

    void init() override;
    void deinit() override;
    el_net_sta_t status() override;

    el_err_code_t join(const char* ssid, const char *pwd) override;
    el_err_code_t quit() override;

    el_err_code_t connect(const char* server, const char *user, const char *pass, topic_cb_t cb) override;
    el_err_code_t subscribe(const char* topic, mqtt_qos_t qos) override;
    el_err_code_t unsubscribe(const char* topic) override;
    el_err_code_t publish(const char* topic, const char* dat, uint32_t len, mqtt_qos_t qos) override;

    operator bool() const { return _is_present; }

private:
    esp_netif_t *esp_netif;
    esp_mqtt_client_handle_t mqtt_client;
};

} // namespace edgelab

#endif