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

#ifndef _EL_NETWORK_WE2_H_
#define _EL_NETWORK_WE2_H_

#include "el_network_at.h"

#include "core/el_debug.h"

namespace edgelab {

class NetworkWE2 : public Network {
public:
    NetworkWE2()  = default;
    ~NetworkWE2() = default;

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
    esp_at_t* _at;
};

} // namespace edgelab

#endif
