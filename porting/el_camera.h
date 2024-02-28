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

#ifndef _EL_CAMERA_H_
#define _EL_CAMERA_H_

#include <cstddef>
#include <cstdint>
#include <forward_list>

#include "core/el_types.h"

namespace edgelab {

class Camera {
   public:
    using SensorOptIdType = decltype(el_sensor_opt_t::id);

    Camera(uint32_t supported_opts_mask = 0) : _is_present(false), _is_streaming(false) {
        std::forward_list<el_sensor_opt_t> presets = {
          el_sensor_opt_t{.id = 0,   .details = "240x240 Auto"},
          el_sensor_opt_t{.id = 1,   .details = "480x480 Auto"},
          el_sensor_opt_t{.id = 2,   .details = "640x480 Auto"},
          el_sensor_opt_t{.id = 3,  .details = "1280x720 Auto"},
          el_sensor_opt_t{.id = 4, .details = "1920x1080 Auto"}
        };

        for (auto& opt : presets) {
            if (supported_opts_mask & (1 << opt.id)) _supported_opts.push_front(opt);
        }

        if (!_supported_opts.empty()) {
            _current_opt_id = _supported_opts.front().id;
        }
    }

    virtual ~Camera() = default;

    virtual el_err_code_t init(SensorOptIdType opt_id) = 0;
    virtual el_err_code_t deinit()                     = 0;

    virtual el_err_code_t start_stream() = 0;
    virtual el_err_code_t stop_stream()  = 0;

    virtual el_err_code_t get_frame(el_img_t* img)           = 0;
    virtual el_err_code_t get_processed_frame(el_img_t* img) = 0;

    operator bool() const { return _is_present; }

    bool is_streaming() const { return _is_streaming; }

    SensorOptIdType current_opt_id() const { return _current_opt_id; }
    const char*     current_opt_detail() const {
        for (auto& opt : _supported_opts) {
            if (opt.id == _current_opt_id) return opt.details;
        }
        return "Unknown";
    }
    const std::forward_list<el_sensor_opt_t>& supported_opts() const { return _supported_opts; }

   protected:
    volatile bool _is_present;
    volatile bool _is_streaming;

    SensorOptIdType                    _current_opt_id;
    std::forward_list<el_sensor_opt_t> _supported_opts;
};

}  // namespace edgelab

#endif
