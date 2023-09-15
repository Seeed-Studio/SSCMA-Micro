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

#ifndef _EL_ALGORITHM_BASE_H_
#define _EL_ALGORITHM_BASE_H_

#include <cstdint>

#include "core/el_types.h"
#include "core/engine/el_engine_base.h"

namespace edgelab {

namespace types {

struct el_algorithm_info_t {
    el_algorithm_type_t type;
    el_algorithm_cat_t  categroy;
    el_sensor_type_t    input_from;
};

}  // namespace types

namespace base {

using namespace edgelab::types;

class Algorithm {
   protected:
    using EngineType = Engine;
    using InfoType   = el_algorithm_info_t;

   public:
    Algorithm(EngineType* engine, const InfoType& info);
    virtual ~Algorithm();

    InfoType get_algorithm_info() const;

    uint32_t get_preprocess_time() const;
    uint32_t get_run_time() const;
    uint32_t get_postprocess_time() const;

   protected:
    el_err_code_t underlying_run(void* input);

    virtual el_err_code_t preprocess()  = 0;
    virtual el_err_code_t postprocess() = 0;

    EngineType* __p_engine;

    void* __p_input;

    el_shape_t __input_shape;
    el_shape_t __output_shape;

    el_quant_param_t __input_quant;
    el_quant_param_t __output_quant;

   private:
    InfoType __algorithm_info;

    uint32_t __preprocess_time;   // ms
    uint32_t __run_time;          // ms
    uint32_t __postprocess_time;  // ms
};

}  // namespace base

}  // namespace edgelab

#endif
