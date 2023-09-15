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

#ifndef _EL_ENGINE_TFLITE_H_
#define _EL_ENGINE_TFLITE_H_

#include <tensorflow/lite/core/c/common.h>
#include <tensorflow/lite/micro/compatibility.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include <cstddef>
#include <cstdint>

#include "core/el_types.h"
#include "el_engine_base.h"

#define TF_LITE_SATTIC_MEMORY

namespace tflite {

enum OpsCount : unsigned int {
    OpsHead = 0,
#ifdef CONFIG_EL_TFLITE_OP_ABS
    AddAbs,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ADD
    AddAdd,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ADDN
    AddAddN,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ARGMAX
    AddArgMax,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ARGMIN
    AddArgMin,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ASSIGN_VARIABLE
    AddAssignVariable,
#endif
#ifdef CONFIG_EL_TFLITE_OP_AVERAGE_POOL_2D
    AddAveragePool2D,
#endif
#ifdef CONFIG_EL_TFLITE_OP_BATCH_TO_SPACE_ND
    AddBatchToSpaceNd,
#endif
#ifdef CONFIG_EL_TFLITE_OP_BROADCAST_ARGS
    AddBroadcastArgs,
#endif
#ifdef CONFIG_EL_TFLITE_OP_BROADCAST_TO
    AddBroadcastTo,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CALL_ONCE
    AddCallOnce,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CAST
    AddCast,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CEIL
    AddCeil,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CIRULAR_BUFFER
    AddCircularBuffer,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CONCATENATION
    AddConcatenation,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CONV_2D
    AddConv2D,
#endif
#ifdef CONFIG_EL_TFLITE_OP_COS
    AddCos,
#endif
#ifdef CONFIG_EL_TFLITE_OP_CUM_SUM
    AddCumSum,
#endif
#ifdef CONFIG_EL_TFLITE_OP_DEPTH_TO_SPACE
    AddDepthToSpace,
#endif
#ifdef CONFIG_EL_TFLITE_OP_DEPTHWISE_CONV_2D
    AddDepthwiseConv2D,
#endif
#ifdef CONFIG_EL_TFLITE_OP_DEQUANTIZE
    AddDequantize,
#endif
#ifdef CONFIG_EL_TFLITE_OP_DETECTION_POSTPROCESS
    AddDetectionPostprocess,
#endif
#ifdef CONFIG_EL_TFLITE_OP_DIV
    AddDiv,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ELU
    AddElu,
#endif
#ifdef CONFIG_EL_TFLITE_OP_EQUAL
    AddEqual,
#endif
#ifdef CONFIG_EL_TFLITE_ETHOS_U
    AddEthosU,
#endif
#ifdef CONFIG_EL_TFLITE_OP_EXP
    AddExp,
#endif
#ifdef CONFIG_EL_TFLITE_OP_EXPAND_DIMS
    AddExpandDims,
#endif
#ifdef CONFIG_EL_TFLITE_OP_FILL
    AddFill,
#endif
#ifdef CONFIG_EL_TFLITE_OP_FLOOR
    AddFloor,
#endif
#ifdef CONFIG_EL_TFLITE_OP_FLOOR_DIV
    AddFloorDiv,
#endif
#ifdef CONFIG_EL_TFLITE_OP_FLOOR_MOD
    AddFloorMod,
#endif
#ifdef CONFIG_EL_TFLITE_OP_FULLY_CONNECTED
    AddFullyConnected,
#endif
#ifdef CONFIG_EL_TFLITE_OP_GATHER
    AddGather,
#endif
#ifdef CONFIG_EL_TFLITE_OP_GATHER_ND
    AddGatherNd,
#endif
#ifdef CONFIG_EL_TFLITE_OP_GREATER
    AddGreater,
#endif
#ifdef CONFIG_EL_TFLITE_OP_GREATER_EQUAL
    AddGreaterEqual,
#endif
#ifdef CONFIG_EL_TFLITE_OP_HARD_SWISH
    AddHardSwish,
#endif
    AddIf,
#ifdef CONFIG_EL_TFLITE_OP_L2_NORMALIZATION
    AddL2Normalization,
#endif
#ifdef CONFIG_EL_TFLITE_OP_L2_POOL_2D
    AddL2Pool2D,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LEAKY_RELU
    AddLeakyRelu,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LESS
    AddLess,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LESS_EQUAL
    AddLessEqual,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LOG
    AddLog,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LOGICAL_AND
    AddLogicalAnd,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LOGICAL_NOT
    AddLogicalNot,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LOGICAL_OR
    AddLogicalOr,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LOGISTIC
    AddLogistic,
#endif
#ifdef CONFIG_EL_TFLITE_OP_LOG_SOFTMAX
    AddLogSoftmax,
#endif
#ifdef CONFIG_EL_TFLITE_OP_MAX_POOL_2D
    AddMaxPool2D,
#endif
#ifdef CONFIG_EL_TFLITE_OP_MAXIMUM
    AddMaximum,
#endif
#ifdef CONFIG_EL_TFLITE_OP_MEAN
    AddMean,
#endif
#ifdef CONFIG_EL_TFLITE_OP_MINIMUM
    AddMinimum,
#endif
#ifdef CONFIG_EL_TFLITE_OP_MIRROR_PAD
    AddMirrorPad,
#endif
#ifdef CONFIG_EL_TFLITE_OP_MUL
    AddMul,
#endif
#ifdef CONFIG_EL_TFLITE_OP_NEG
    AddNeg,
#endif
#ifdef CONFIG_EL_TFLITE_OP_NOT_EQUAL
    AddNotEqual,
#endif
#ifdef CONFIG_EL_TFLITE_OP_PACK
    AddPack,
#endif
#ifdef CONFIG_EL_TFLITE_OP_PAD
    AddPad,
#endif
#ifdef CONFIG_EL_TFLITE_OP_PADV2
    AddPadV2,
#endif
#ifdef CONFIG_EL_TFLITE_OP_PRELU
    AddPrelu,
#endif
#ifdef CONFIG_EL_TFLITE_OP_QUANTIZE
    AddQuantize,
#endif
#ifdef CONFIG_EL_TFLITE_OP_READ_VARIABLE
    AddReadVariable,
#endif
#ifdef CONFIG_EL_TFLITE_OP_REDUCE_ANY
    AddReduceMax,
#endif
#ifdef CONFIG_EL_TFLITE_OP_RELU
    AddRelu,
#endif
#ifdef CONFIG_EL_TFLITE_OP_RELU6
    AddRelu6,
#endif
#ifdef CONFIG_EL_TFLITE_OP_RESHAPE
    AddReshape,
#endif
#ifdef CONFIG_EL_TFLITE_OP_RESIZE_BILINEAR
    AddResizeBilinear,
#endif
#ifdef CONFIG_EL_TFLITE_OP_RESIZE_NEAREST_NEIGHBOR
    AddResizeNearestNeighbor,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ROUND
    AddRound,
#endif
#ifdef CONFIG_EL_TFLITE_OP_RSQRT
    AddRsqrt,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SELECT_V2
    AddSelectV2,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SHAPE
    AddShape,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SIN
    AddSin,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SLICE
    AddSlice,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SOFTMAX
    AddSoftmax,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SPACE_TO_BATCH_ND
    AddSpaceToBatchNd,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SPACE_TO_DEPTH
    AddSpaceToDepth,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SPLIT
    AddSplit,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SPLIT_V
    AddSplitV,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SQRT
    AddSqrt,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SQUARE
    AddSquare,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SQUARED_DIFFERENCE
    AddSquaredDifference,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SQUEEZE
    AddSqueeze,
#endif
#ifdef CONFIG_EL_TFLITE_OP_STRIDED_SLICE
    AddStridedSlice,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SUB
    AddSub,
#endif
#ifdef CONFIG_EL_TFLITE_OP_SUM
    AddSum,
#endif
#ifdef CONFIG_EL_TFLITE_SVDF
    AddSvdf,
#endif
#ifdef CONFIG_EL_TFLITE_OP_TANH
    AddTanh,
#endif
#ifdef CONFIG_EL_TFLITE_OP_TRANSPOSE
    AddTranspose,
#endif
#ifdef CONFIG_EL_TFLITE_OP_TRANSPOSE_CONV
    AddTransposeConv,
#endif
#ifdef CONFIG_EL_TFLITE_OP_UNIDIRECTIONAL_SEQUENCE_LSTM
    AddUnidirectionalSequenceLSTM,
#endif
#ifdef CONFIG_EL_TFLITE_OP_UNPACK
    AddUnpack,
#endif
#ifdef CONFIG_EL_TFLITE_OP_VARHANDLE
    AddVarHandle,
#endif
#ifdef CONFIG_EL_TFLITE_OP_WHILE
    AddWhile,
#endif
#ifdef CONFIG_EL_TFLITE_OP_ZEROS_LIKE
    AddZerosLike,
#endif
    OpsTail
};

class OpsResolver : public MicroMutableOpResolver<OpsCount::OpsTail - OpsCount::OpsHead> {
   public:
    OpsResolver();

