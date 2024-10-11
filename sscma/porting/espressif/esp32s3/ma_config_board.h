#ifndef _MA_CONFIG_BOARD_H_
#define _MA_CONFIG_BOARD_H_

#define MA_BOARD_NAME                     "XIAO(ESP32S3)"

#define MA_USE_ENGINE_TFLITE               1
#define MA_ENGINE_TFLITE_TENSOE_ARENA_SIZE (1000 * 1024)
#define MA_USE_ENGINE_TENSOR_INDEX         1

#define MA_TFLITE_OP_SOFTMAX               1
#define MA_TFLITE_OP_PADV2                 1
#define MA_TFLITE_OP_TRANSPOSE             1
#define MA_TFLITE_OP_ETHOS_U               1
#define MA_TFLITE_OP_DEQUANTIZE            1
#define MA_TFLITE_OP_QUANTIZE              1
#define MA_TFLITE_OP_FULLY_CONNECTED       1
#define MA_TFLITE_OP_GATHER                1
#define MA_TFLITE_OP_RESHAPE               1
#define MA_TFLITE_OP_MAX_POOL_2D           1
#define MA_TFLITE_OP_MUL                   1
#define MA_TFLITE_OP_BROADCAST_TO          1

#define MA_INVOKE_ENABLE_RUN_HOOK          1

#define MA_SENSOR_ENCODE_USE_STATIC_BUFFER 1
#define MA_SENSOR_ENCODE_STATIC_BUFFER_ADDR (0x36000000 + (200 * 1024))
#define MA_SENSOR_ENCODE_STATIC_BUFFER_SIZE (0x36060000 - MA_SENSOR_ENCODE_STATIC_BUFFER_ADDR)

#define MA_FILESYSTEM_LITTLEFS             1
#define MA_STORAGE_LFS_USE_FLASHBD         1

#define MA_OSAL_FREERTOS                   1
#define MA_SEVER_AT_EXECUTOR_STACK_SIZE    (20 * 1024)
#define MA_SEVER_AT_EXECUTOR_TASK_PRIO     2

// #define MA_CONFIG_BOARD_I2C_SLAVE          1

#endif  // _MA_CONFIG_BOARD_H_