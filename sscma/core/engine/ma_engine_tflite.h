#ifndef _MA_ENGINE_TFLITE_H_
#define _MA_ENGINE_TFLITE_H_

#include <cstddef>
#include <cstdint>

#include "../ma_common.h"

#if MA_USE_ENGINE_TFLITE

#include <tensorflow/lite/c/common.h>
#include <tensorflow/lite/micro/compatibility.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "ma_engine_base.h"

#if MA_USE_FILESYSTEM_POSIX
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace tflite {

enum OpsCount : unsigned int {
    OpsHead = 0,
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ABS
    AddAbs,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ADD
    AddAdd,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ADDN
    AddAddN,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ARGMAX
    AddArgMax,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ARGMIN
    AddArgMin,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ASSIGN_VARIABLE
    AddAssignVariable,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_AVERAGE_POOL_2D
    AddAveragePool2D,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_BATCH_TO_SPACE_ND
    AddBatchToSpaceNd,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_BROADCAST_ARGS
    AddBroadcastArgs,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_BROADCAST_TO
    AddBroadcastTo,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CALL_ONCE
    AddCallOnce,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CAST
    AddCast,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CEIL
    AddCeil,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CIRULAR_BUFFER
    AddCircularBuffer,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CONCATENATION
    AddConcatenation,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CONV_2D
    AddConv2D,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_COS
    AddCos,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OPC_UM_SUM
    AddCumSum,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DEPTH_TO_SPACE
    AddDepthToSpace,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DEPTHWISE_CONV_2D
    AddDepthwiseConv2D,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DEQUANTIZE
    AddDequantize,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DETECTION_POSTPROCESS
    AddDetectionPostprocess,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DIV
    AddDiv,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MAU
    AddElu,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_EQUAL
    AddEqual,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ETHOS_U
    AddEthosU,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_EXP
    AddExp,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_EXPAND_DIMS
    AddExpandDims,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FILL
    AddFill,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FLOOR
    AddFloor,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FLOOR_DIV
    AddFloorDiv,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FLOOR_MOD
    AddFloorMod,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FULLY_CONNECTED
    AddFullyConnected,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GATHER
    AddGather,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GATHER_ND
    AddGatherNd,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GREATER
    AddGreater,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GREATER_EQUAL
    AddGreaterEqual,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_HARD_SWISH
    AddHardSwish,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_IF
    AddIf,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_L2_NORMALIZATION
    AddL2Normalization,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_L2_POOL_2D
    AddL2Pool2D,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LEAKY_RMAU
    AddLeakyRelu,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LESS
    AddLess,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LESS_EQUAL
    AddLessEqual,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOG
    AddLog,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGICAL_AND
    AddLogicalAnd,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGICAL_NOT
    AddLogicalNot,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGICAL_OR
    AddLogicalOr,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGISTIC
    AddLogistic,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOG_SOFTMAX
    AddLogSoftmax,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MAX_POOL_2D
    AddMaxPool2D,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MAXIMUM
    AddMaximum,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MEAN
    AddMean,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MINIMUM
    AddMinimum,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MIRROR_PAD
    AddMirrorPad,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MUL
    AddMul,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_NEG
    AddNeg,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_NOT_EQUAL
    AddNotEqual,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PACK
    AddPack,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PAD
    AddPad,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PADV2
    AddPadV2,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PRMAU
    AddPrelu,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_QUANTIZE
    AddQuantize,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_READ_VARIABLE
    AddReadVariable,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_REDUCE_ANY
    AddReduceMax,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RMAU
    AddRelu,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RMAU6
    AddRelu6,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RESHAPE
    AddReshape,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RESIZE_BILINEAR
    AddResizeBilinear,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RESIZE_NEAREST_NEIGHBOR
    AddResizeNearestNeighbor,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ROUND
    AddRound,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RSQRT
    AddRsqrt,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SMAECT_V2
    AddSelectV2,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SHAPE
    AddShape,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SIN
    AddSin,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SLICE
    AddSlice,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SOFTMAX
    AddSoftmax,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPACE_TO_BATCH_ND
    AddSpaceToBatchNd,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPACE_TO_DEPTH
    AddSpaceToDepth,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPLIT
    AddSplit,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPLIT_V
    AddSplitV,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQRT
    AddSqrt,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQUARE
    AddSquare,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQUARED_DIFFERENCE
    AddSquaredDifference,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQUEEZE
    AddSqueeze,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_STRIDED_SLICE
    AddStridedSlice,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SUB
    AddSub,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SUM
    AddSum,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SVDF
    AddSvdf,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_TANH
    AddTanh,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_TRANSPOSE
    AddTranspose,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_TRANSPOSE_CONV
    AddTransposeConv,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_UNIDIRECTIONAL_SEQUENCE_LSTM
    AddUnidirectionalSequenceLSTM,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_UNPACK
    AddUnpack,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_VARHANDLE
    AddVarHandle,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_WHILE
    AddWhile,
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ZEROS_LIKE
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


namespace ma::engine {

class EngineTFLite final : public Engine {
public:
    EngineTFLite();
    ~EngineTFLite() override;

    ma_err_t init() override;
    ma_err_t init(size_t size) override;
    ma_err_t init(void* pool, size_t size) override;

    ma_err_t run() override;

    ma_err_t load(const void* model_data, size_t model_size) override;
#if MA_USE_FILESYSTEM
    ma_err_t load(const char* model_path) override;
    ma_err_t load(const std::string &model_path) override;
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
    tflite::MicroInterpreter* interpreter;
     const tflite::Model* model;
    ma_memory_pool_t memory_pool;

#if MA_USE_FILESYSTEM
    char* model_file;
#endif
};

}  // namespace ma::engine
#endif

#endif