   private:
    TF_LITE_REMOVE_VIRTUAL_DELETE
};

}  // namespace tflite

namespace edgelab {

class EngineTFLite : public base::Engine {
   public:
    EngineTFLite();
    ~EngineTFLite();

    el_err_code_t init() override;
    el_err_code_t init(size_t size) override;
    el_err_code_t init(void* pool, size_t size) override;

    el_err_code_t run() override;

#ifdef CONFIG_EL_FILESYSTEM
    el_err_code_t load_model(const char* model_path) override;
#endif

    el_err_code_t load_model(const void* model_data, size_t model_size) override;

    el_err_code_t set_input(size_t index, const void* input_data, size_t input_size) override;
    void*         get_input(size_t index) override;

    void* get_output(size_t index) override;

    el_shape_t       get_input_shape(size_t index) const override;
    el_shape_t       get_output_shape(size_t index) const override;
    el_quant_param_t get_input_quant_param(size_t index) const override;
    el_quant_param_t get_output_quant_param(size_t index) const override;

#ifdef CONFIG_EL_INFERENCER_TENSOR_NAME
    size_t           get_input_index(const char* input_name) const override;
    size_t           get_output_index(const char* output_name) const override;
    el_err_code_t    set_input(const char* input_name, const void* input_data, size_t input_size) override;
    void*            get_input(const char* input_name) override;
    void*            get_output(const char* output_name) override;
    el_shape_t       get_input_shape(const char* input_name) const override;
    el_shape_t       get_output_shape(const char* output_name) const override;
    el_quant_param_t get_input_quant_param(const char* input_name) const override;
    el_quant_param_t get_output_quant_param(const char* output_name) const override;
#endif

   private:
    static tflite::OpsResolver resolver;
    tflite::MicroInterpreter*  interpreter;
    const tflite::Model*       model;
    el_memory_pool_t           memory_pool;

#ifdef CONFIG_EL_FILESYSTEM
    const char* model_file;
#endif
};

}  // namespace edgelab

#endif
