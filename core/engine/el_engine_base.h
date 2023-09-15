/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu, nullptr (Seeed Technology Inc.)
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

#ifndef _EL_ENGINE_BASE_H_
#define _EL_ENGINE_BASE_H_

#include <cstdint>

#include "core/el_config_internal.h"
#include "core/el_types.h"

#ifdef CONFIG_EL_FILESYSTEM
    #include <fstream>
    #include <iostream>
#endif

namespace edgelab::base {

class Engine {
   public:
    Engine()          = default;
    virtual ~Engine() = default;

    virtual el_err_code_t init()                        = 0;
    virtual el_err_code_t init(size_t size)             = 0;
    virtual el_err_code_t init(void* pool, size_t size) = 0;

    virtual el_err_code_t run() = 0;

#ifdef CONFIG_EL_FILESYSTEM
    virtual el_err_code_t load_model(const char* model_path) = 0;
#endif

    virtual el_err_code_t load_model(const void* model_data, size_t model_size) = 0;

    virtual el_err_code_t set_input(size_t index, const void* input_data, size_t input_size) = 0;
    virtual void*         get_input(size_t index)                                            = 0;

    virtual void* get_output(size_t index) = 0;

    virtual el_shape_t       get_input_shape(size_t index) const        = 0;
    virtual el_shape_t       get_output_shape(size_t index) const       = 0;
    virtual el_quant_param_t get_input_quant_param(size_t index) const  = 0;
    virtual el_quant_param_t get_output_quant_param(size_t index) const = 0;

#ifdef CONFIG_EL_INFERENCER_TENSOR_NAME
    virtual size_t           get_input_index(const char* input_name) const                                = 0;
    virtual size_t           get_output_index(const char* output_name) const                              = 0;
    virtual void*            get_input(const char* input_name)                                            = 0;
    virtual el_err_code_t    set_input(const char* input_name, const void* input_data, size_t input_size) = 0;
    virtual void*            get_output(const char* output_name)                                          = 0;
    virtual el_shape_t       get_input_shape(const char* input_name) const                                = 0;
    virtual el_shape_t       get_output_shape(const char* output_name) const                              = 0;
    virtual el_quant_param_t get_input_quant_param(const char* input_name) const                          = 0;
    virtual el_quant_param_t get_output_quant_param(const char* output_name) const                        = 0;
#endif
};

}  // namespace edgelab::base

#endif
