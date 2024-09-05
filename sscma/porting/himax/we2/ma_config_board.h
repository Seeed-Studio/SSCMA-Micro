#ifndef _MA_CONFIG_BOARD_H_
#define _MA_CONFIG_BOARD_H_

#ifndef MA_PLATFORM_HIMAX
    #error "Include with explicit platform"
#endif

#ifdef MA_BOARD_GROVE_VISION_AI_V2
    #include "boards/ma_board_grove_vision_ai_v2.h"
#elif defined(MA_BOARD_WATCHER)
    #include "boards/ma_board_watcher.h"
#else
    #error "Missing board configuration"
#endif

#define MA_USE_ENGINE_TFLITE               1
#define MA_ENGINE_TFLITE_TENSOE_ARENA_SIZE (1200 * 1024)
#define MA_USE_ENGINE_TENSOR_INDEX         1

#define MA_FILESYSTEM_LITTLEFS             1
#define MA_STORAGE_LFS_USE_FLASHBD         1

#define MA_OSAL_FREERTOS                   1

#endif  // _MA_CONFIG_BOARD_H_