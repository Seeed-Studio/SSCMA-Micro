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

#ifndef _EL_ENGINE_BASE_H_
#define _EL_ENGINE_BASE_H_

#include <cstdint>
#include <forward_list>

#include "core/el_config_internal.h"
#include "core/el_types.h"

namespace edgelab::base {

class Engines;

class Engine {
   public:
    virtual void*         get_input(std::size_t index)                                                 = 0;
    virtual el_err_code_t set_input(std::size_t index, const void* input_data, std::size_t input_size) = 0;

    virtual void* get_output(std::size_t index) = 0;

    virtual el_err_code_t run() = 0;

    virtual el_shape_t       get_input_shape(std::size_t index) const        = 0;
    virtual el_shape_t       get_output_shape(std::size_t index) const       = 0;
    virtual el_quant_param_t get_input_quant_param(std::size_t index) const  = 0;
    virtual el_quant_param_t get_output_quant_param(std::size_t index) const = 0;

   protected:
    Engine()          = default;
    virtual ~Engine() = default;

    friend class Engines;
};

class Engines {
   public:
    using EngineListType = std::forward_list<std::add_pointer<Engine>::type>;

   public:
    virtual ~Engines() = default;

    virtual el_err_code_t init(void* memory_resource, std::std::size_t size)             = 0;
    virtual el_err_code_t spawn_from_model(const void* model_ptr, std::std::size_t size) = 0;
#ifdef CONFIG_EL_FILESYSTEM
    virtual el_err_code_t spawn_from_model(const char* model_path) = 0;
#endif
    virtual el_err_code_t release() = 0;

    EngineListType get_engines() { return &__engines; }

   protected:
    Engines() = default;

   private:
    EngineListType __engines;

#ifdef CONFIG_EL_FILESYSTEM
    std::forward_list<uint8_t*> __model_buffers;
#endif
};

}  // namespace edgelab::base

#endif
