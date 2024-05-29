#include "ma_engine_cvinn.h"

#include "core/ma_debug.h"

#if MA_USE_ENGINE_CVINN

namespace ma::engine {

const static char*     TAG = "ma::engine::cvinn";

const ma_tensor_type_t mapped_tensor_types[] = {
    MA_TENSOR_TYPE_F32,
    MA_TENSOR_TYPE_S32,
    MA_TENSOR_TYPE_U32,
    MA_TENSOR_TYPE_BF16,
    MA_TENSOR_TYPE_S16,
    MA_TENSOR_TYPE_U16,
    MA_TENSOR_TYPE_S8,
    MA_TENSOR_TYPE_U8,
};

EngineCVINN::EngineCVINN() {
    model = nullptr;
}

EngineCVINN::~EngineCVINN() {
    CVI_NN_CleanupModel(model);
}

ma_err_t EngineCVINN::init() {

    return MA_OK;
}

ma_err_t EngineCVINN::init(size_t size) {

    return MA_OK;
}

ma_err_t EngineCVINN::init(void* pool, size_t size) {

    return MA_OK;
}

ma_err_t EngineCVINN::load_model(const char* model_path) {
    ma_err_t ret  = MA_OK;
    CVI_RC   _ret = CVI_RC_SUCCESS;
    _ret == CVI_NN_RegisterModel(model_path, &model);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }

    _ret = CVI_NN_GetInputOutputTensors(
        model, &input_tensors, &input_num, &output_tensors, &output_num);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }
    return MA_OK;
}

ma_err_t EngineCVINN::load_model(const void* model_data, size_t model_size) {
    ma_err_t ret  = MA_OK;
    CVI_RC   _ret = CVI_RC_SUCCESS;

    _ret ==
        CVI_NN_RegisterModelFromBuffer(static_cast<const int8_t*>(model_data), model_size, &model);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }

    _ret = CVI_NN_GetInputOutputTensors(
        model, &input_tensors, &input_num, &output_tensors, &output_num);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }
    return MA_OK;
}


ma_err_t EngineCVINN::run() {
    MA_ASSERT(model != nullptr);
    ma_err_t ret = MA_OK;

    if (CVI_RC_SUCCESS !=
        CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num)) {
        ret = MA_EINVAL;
    }

    return ret;
}


ma_tensor_t EngineCVINN::get_input(const char* name) {
    MA_ASSERT(model != nullptr);
    ma_tensor_t tensor{0};
    CVI_TENSOR* input_tensor = CVI_NN_GetTensorByName(name, input_tensors, input_num);
    if (input_tensor != nullptr) {
        tensor.data.data   = CVI_NN_TensorPtr(input_tensor);
        tensor.type        = mapped_tensor_types[input_tensor->fmt];
        tensor.size        = CVI_NN_TensorSize(input_tensor);
        tensor.name        = CVI_NN_TensorName(input_tensor);
        tensor.shape       = get_input_shape(name);
        tensor.quant_param = get_input_quant_param(name);
        tensor.is_variable = true;
    }
    return tensor;
}


ma_tensor_t EngineCVINN::get_output(const char* name) {
    MA_ASSERT(model != nullptr);
    ma_tensor_t tensor{0};
    CVI_TENSOR* output_tensor = CVI_NN_GetTensorByName(name, output_tensors, output_num);
    if (output_tensor != nullptr) {
        tensor.data.data   = CVI_NN_TensorPtr(output_tensor);
        tensor.type        = mapped_tensor_types[output_tensor->fmt];
        tensor.size        = CVI_NN_TensorSize(output_tensor);
        tensor.name        = CVI_NN_TensorName(output_tensor);
        tensor.shape       = get_output_shape(name);
        tensor.quant_param = get_output_quant_param(name);
        tensor.is_variable = false;
    }
    return tensor;
}

ma_shape_t EngineCVINN::get_input_shape(const char* name) {
    MA_ASSERT(model != nullptr);
    ma_shape_t  shape{0};
    CVI_TENSOR* input_tensor = CVI_NN_GetTensorByName(name, input_tensors, input_num);
    if (input_tensor == nullptr) {
        return shape;
    }
    CVI_SHAPE input_shape = CVI_NN_TensorShape(input_tensors);
    shape.size            = input_shape.dim_size;
    MA_ASSERT(shape.size < MA_ENGINE_SHAPE_MAX_DIM);
    for (int i = 0; i < shape.size; i++) {
        shape.dims[i] = input_shape.dim[i];
    }
    return shape;
}


ma_shape_t EngineCVINN::get_output_shape(const char* name) {
    MA_ASSERT(model != nullptr);
    ma_shape_t  shape{0};
    CVI_TENSOR* output_tensor = CVI_NN_GetTensorByName(name, output_tensors, output_num);
    if (output_tensor == nullptr) {
        return shape;
    }
    CVI_SHAPE output_shape = CVI_NN_TensorShape(output_tensors);
    shape.size             = output_shape.dim_size;
    MA_ASSERT(shape.size < MA_ENGINE_SHAPE_MAX_DIM);
    for (int i = 0; i < shape.size; i++) {
        shape.dims[i] = output_shape.dim[i];
    }
    return shape;
}


ma_quant_param_t EngineCVINN::get_input_quant_param(const char* name) {
    MA_ASSERT(model != nullptr);
    ma_quant_param_t quant_param{0};
    CVI_TENSOR*      input_tensor = CVI_NN_GetTensorByName(name, input_tensors, input_num);
    if (input_tensor == nullptr) {
        return quant_param;
    }
    quant_param.scale      = CVI_NN_TensorQuantScale(input_tensor);
    quant_param.zero_point = CVI_NN_TensorQuantZeroPoint(input_tensor);
    return quant_param;
}


ma_quant_param_t EngineCVINN::get_output_quant_param(const char* name) {
    MA_ASSERT(model != nullptr);
    ma_quant_param_t quant_param{0};
    CVI_TENSOR*      output_tensor = CVI_NN_GetTensorByName(name, output_tensors, output_num);
    if (output_tensor == nullptr) {
        return quant_param;
    }
    quant_param.scale      = CVI_NN_TensorQuantScale(output_tensor);
    quant_param.zero_point = CVI_NN_TensorQuantZeroPoint(output_tensor);
    return quant_param;
}

size_t EngineCVINN::get_input_size() {

    return input_num;
}
size_t EngineCVINN::get_output_size() {
    return output_num;
}

}  // namespace ma::engine

#endif