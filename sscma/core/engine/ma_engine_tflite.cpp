#include "ma_engine_tflite.h"

#if MA_USE_ENGINE_TFLITE

namespace tflite {

OpsResolver::OpsResolver() {
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ABS
    AddAbs();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ADD
    AddAdd();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ADDN
    AddAddN();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ARGMAX
    AddArgMax();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ARGMIN
    AddArgMin();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ASSIGN_VARIABLE
    AddAssignVariable();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_AVERAGE_POOL_2D
    AddAveragePool2D();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_BATCH_TO_SPACE_ND
    AddBatchToSpaceNd();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_BROADCAST_ARGS
    AddBroadcastArgs();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_BROADCAST_TO
    AddBroadcastTo();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CALL_ONCE
    AddCallOnce();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CAST
    AddCast();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CEIL
    AddCeil();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CIRULAR_BUFFER
    AddCircularBuffer();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CONCATENATION
    AddConcatenation();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CONV_2D
    AddConv2D();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_COS
    AddCos();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_CUM_SUM
    AddCumSum();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DEPTH_TO_SPACE
    AddDepthToSpace();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DEPTHWISE_CONV_2D
    AddDepthwiseConv2D();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DEQUANTIZE
    AddDequantize();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DETECTION_POSTPROCESS
    AddDetectionPostprocess();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_DIV
    AddDiv();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MAU
    AddElu();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_EQUAL
    AddEqual();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ETHOS_U
    AddEthosU();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_EXP
    AddExp();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_EXPAND_DIMS
    AddExpandDims();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FILL
    AddFill();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FLOOR
    AddFloor();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FLOOR_DIV
    AddFloorDiv();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FLOOR_MOD
    AddFloorMod();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_FULLY_CONNECTED
    AddFullyConnected();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GATHER
    AddGather();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GATHER_ND
    AddGatherNd();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GREATER
    AddGreater();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_GREATER_EQUAL
    AddGreaterEqual();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_HARD_SWISH
    AddHardSwish();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_IF
    AddIf();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_L2_NORMALIZATION
    AddL2Normalization();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_L2_POOL_2D
    AddL2Pool2D();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LEAKY_RMAU
    AddLeakyRelu();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LESS
    AddLess();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LESS_EQUAL
    AddLessEqual();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOG
    AddLog();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGICAL_AND
    AddLogicalAnd();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGICAL_NOT
    AddLogicalNot();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGICAL_OR
    AddLogicalOr();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOGISTIC
    AddLogistic();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_LOG_SOFTMAX
    AddLogSoftmax();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MAX_POOL_2D
    AddMaxPool2D();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MAXIMUM
    AddMaximum();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MEAN
    AddMean();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MINIMUM
    AddMinimum();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MIRROR_PAD
    AddMirrorPad();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_MUL
    AddMul();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_NEG
    AddNeg();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_NOT_EQUAL
    AddNotEqual();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PACK
    AddPack();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PAD
    AddPad();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PADV2
    AddPadV2();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_PRMAU
    AddPrelu();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_QUANTIZE
    AddQuantize();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_READ_VARIABLE
    AddReadVariable();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_REDUCE_ANY
    AddReduceMax();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RMAU
    AddRelu();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RMAU6
    AddRelu6();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RESHAPE
    AddReshape();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RESIZE_BILINEAR
    AddResizeBilinear();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RESIZE_NEAREST_NEIGHBOR
    AddResizeNearestNeighbor();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ROUND
    AddRound();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_RSQRT
    AddRsqrt();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SMAECT_V2
    AddSelectV2();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SHAPE
    AddShape();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SIN
    AddSin();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SLICE
    AddSlice();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SOFTMAX
    AddSoftmax();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPACE_TO_BATCH_ND
    AddSpaceToBatchNd();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPACE_TO_DEPTH
    AddSpaceToDepth();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPLIT
    AddSplit();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SPLIT_V
    AddSplitV();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQRT
    AddSqrt();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQUARE
    AddSquare();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQUARED_DIFFERENCE
    AddSquaredDifference();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SQUEEZE
    AddSqueeze();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_STRIDED_SLICE
    AddStridedSlice();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SUB
    AddSub();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_SUM
    AddSum();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OPSVDF
    AddSvdf();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_TANH
    AddTanh();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_TRANSPOSE
    AddTranspose();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_TRANSPOSE_CONV
    AddTransposeConv();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_UNIDIRECTIONAL_SEQUENCE_LSTM
    AddUnidirectionalSequenceLSTM();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_UNPACK
    AddUnpack();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_VARHANDLE
    AddVarHandle();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_WHILE
    AddWhile();
#endif
#if MA_TFLITE_OP_ALL || MA_TFLITE_OP_ZEROS_LIKE
    AddZerosLike();
#endif
}

}  // namespace tflite

