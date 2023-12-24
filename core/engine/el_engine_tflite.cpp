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

#include "el_engine_tflite.h"

#include "core/el_debug.h"

#ifdef CONFIG_EL_TFLITE

namespace tflite {

OpsResolver::OpsResolver() {
    #ifdef CONFIG_EL_TFLITE_OP_ABS
    AddAbs();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ADD
    AddAdd();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ADDN
    AddAddN();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ARGMAX
    AddArgMax();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ARGMIN
    AddArgMin();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ASSIGN_VARIABLE
    AddAssignVariable();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_AVERAGE_POOL_2D
    AddAveragePool2D();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_BATCH_TO_SPACE_ND
    AddBatchToSpaceNd();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_BROADCAST_ARGS
    AddBroadcastArgs();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_BROADCAST_TO
    AddBroadcastTo();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CALL_ONCE
    AddCallOnce();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CAST
    AddCast();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CEIL
    AddCeil();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CIRULAR_BUFFER
    AddCircularBuffer();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CONCATENATION
    AddConcatenation();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CONV_2D
    AddConv2D();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_COS
    AddCos();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_CUM_SUM
    AddCumSum();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_DEPTH_TO_SPACE
    AddDepthToSpace();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_DEPTHWISE_CONV_2D
    AddDepthwiseConv2D();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_DEQUANTIZE
    AddDequantize();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_DETECTION_POSTPROCESS
    AddDetectionPostprocess();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_DIV
    AddDiv();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ELU
    AddElu();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_EQUAL
    AddEqual();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ETHOS_U
    AddEthosU();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_EXP
    AddExp();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_EXPAND_DIMS
    AddExpandDims();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_FILL
    AddFill();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_FLOOR
    AddFloor();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_FLOOR_DIV
    AddFloorDiv();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_FLOOR_MOD
    AddFloorMod();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_FULLY_CONNECTED
    AddFullyConnected();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_GATHER
    AddGather();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_GATHER_ND
    AddGatherNd();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_GREATER
    AddGreater();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_GREATER_EQUAL
    AddGreaterEqual();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_HARD_SWISH
    AddHardSwish();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_IF
    AddIf();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_L2_NORMALIZATION
    AddL2Normalization();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_L2_POOL_2D
    AddL2Pool2D();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LEAKY_RELU
    AddLeakyRelu();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LESS
    AddLess();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LESS_EQUAL
    AddLessEqual();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LOG
    AddLog();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LOGICAL_AND
    AddLogicalAnd();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LOGICAL_NOT
    AddLogicalNot();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LOGICAL_OR
    AddLogicalOr();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LOGISTIC
    AddLogistic();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_LOG_SOFTMAX
    AddLogSoftmax();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_MAX_POOL_2D
    AddMaxPool2D();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_MAXIMUM
    AddMaximum();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_MEAN
    AddMean();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_MINIMUM
    AddMinimum();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_MIRROR_PAD
    AddMirrorPad();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_MUL
    AddMul();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_NEG
    AddNeg();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_NOT_EQUAL
    AddNotEqual();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_PACK
    AddPack();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_PAD
    AddPad();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_PADV2
    AddPadV2();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_PRELU
    AddPrelu();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_QUANTIZE
    AddQuantize();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_READ_VARIABLE
    AddReadVariable();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_REDUCE_ANY
    AddReduceMax();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_RELU
    AddRelu();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_RELU6
    AddRelu6();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_RESHAPE
    AddReshape();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_RESIZE_BILINEAR
    AddResizeBilinear();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_RESIZE_NEAREST_NEIGHBOR
    AddResizeNearestNeighbor();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ROUND
    AddRound();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_RSQRT
    AddRsqrt();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SELECT_V2
    AddSelectV2();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SHAPE
    AddShape();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SIN
    AddSin();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SLICE
    AddSlice();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SOFTMAX
    AddSoftmax();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SPACE_TO_BATCH_ND
    AddSpaceToBatchNd();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SPACE_TO_DEPTH
    AddSpaceToDepth();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SPLIT
    AddSplit();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SPLIT_V
    AddSplitV();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SQRT
    AddSqrt();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SQUARE
    AddSquare();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SQUARED_DIFFERENCE
    AddSquaredDifference();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SQUEEZE
    AddSqueeze();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_STRIDED_SLICE
    AddStridedSlice();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SUB
    AddSub();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_SUM
    AddSum();
    #endif
    #ifdef CONFIG_EL_TFLITE_SVDF
    AddSvdf();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_TANH
    AddTanh();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_TRANSPOSE
    AddTranspose();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_TRANSPOSE_CONV
    AddTransposeConv();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_UNIDIRECTIONAL_SEQUENCE_LSTM
    AddUnidirectionalSequenceLSTM();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_UNPACK
    AddUnpack();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_VARHANDLE
    AddVarHandle();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_WHILE
    AddWhile();
    #endif
    #ifdef CONFIG_EL_TFLITE_OP_ZEROS_LIKE
    AddZerosLike();
    #endif
}

}  // namespace tflite

