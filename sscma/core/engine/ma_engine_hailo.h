#ifndef _MA_ENGINE_HAILO_H_
#define _MA_ENGINE_HAILO_H_

#include "../ma_common.h"

#if MA_USE_ENGINE_HAILO

#include "ma_engine_base.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <hailo/hailort.hpp>

namespace ma::engine {

using namespace std;
using namespace hailort;

class EngineHailo final : public Engine {
public:
    using ExternalHandler = function<ma_err_t(int, void*, size_t)>;

    EngineHailo();
    ~EngineHailo() override;

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

private:
    shared_ptr<VDevice> _vdevice;
    shared_ptr<InferModel> _model;
    shared_ptr<ConfiguredInferModel> _configured_model;
    shared_ptr<ConfiguredInferModel::Bindings> _bindings;

    unordered_map<string, shared_ptr<ma_tensor_t>> _io_buffers;
    unordered_map<string, shared_ptr<ExternalHandler>> _external_handlers;

    vector<shared_ptr<ma_tensor_t>> _input_tensors;
    vector<shared_ptr<ma_tensor_t>> _output_tensors;
};

}  // namespace ma::engine

#endif

#endif