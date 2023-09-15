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

#ifndef _EL_DEVICE_H_
#define _EL_DEVICE_H_

#include <algorithm>
#include <forward_list>

#include "core/el_types.h"
#include "porting/el_camera.h"
#include "porting/el_display.h"
#include "porting/el_serial.h"
#include "porting/el_transport.h"

namespace edgelab {

class Device {
   public:
    ~Device() = default;

    static Device* get_device();

    const char* get_device_name() const { return _device_name; }
    uint32_t    get_device_id() const { return _device_id; }
    uint32_t    get_chip_revision_id() const { return _revision_id; }

    Camera*  get_camera() { return _camera; }
    Display* get_display() { return _display; }
    Serial*  get_serial() { return _serial; }

    virtual void restart() = 0;

    el_sensor_info_t get_sensor_info(uint8_t id) const {
        auto it = std::find_if(_registered_sensors.begin(), _registered_sensors.end(), [&](const el_sensor_info_t& s) {
            return s.id == id;
        });
        if (it != _registered_sensors.end()) return *it;
        return {};
    }

    el_sensor_info_t get_sensor_info(uint8_t id, el_sensor_type_t type) const {
        auto it = std::find_if(_registered_sensors.begin(), _registered_sensors.end(), [&](const el_sensor_info_t& s) {
            return s.id == id && s.type == type;
        });
        if (it != _registered_sensors.end()) return *it;
        return {};
    }

    const std::forward_list<el_sensor_info_t>& get_all_sensor_info() const { return _registered_sensors; }

    size_t get_all_sensor_info_count() const {
        return std::distance(_registered_sensors.begin(), _registered_sensors.end());
    }

    bool has_sensor(uint8_t id) const {
        auto it = std::find_if(_registered_sensors.begin(), _registered_sensors.end(), [&](const el_sensor_info_t& s) {
            return s.id == id;
        });
        return it != _registered_sensors.end();
    }

    bool has_sensor(uint8_t id, el_sensor_type_t type) const {
        auto it = std::find_if(_registered_sensors.begin(), _registered_sensors.end(), [&](const el_sensor_info_t& s) {
            return s.id == id && s.type == type;
        });
        return it != _registered_sensors.end();
    }

    bool set_sensor_state(uint8_t id, el_sensor_state_t state) {
        auto it = std::find_if(_registered_sensors.begin(), _registered_sensors.end(), [&](const el_sensor_info_t& s) {
            return s.id == id;
        });
        if (it == _registered_sensors.end()) return false;
        it->state = state;
        return true;
    }

   protected:
    Device()
        : _device_name(""), _device_id(0), _revision_id(0), _camera(nullptr), _display(nullptr), _serial(nullptr) {}

    const char* _device_name;
    uint32_t    _device_id;
    uint32_t    _revision_id;

    Camera*  _camera;
    Display* _display;
    Serial*  _serial;

    std::forward_list<el_sensor_info_t> _registered_sensors;
};

}  // namespace edgelab

#endif
