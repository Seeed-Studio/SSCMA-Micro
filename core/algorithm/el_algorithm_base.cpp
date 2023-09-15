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

#include "el_algorithm_base.h"

#include <cstdint>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/engine/el_engine_base.h"

namespace edgelab::base {

Algorithm::Algorithm(EngineType* engine, const InfoType& info)
    : __p_engine(engine),
      __p_input(nullptr),
      __algorithm_info(info),
      __preprocess_time(0),
      __run_time(0),
      __postprocess_time(0) {
    __input_shape  = engine->get_input_shape(0);
    __output_shape = engine->get_output_shape(0);
    __input_quant  = engine->get_input_quant_param(0);
    __output_quant = engine->get_output_quant_param(0);
}

Algorithm::~Algorithm() {
    __p_engine = nullptr;
    __p_input  = nullptr;

    __preprocess_time  = 0;
    __run_time         = 0;
    __postprocess_time = 0;
}

el_err_code_t Algorithm::underlying_run(void* input) {
    el_err_code_t ret{EL_OK};
    uint32_t      start_time{0};
    uint32_t      end_time{0};

    EL_ASSERT(__p_engine != nullptr);

    __p_input = input;

    // preprocess
    start_time        = el_get_time_ms();
    ret               = preprocess();
    end_time          = el_get_time_ms();
    __preprocess_time = end_time - start_time;

    if (ret != EL_OK) {
        return ret;
    }

    // run
    start_time = el_get_time_ms();
    ret        = __p_engine->run();
    end_time   = el_get_time_ms();
    __run_time = end_time - start_time;

    if (ret != EL_OK) {
        return ret;
    }

    // postprocess
    start_time         = el_get_time_ms();
    ret                = postprocess();
    end_time           = el_get_time_ms();
    __postprocess_time = end_time - start_time;

    return ret;
}

Algorithm::InfoType Algorithm::get_algorithm_info() const { return __algorithm_info; };

uint32_t Algorithm::get_preprocess_time() const { return __preprocess_time; }

uint32_t Algorithm::get_run_time() const { return __run_time; }

uint32_t Algorithm::get_postprocess_time() const { return __postprocess_time; }

}  // namespace edgelab::base
