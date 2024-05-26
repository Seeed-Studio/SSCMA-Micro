#ifndef _MA_ENGINE_BASE_H_
#define _MA_ENGINE_BASE_H_

#include <cstdint>

#include "core/ma_config_internal.h"
#include "core/ma_ctypes.h"

#if MA_USE_FILESYSTEM
#include <fstream>
#include <iostream>
#endif

namespace ma::engine {

class BaseEngine {
public:
    BaseEngine()          = default;
    virtual ~BaseEngine() = default;

    virtual ma_err_t init()                        = 0;
    virtual ma_err_t init(size_t size)             = 0;
    virtual ma_err_t init(void* pool, size_t size) = 0;

    virtual ma_err_t run() = 0;

    virtual ma_err_t load_model(const void* model_data, size_t model_size) = 0;
#if MA_USE_FILESYSTEM
    virtual ma_err_t load_model(const char* model_path) = 0;
#endif

    virtual ma_tensor_t      get_input(size_t index)              = 0;
    virtual ma_tensor_t      get_output(size_t index)             = 0;
    virtual ma_shape_t       get_input_shape(size_t index)        = 0;
    virtual ma_shape_t       get_output_shape(size_t index)       = 0;
    virtual ma_quant_param_t get_input_quant_param(size_t index)  = 0;
    virtual ma_quant_param_t get_output_quant_param(size_t index) = 0;

#if MA_USE_ENGINE_TENSOR_NAME
    virtual size_t           get_input_index(const char* input_name)         = 0;
    virtual size_t           get_output_index(const char* output_name)       = 0;
    virtual ma_tensor_t      get_input(const char* input_name)               = 0;
    virtual ma_tensor_t      get_output(const char* output_name)             = 0;
    virtual ma_shape_t       get_input_shape(const char* input_name)         = 0;
    virtual ma_shape_t       get_output_shape(const char* output_name)       = 0;
    virtual ma_quant_param_t get_input_quant_param(const char* input_name)   = 0;
    virtual ma_quant_param_t get_output_quant_param(const char* output_name) = 0;
#endif
};

}  // namespace ma::engine

#endif