namespace ma::engine {

static const ma_tensor_type_t mapped_tensor_types[] = {
    MA_TENSOR_TYPE_NONE, MA_TENSOR_TYPE_F32, MA_TENSOR_TYPE_S32,  MA_TENSOR_TYPE_U8,
    MA_TENSOR_TYPE_S64,  MA_TENSOR_TYPE_STR, MA_TENSOR_TYPE_BOOL, MA_TENSOR_TYPE_S16,
    MA_TENSOR_TYPE_NONE, MA_TENSOR_TYPE_S8,  MA_TENSOR_TYPE_F16,  MA_TENSOR_TYPE_F64,
    MA_TENSOR_TYPE_NONE, MA_TENSOR_TYPE_S64, MA_TENSOR_TYPE_NONE, MA_TENSOR_TYPE_NONE,
    MA_TENSOR_TYPE_U32,  MA_TENSOR_TYPE_U64, MA_TENSOR_TYPE_NONE, MA_TENSOR_TYPE_BF16,
};


EngineTFLite::EngineTFLite() {
    interpreter      = nullptr;
    model            = nullptr;
    memory_pool.pool = nullptr;
    memory_pool.size = 0;
#if MA_USE_FILESYSTEM
    model_file = nullptr;
#endif
}

EngineTFLite::~EngineTFLite() {
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
    }
    if (memory_pool.pool != nullptr && memory_pool.own) {
        #ifndef MA_USE_STATIC_TENSOR_ARENA
        delete[] static_cast<uint8_t*>(memory_pool.pool);
        #endif
        memory_pool.pool = nullptr;
    }
#if MA_USE_FILESYSTEM
    if (model_file != nullptr) {
        delete model_file;
        model_file = nullptr;
    }
#endif
}

ma_err_t EngineTFLite::init() {
    return init(MA_ENGINE_TFLITE_TENSOE_ARENA_SIZE);
}

#ifdef MA_USE_STATIC_TENSOR_ARENA
extern "C" {
extern uint8_t* _ma_static_tensor_arena;
}
#endif

ma_err_t EngineTFLite::init(size_t size) {
    if (memory_pool.pool != nullptr) {
        return MA_EPERM;
    }
    #ifdef MA_USE_STATIC_TENSOR_ARENA
    void* pool = _ma_static_tensor_arena;
    #else
    void* pool = new uint8_t[size];
    #endif
    if (pool == nullptr) {
        return MA_ENOMEM;
    }
    memory_pool.pool = pool;
    memory_pool.size = size;
    memory_pool.own  = true;
    return MA_OK;
}

ma_err_t EngineTFLite::init(void* pool, size_t size) {
    if (memory_pool.pool != nullptr) {
        return MA_EPERM;
    }
    memory_pool.pool = pool;
    memory_pool.size = size;
    memory_pool.own  = false;
    return MA_OK;
}

ma_err_t EngineTFLite::run() {
    MA_ASSERT(interpreter != nullptr);

    if (kTfLiteOk != interpreter->Invoke()) {
        return MA_ELOG;
    }
    return MA_OK;
}

ma_err_t EngineTFLite::load(const void* model_data, size_t model_size) {
    model = tflite::GetModel(model_data);

    if (model == nullptr) {
        return MA_EINVAL;
    }

    // Clear the existing interpreter
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
    }

    static tflite::OpsResolver resolver;

    interpreter = new tflite::MicroInterpreter(
        model, resolver, static_cast<uint8_t*>(memory_pool.pool), memory_pool.size);
    if (interpreter == nullptr) {
        return MA_ENOMEM;
    }
    if (kTfLiteOk != interpreter->AllocateTensors()) {
        delete interpreter;
        interpreter = nullptr;
        return MA_ELOG;
    }
    return MA_OK;
}
ma_tensor_t EngineTFLite::getInput(int32_t index) {
    MA_ASSERT(interpreter != nullptr);
    ma_tensor_t tensor{0};

    if (index >= interpreter->inputs().size()) {
        return tensor;
    }
    TfLiteTensor* input = interpreter->input_tensor(index);
    if (input == nullptr) {
        return tensor;
    }

    tensor.data.data   = input->data.data;
    tensor.size        = input->bytes;
#if MA_USE_ENGINE_TENSOR_INDEX
    tensor.index       = index;
#endif
    tensor.type        = mapped_tensor_types[static_cast<int>(input->type)];
    tensor.shape       = getInputShape(index);
    tensor.quant_param = getInputQuantParam(index);
    tensor.is_variable = true;


    return tensor;
}

