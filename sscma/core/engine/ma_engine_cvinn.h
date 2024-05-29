#ifndef _MA_ENGINE_CVINN_H_
#define _MA_ENGINE_CVINN_H_

#include <cstddef>
#include <cstdint>


#include "core/ma_common.h"

#if MA_USE_ENGINE_CVINN

#include <cviruntime.h>

#include "ma_engine_base.h"


namespace ma::engine {

class EngineCVINN final : public BaseEngine {
public:
    EngineCVINN();
    ~EngineCVINN();

    ma_err_t init() override;
    ma_err_t init(size_t size) override;
    ma_err_t init(void* pool, size_t size) override;

    ma_err_t run() override;

    ma_err_t load_model(const void* model_data, size_t model_size) override;
#if MA_USE_FILESYSTEM
    ma_err_t load_model(const char* model_path) override;
#endif

    size_t           get_input_size() override;
    size_t           get_output_size() override;
    ma_tensor_t      get_input(const char* name) override;
    ma_tensor_t      get_output(const char* name) override;
    ma_shape_t       get_input_shape(const char* name) override;
    ma_shape_t       get_output_shape(const char* name) override;
    ma_quant_param_t get_input_quant_param(const char* name) override;
    ma_quant_param_t get_output_quant_param(const char* name) override;

private:
    CVI_MODEL_HANDLE model;
    CVI_TENSOR*      input_tensors;
    CVI_TENSOR*      output_tensors;
    int32_t          input_num;
    int32_t          output_num;
};

}  // namespace ma::engine

#endif

#endif  // _MA_ENGINE_CVINN_H_