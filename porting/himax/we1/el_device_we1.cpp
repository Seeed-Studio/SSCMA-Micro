/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 (Seeed Technology Inc.)
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

#include "el_device_we1.h"

extern "C" {
#include <powermode.h>
}

#include <cstdint>

#include "core/el_debug.h"
#include "el_camera_we1.h"
#include "el_network_we1.h"
#include "el_serial_we1.h"

namespace edgelab {

DeviceWE1::DeviceWE1() { init(); }

void DeviceWE1::init() {
    this->_device_name = "Grove Vision AI (WE-I)";
    this->_device_id   = 0x0001;
    this->_revision_id = 0x0001;

    static uint8_t sensor_id = 0;

    static CameraWE1 camera{};
    this->_camera = &camera;
    this->_registered_sensors.emplace_front(el_sensor_info_t{
      .id = ++sensor_id, .type = el_sensor_type_t::EL_SENSOR_TYPE_CAM, .state = el_sensor_state_t::EL_SENSOR_STA_REG});

    static SerialWE1 serial{};
    this->_serial = &serial;
}

void DeviceWE1::restart() {
#ifdef EXTERNAL_LDO
    hx_lib_pm_chip_rst(PMU_WE1_POWERPLAN_EXTERNAL_LDO);
#else
    hx_lib_pm_chip_rst(PMU_WE1_POWERPLAN_INTERNAL_LDO);
#endif
}

Device* Device::get_device() {
    static DeviceWE1 device{};
    return &device;
}

}  // namespace edgelab
