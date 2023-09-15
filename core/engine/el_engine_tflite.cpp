/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
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
    #ifdef CONFIG_EL_TFLITE_ETHOS_U
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
    AddIf();
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

tflite::OpsResolver EngineTFLite::resolver;

EngineTFLite::EngineTFLite() {
    interpreter      = nullptr;
    model            = nullptr;
    memory_pool.pool = nullptr;
    memory_pool.size = 0;
    #ifdef CONFIG_EL_FILESYSTEM
    model_file = nullptr;
    #endif
}

EngineTFLite::~EngineTFLite() {
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
    }
    if (memory_pool.pool != nullptr) {
        delete[] static_cast<uint8_t*>(memory_pool.pool);
        memory_pool.pool = nullptr;
    }
    #ifdef CONFIG_EL_FILESYSTEM
    if (model_file != nullptr) {
        delete model_file;
        model_file = nullptr;
    }
    #endif
}

el_err_code_t EngineTFLite::init() { return EL_OK; }

el_err_code_t EngineTFLite::init(size_t size) {
    void* pool = new uint8_t[size];
    if (pool == nullptr) {
        return EL_ENOMEM;
    }
    memory_pool.pool = pool;
    memory_pool.size = size;
    return init();
}

el_err_code_t EngineTFLite::init(void* pool, size_t size) {
    memory_pool.pool = pool;
    memory_pool.size = size;
    return init();
}

el_err_code_t EngineTFLite::run() {
    EL_ASSERT(interpreter != nullptr);

    if (kTfLiteOk != interpreter->Invoke()) {
        return EL_ELOG;
    }
    return EL_OK;
}

el_err_code_t EngineTFLite::load_model(const void* model_data, size_t model_size) {
    model = tflite::GetModel(model_data);
    if (model == nullptr) {
        return EL_EINVAL;
    }

    interpreter =
      new tflite::MicroInterpreter(model, resolver, static_cast<uint8_t*>(memory_pool.pool), memory_pool.size);
    if (interpreter == nullptr) {
        return EL_ENOMEM;
    }
    if (kTfLiteOk != interpreter->AllocateTensors()) {
        delete interpreter;
        interpreter = nullptr;
        return EL_ELOG;
    }
    return EL_OK;
}

el_err_code_t EngineTFLite::set_input(size_t index, const void* input_data, size_t input_size) {
    EL_ASSERT(interpreter != nullptr);

    if (index >= interpreter->inputs().size()) {
        return EL_EINVAL;
    }
    TfLiteTensor* input = interpreter->input_tensor(index);
    if (input == nullptr) {
        return EL_EINVAL;
    }
    if (input_size != input->bytes) {
        return EL_EINVAL;
    }
    memcpy(input->data.data, input_data, input_size);
    return EL_OK;
}

void* EngineTFLite::get_input(size_t index) {
    EL_ASSERT(interpreter != nullptr);

    if (index >= interpreter->inputs().size()) {
        return nullptr;
    }
    TfLiteTensor* input = interpreter->input_tensor(index);
    if (input == nullptr) {
        return nullptr;
    }
    return input->data.data;
}

void* EngineTFLite::get_output(size_t index) {
    EL_ASSERT(interpreter != nullptr);

    if (index >= interpreter->outputs().size()) {
        return nullptr;
    }
    TfLiteTensor* output = interpreter->output_tensor(index);
    if (output == nullptr) {
        return nullptr;
    }
    return output->data.data;
}

el_shape_t EngineTFLite::get_input_shape(size_t index) const {
    el_shape_t shape;

    EL_ASSERT(interpreter != nullptr);

    shape.size = 0;
    if (index >= interpreter->inputs().size()) {
        return shape;
    }
    TfLiteTensor* input = interpreter->input_tensor(index);
    if (input == nullptr) {
        return shape;
    }
    shape.size = input->dims->size;
    shape.dims = input->dims->data;
    return shape;
}

el_shape_t EngineTFLite::get_output_shape(size_t index) const {
    el_shape_t shape;
    shape.size = 0;

    EL_ASSERT(interpreter != nullptr);

    if (index >= interpreter->outputs().size()) {
        return shape;
    }
    TfLiteTensor* output = interpreter->output_tensor(index);
    if (output == nullptr) {
        return shape;
    }
    shape.size = output->dims->size;
    shape.dims = output->dims->data;
    return shape;
}

el_quant_param_t EngineTFLite::get_input_quant_param(size_t index) const {
    el_quant_param_t quant_param;
    quant_param.scale      = 0;
    quant_param.zero_point = 0;

    EL_ASSERT(interpreter != nullptr);

    if (index >= interpreter->inputs().size()) {
        return quant_param;
    }
    TfLiteTensor* input = interpreter->input_tensor(index);
    if (input == nullptr) {
        return quant_param;
    }
    quant_param.scale      = input->params.scale;
    quant_param.zero_point = input->params.zero_point;
    return quant_param;
}

el_quant_param_t EngineTFLite::get_output_quant_param(size_t index) const {
    el_quant_param_t quant_param;
    quant_param.scale      = 0;
    quant_param.zero_point = 0;

    EL_ASSERT(interpreter != nullptr);

    if (index >= interpreter->outputs().size()) {
        return quant_param;
    }
    TfLiteTensor* output = interpreter->output_tensor(index);
    if (output == nullptr) {
        return quant_param;
    }
    quant_param.scale      = output->params.scale;
    quant_param.zero_point = output->params.zero_point;
    return quant_param;
}

    #ifdef CONFIG_EL_FILESYSTEM
el_err_code_t EngineTFLite::load_model(const char* model_path) {
    el_err_code_t ret  = EL_OK;
    size_t        size = 0;
    std::ifstream file(model_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return EL_ELOG;
    }
    size       = file.tellg();
    model_file = new char[size];
    if (model_file == nullptr) {
        return EL_ENOMEM;
    }
    file.seekg(0, std::ios::beg);
    file.read(static_cast<char*>(model_file), size);
    file.close();
    ret = load_model(model_file, size);
    if (ret != EL_OK) {
        delete model_file;
        model_file = nullptr;
    }

    return ret;
}
    #endif

}  // namespace edgelab

#endif
