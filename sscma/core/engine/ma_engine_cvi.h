#ifndef _MA_ENGINE_CVI_H_
#define _MA_ENGINE_CVI_H_

#include <cstddef>
#include <cstdint>


#include "../ma_common.h"

#if MA_USE_ENGINE_CVI

#include <cviruntime.h>

#include "ma_engine_base.h"


namespace ma::engine {

class EngineCVI final : public Engine {
public:
    EngineCVI();
    ~EngineCVI() override;

    ma_err_t init() override;
    ma_err_t init(size_t size) override;
    ma_err_t init(void* pool, size_t size) override;

    ma_err_t run() override;

    ma_err_t load(const void* model_data, size_t model_size) override;
#if MA_USE_FILESYSTEM
    ma_err_t load(const char* model_path) override;
    ma_err_t load(const std::string& model_path) override;
#endif

    int32_t getInputSize() override;
    int32_t getOutputSize() override;
    ma_tensor_t getInput(int32_t index) override;
    ma_tensor_t getOutput(int32_t index) override;
    ma_shape_t getInputShape(int32_t index) override;
    ma_shape_t getOutputShape(int32_t index) override;
    ma_quant_param_t getInputQuantParam(int32_t index) override;
    ma_quant_param_t getOutputQuantParam(int32_t index) override;

    ma_err_t setInput(int32_t index, const ma_tensor_t& tensor) override;

    int32_t getInputNum(const char* name) override;
    int32_t getOutputNum(const char* name) override;

private:
    CVI_MODEL_HANDLE model;
    CVI_TENSOR* input_tensors;
    CVI_TENSOR* output_tensors;
    int32_t input_num;
    int32_t output_num;
};

}  // namespace ma

#endif

#endif  // _MA_ENGINE_CVINN_H_