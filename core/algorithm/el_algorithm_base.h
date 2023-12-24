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
#include <type_traits>

#include "core/el_types.h"
#include "core/engine/el_engine_base.h"

namespace edgelab {

namespace types {

struct el_algorithm_info_t {
    el_algorithm_type_t type;
    el_algorithm_cat_t  categroy;
    el_sensor_type_t    input_from;
};

struct el_algorithm_config_t {
    static constexpr el_algorithm_info_t info{
      .type = EL_ALGO_TYPE_NONE, .categroy = EL_ALGO_CAT_NONE, .input_from = EL_SENSOR_TYPE_NONE};
};

}  // namespace types

namespace base {

using namespace edgelab::types;

class Algorithm {
   protected:
    using InfoType   = std::add_pointer<el_algorithm_info_t>::type;
    using ConfigType = el_algorithm_config_t;

    using InputType  = void*;
    using ResultType = void*;

    using EngineType     = std::add_pointer<Engine>::type;
    using EngineListType = Engines::EngineListType;

    using EnginesType = std::add_pointer<Engines>::type;

   public:
    virtual ~Algorithm();

    static constexpr EngineListType find_invokable_engines(EngineListType engines);

    virtual el_err_code_t invoke(void* input) = 0;
    virtual void*         get_results()       = 0;

    InfoType get_algorithm_info() const;

    uint32_t get_preprocess_time() const;
    uint32_t get_invoke_time() const;
    uint32_t get_postprocess_time() const;

   protected:
    Algorithm(const InfoType info);

    void* __input;

   private:
    const InfoType __algorithm_info;

    uint32_t __preprocess_time;   // ms
    uint32_t __invoke_time;       // ms
    uint32_t __postprocess_time;  // ms
};

class AlgorithmSingleStage : public Algorithm {
   public:
    virtual ~AlgorithmSingleStage() = default;

    virtual el_err_code_t invoke(void* input) override;

   protected:
    AlgorithmSingleStage(EngineListType engines, const InfoType info);

    virtual el_err_code_t preprocess()  = 0;
    virtual el_err_code_t postprocess() = 0;

    EngineType __engine;
};

class AlgorithmMultiStage : public Algorithm {
   public:
    virtual ~AlgorithmMultiStage() = default;

   protected:
    AlgorithmMultiStage(EngineListType engines, const InfoType info);

    EngineListType __engine_list;
};

}  // namespace base

}  // namespace edgelab

#endif