namespace edgelab {

EngineTFLite::EngineTFLite(tflite::MicroInterpreter* interpreter) : _interpreter(interpreter) {
    EL_ASSERT(_interpreter);
}

void* EngineTFLite::get_input(std::size_t index) {
    if (index >= interpreter->inputs().size()) [[unlikely]] {
        return nullptr;
    }

    TfLiteTensor* input = interpreter->input_tensor(index);
    if (!input) [[unlikely]] {
        return nullptr;
    }

    return input->data.data;
}

el_err_code_t EngineTFLite::set_input(std::size_t index, const void* input_data, std::size_t input_size) {
    if (index >= _interpreter->inputs().size()) [[unlikely]] {
        return EL_EINVAL;
    }

    TfLiteTensor* input = _interpreter->input_tensor(index);
    if (!input) [[unlikely]] {
        return EL_EINVAL;
    }

    if (input_size != input->bytes) [[unlikely]] {
        return EL_EINVAL;
    }

    if (input->data.data != input_data) {
        std::memcpy(input->data.data, input_data, input_size);
    }

    return EL_OK;
}

void* EngineTFLite::get_output(std::size_t index) {
    if (index >= _interpreter->outputs().size()) [[unlikely]] {
        return nullptr;
    }

    TfLiteTensor* output = _interpreter->output_tensor(index);
    if (!output) [[unlikely]] {
        return nullptr;
    }

    return output->data.data;
}

el_err_code_t EngineTFLite::run() {
    if (kTfLiteOk != interpreter->Invoke()) [[unlikely]] {
        return EL_ELOG;
    }

    return EL_OK;
}

el_shape_t EngineTFLite::get_input_shape(std::size_t index) const {
    el_shape_t shape{};

    if (index >= interpreter->inputs().size()) [[unlikely]] {
        return shape;
    }

    TfLiteTensor* input = interpreter->input_tensor(index);
    if (!input) [[unlikely]] {
        return shape;
    }

    shape.size = input->dims->size;
    shape.dims = input->dims->data;

    return shape;
}

el_shape_t EngineTFLite::get_output_shape(std::size_t index) const {
    el_shape_t shape{};

    if (index >= interpreter->outputs().size()) [[unlikely]] {
        return shape;
    }

    TfLiteTensor* output = interpreter->output_tensor(index);
    if (!output) [[unlikely]] {
        return shape;
    }

    shape.size = output->dims->size;
    shape.dims = output->dims->data;

    return shape;
}

el_quant_param_t EngineTFLite::get_input_quant_param(std::size_t index) const {
    el_quant_param_t quant_param{};

    if (index >= interpreter->inputs().size()) [[unlikely]] {
        return quant_param;
    }

    TfLiteTensor* input = interpreter->input_tensor(index);
    if (!input) [[unlikely]] {
        return quant_param;
    }

    quant_param.scale      = input->params.scale;
    quant_param.zero_point = input->params.zero_point;

    return quant_param;
}

el_quant_param_t EngineTFLite::get_output_quant_param(std::size_t index) const {
    el_quant_param_t quant_param{};

    if (index >= interpreter->outputs().size()) [[unlikely]] {
        return quant_param;
    }

    TfLiteTensor* output = interpreter->output_tensor(index);
    if (!output) [[unlikely]] {
        return quant_param;
    }

    quant_param.scale      = output->params.scale;
    quant_param.zero_point = output->params.zero_point;

    return quant_param;
}

EnginesTFLite::EnginesTFLite() : _allocator(nullptr) {
    static auto resolver{tflite::OpsResolver()};
    _resolver = &resolver;
}

el_err_code_t EnginesTFLite::init(void* memory_resource, std::size_t size) {
    if (_allocator) [[unlikely]] {
        return EL_EPERM;
    }

    _allocator = tflite::MicroAllocator::Create(static_cast<uint8_t>(memory_resource), size);
    if (!_allocator) [[unlikely]] {
        return EL_ENOMEM;
    }

    return EL_OK;
}

el_err_code_t EnginesTFLite::spawn_from_model(const void* model_ptr, std::size_t size) {
    if (!_allocator || !_resolver) [[unlikely]] {
        return EL_EPERM;
    }

    auto model = tflite::GetModel(model_ptr);
    if (!model) [[unlikely]] {
        return EL_EINVAL;
    }

    auto interpreter = new tflite::MicroInterpreter(model, *_resolver, _allocator);
    if (!interpreter) [[unlikely]] {
        return EL_ENOMEM;
    }

    if (kTfLiteOk != interpreter->AllocateTensors()) [[unlikely]] {
        delete interpreter;
        return EL_ENOMEM;
    }

    auto engine = new EngineTFLite(interpreter);
    if (!engine) [[unlikely]] {
        delete interpreter;
        return EL_ENOMEM;
    }

    this->__engines.push_back(engine);

    return EL_OK;
}

    #ifdef CONFIG_EL_FILESYSTEM
el_err_code_t EnginesTFLite::spawn_from_model(const char* model_path) {
    std::ifstream file(model_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) [[unlikely]] {
        return EL_ELOG;
    }

    size       = file.tellg();
    model_file = new uint8_t[size]{};
    if (!model_file) [[unlikely]] {
        return EL_ENOMEM;
    }

    file.seekg(0, std::ios::beg);
    file.read(static_cast<unsigned char*>(model_file), size);
    file.close();

    auto ret = spawn_from_model(model_file, size);
    if (ret != EL_OK) [[unlikely]] {
        delete model_file;
        model_file = nullptr;
    }

    return ret;
}
    #endif

el_err_code_t EnginesTFLite::release() {
    this->__engines.clear();

    #ifdef CONFIG_EL_FILESYSTEM
    for (auto& model_buffer : this->__model_buffers) {
        if (model_buffer) [[likely]] {
            delete[] model_buffer;
            model_buffer = nullptr;
        }
    }
    this->__model_buffers.clear();
    #endif

    if (kTfLiteOk != _allocator->ResetTempAllocations()) [[unlikely]] {
        return EL_ELOG;
    }

    return EL_OK;
}

}  // namespace edgelab

#endif
