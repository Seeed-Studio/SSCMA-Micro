#include "ma_engine_cvi.h"

#if MA_USE_ENGINE_CVI

#include <dirent.h>

namespace ma::engine {

constexpr char TAG[] = "ma::engine::cvinn";


static const ma_tensor_type_t mapped_tensor_types[] = {
    MA_TENSOR_TYPE_F32,
    MA_TENSOR_TYPE_S32,
    MA_TENSOR_TYPE_U32,
    MA_TENSOR_TYPE_BF16,
    MA_TENSOR_TYPE_S16,
    MA_TENSOR_TYPE_U16,
    MA_TENSOR_TYPE_S8,
    MA_TENSOR_TYPE_U8,
};

EngineCVI::EngineCVI() {
    model = nullptr;
}

EngineCVI::~EngineCVI() {
    if (model != nullptr) {
        CVI_NN_CleanupModel(model);
    }
}

ma_err_t EngineCVI::init() {
    return MA_OK;
}

ma_err_t EngineCVI::init(size_t size) {

    return MA_OK;
}

ma_err_t EngineCVI::init(void* pool, size_t size) {

    return MA_OK;
}

ma_err_t EngineCVI::load(const char* model_path) {
    ma_err_t ret = MA_OK;
    CVI_RC _ret  = CVI_RC_SUCCESS;

    if (model != nullptr) {
        CVI_NN_CleanupModel(model);
        model = nullptr;
    }

    _ret == CVI_NN_RegisterModel(model_path, &model);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }

    _ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors, &output_num);


    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }
    return MA_OK;
}


ma_err_t EngineCVI::load(const std::string& model_path) {
    return load(model_path.c_str());
}

ma_err_t EngineCVI::load(const void* model_data, size_t model_size) {
    ma_err_t ret = MA_OK;
    CVI_RC _ret  = CVI_RC_SUCCESS;

    if (model != nullptr) {
        CVI_NN_CleanupModel(model);
        model = nullptr;
    }

    _ret == CVI_NN_RegisterModelFromBuffer(static_cast<const int8_t*>(model_data), model_size, &model);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }

    _ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors, &output_num);

    if (_ret != CVI_RC_SUCCESS) {
        return MA_EINVAL;
    }
    return MA_OK;
}

ma_err_t EngineCVI::run() {
    MA_ASSERT(model != nullptr);
    ma_err_t ret = MA_OK;

    if (CVI_RC_SUCCESS != CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num)) {
        ret = MA_EINVAL;
    }

    return ret;
}


ma_tensor_t EngineCVI::getInput(int32_t index) {
    MA_ASSERT(model != nullptr);
    ma_tensor_t tensor{0};
    if (index >= input_num) {
        return tensor;
    }
    tensor.data.data   = CVI_NN_TensorPtr(&input_tensors[index]);
    tensor.type        = mapped_tensor_types[input_tensors[index].fmt];
    tensor.size        = CVI_NN_TensorSize(&input_tensors[index]);
    tensor.name        = CVI_NN_TensorName(&input_tensors[index]);
    tensor.shape       = getInputShape(index);
    tensor.quant_param = getInputQuantParam(index);
    tensor.is_variable = true;

    return tensor;
}


ma_tensor_t EngineCVI::getOutput(int32_t index) {
    MA_ASSERT(model != nullptr);
    ma_tensor_t tensor{0};
    if (index >= output_num) {
        return tensor;
    }
    tensor.data.data   = CVI_NN_TensorPtr(&output_tensors[index]);
    tensor.type        = mapped_tensor_types[output_tensors[index].fmt];
    tensor.size        = CVI_NN_TensorSize(&output_tensors[index]);
    tensor.name        = CVI_NN_TensorName(&output_tensors[index]);
    tensor.shape       = getOutputShape(index);
    tensor.quant_param = getOutputQuantParam(index);
    tensor.is_variable = false;

    return tensor;
}

ma_shape_t EngineCVI::getInputShape(int32_t index) {
    MA_ASSERT(model != nullptr);
    ma_shape_t shape{0};
    if (index >= input_num) {
        return shape;
    }

    CVI_SHAPE input_shape = CVI_NN_TensorShape(&input_tensors[index]);
    shape.size            = input_shape.dim_size;
    MA_ASSERT(shape.size < MA_ENGINE_SHAPE_MAX_DIM);
    for (int i = 0; i < shape.size; i++) {
        shape.dims[i] = input_shape.dim[i];
    }
    return shape;
}


ma_shape_t EngineCVI::getOutputShape(int32_t index) {
    MA_ASSERT(model != nullptr);
    ma_shape_t shape{0};
    if (index >= output_num) {
        return shape;
    }
    CVI_SHAPE output_shape = CVI_NN_TensorShape(&output_tensors[index]);
    shape.size             = output_shape.dim_size;
    MA_ASSERT(shape.size < MA_ENGINE_SHAPE_MAX_DIM);
    for (int i = 0; i < shape.size; i++) {
        shape.dims[i] = output_shape.dim[i];
    }
    return shape;
}


ma_quant_param_t EngineCVI::getInputQuantParam(int32_t index) {
    MA_ASSERT(model != nullptr);
    ma_quant_param_t quant_param{0};
    if (index >= input_num) {
        return quant_param;
    }
    quant_param.scale      = CVI_NN_TensorQuantScale(&input_tensors[index]);
    quant_param.zero_point = CVI_NN_TensorQuantZeroPoint(&input_tensors[index]);
    return quant_param;
}


ma_quant_param_t EngineCVI::getOutputQuantParam(int32_t index) {
    MA_ASSERT(model != nullptr);
    ma_quant_param_t quant_param{0};
    if (index >= output_num) {
        return quant_param;
    }
    quant_param.scale      = CVI_NN_TensorQuantScale(&output_tensors[index]);
    quant_param.zero_point = CVI_NN_TensorQuantZeroPoint(&output_tensors[index]);
    return quant_param;
}

int32_t EngineCVI::getInputSize() {
    return input_num;
}
int32_t EngineCVI::getOutputSize() {
    return output_num;
}

int32_t EngineCVI::getInputNum(const char* name) {
    MA_ASSERT(model != nullptr);
    for (int32_t i = 0; i < input_num; i++) {
        if (strcmp(CVI_NN_TensorName(&input_tensors[i]), name) == 0) {
            return i;
        }
    }
    return -1;
}
int32_t EngineCVI::getOutputNum(const char* name) {
    MA_ASSERT(model != nullptr);
    for (int32_t i = 0; i < output_num; i++) {
        if (strcmp(CVI_NN_TensorName(&output_tensors[i]), name) == 0) {
            return i;
        }
    }
    return -1;
}


ma_err_t EngineCVI::setInput(int32_t index, const ma_tensor_t& tensor) {
    MA_ASSERT(model != nullptr);
    if (index >= input_num) {
        return MA_EINVAL;
    }
    if (tensor.is_physical) {
        CVI_NN_SetTensorPhysicalAddr(&input_tensors[index], reinterpret_cast<uint64_t>(tensor.data.data));
    } else {
        CVI_NN_SetTensorPtr(&input_tensors[index], tensor.data.data);
    }
    return MA_OK;
}


}  // namespace ma::engine

#endif