ma_tensor_t EngineTFLite::getOutput(int32_t index) {
    MA_ASSERT(interpreter != nullptr);
    ma_tensor_t tensor{0};

    if (index >= interpreter->outputs().size()) {
        return tensor;
    }
    TfLiteTensor* output = interpreter->output_tensor(index);
    if (output == nullptr) {
        return tensor;
    }

    tensor.data.data   = output->data.data;
    tensor.size        = output->bytes;
#if MA_USE_ENGINE_TENSOR_INDEX
    tensor.index       = index;
#endif
    tensor.type        = mapped_tensor_types[static_cast<int>(output->type)];
    tensor.shape       = getOutputShape(index);
    tensor.quant_param = getOutputQuantParam(index);
    tensor.is_variable = false;

    return tensor;
}

ma_shape_t EngineTFLite::getInputShape(int32_t index) {
    ma_shape_t shape;

    MA_ASSERT(interpreter != nullptr);

    shape.size = 0;
    if (index >= interpreter->inputs().size()) {
        return shape;
    }
    TfLiteTensor* input = interpreter->input_tensor(index);
    if (input == nullptr) {
        return shape;
    }
    shape.size = input->dims->size;
    MA_ASSERT(shape.size < MA_ENGINE_SHAPE_MAX_DIM);
    for (int i = 0; i < shape.size; i++) {
        shape.dims[i] = input->dims->data[i];
    }

    return shape;
}

ma_shape_t EngineTFLite::getOutputShape(int32_t index) {
    ma_shape_t shape;
    shape.size = 0;

    MA_ASSERT(interpreter != nullptr);

    if (index >= interpreter->outputs().size()) {
        return shape;
    }
    TfLiteTensor* output = interpreter->output_tensor(index);
    if (output == nullptr) {
        return shape;
    }
    shape.size = output->dims->size;

    MA_ASSERT(shape.size < MA_ENGINE_SHAPE_MAX_DIM);

    for (int i = 0; i < shape.size; i++) {
        shape.dims[i] = output->dims->data[i];
    }

    return shape;
}

ma_quant_param_t EngineTFLite::getInputQuantParam(int32_t index) {
    ma_quant_param_t quant_param;
    quant_param.scale      = 0;
    quant_param.zero_point = 0;

    MA_ASSERT(interpreter != nullptr);

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

ma_quant_param_t EngineTFLite::getOutputQuantParam(int32_t index) {
    ma_quant_param_t quant_param;
    quant_param.scale      = 0;
    quant_param.zero_point = 0;

    MA_ASSERT(interpreter != nullptr);

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

ma_err_t EngineTFLite::setInput(int32_t index, const ma_tensor_t& tensor) {
    return MA_ENOTSUP;
}

#if MA_USE_FILESYSTEM
ma_err_t EngineTFLite::load(const char* model_path) {
    ma_err_t ret = MA_OK;
    size_t size  = 0;
#ifdef MA_USE_FILESYSTEM_POSIX
    if(model_file != nullptr) {
        delete model_file;
        model_file = nullptr;
    }
    std::ifstream file(model_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return MA_ELOG;
    }
    size       = file.tellg();
    model_file = new char[size];
    if (model_file == nullptr) {
        return MA_ENOMEM;
    }
    file.seekg(0, std::ios::beg);
    file.read(model_file, size);
    file.close();
    ret = load(model_file, size);
    if (ret != MA_OK) {
        delete model_file;
        model_file = nullptr;
    }

    return ret;
#else
    return MA_ENOTSUP;
#endif
}

ma_err_t EngineTFLite::load(const std::string& model_path) {
    return load(model_path.c_str());
}
#endif

int32_t EngineTFLite::getInputSize() {
    MA_ASSERT(interpreter != nullptr);
    return interpreter->inputs().size();
}
int32_t EngineTFLite::getOutputSize() {
    MA_ASSERT(model != nullptr);
    return interpreter->outputs().size();
}

}  // namespace ma::engine

#endif
