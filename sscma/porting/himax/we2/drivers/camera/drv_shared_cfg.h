#ifndef _DRV_SHARED_CFG_H_
#define _DRV_SHARED_CFG_H_

#include <WE2_device_addr.h>

#define DEAULT_XHSUTDOWN_PIN      AON_GPIO2

#define ENABLE_SENSOR_FAST_SWITCH 1
#define SKIP_FRAME_COUNT          2
#define WAIT_FRAME_YIELD_MS       3

#define ALIGN_32BIT(x)            (((x) + 0x1F) & ~0x1F)

#define FRAME_WIDTH_MAX           640
#define FRAME_HEIGHT_MAX          480

#define SRAM_1_TAIL_ADDR          0x3416A000
#define SRAM_1_TAIL_SIZE          (0x34200000 - 0x3416A000)  // 600KB

#define SRAM_2_START_ADDR         BASE_ADDR_SRAM2_ALIAS
#define SRAM_2_SIZE               (0x36060000 - BASE_ADDR_SRAM2_ALIAS)  // 384KB

#define YUV422_SIZE_EXP(w, h)     (w * h * 2)

#define YUV422_BASE_ADDR          ALIGN_32BIT(SRAM_1_TAIL_ADDR)
#define YUV422_BASE_SIZE          YUV422_SIZE_EXP(FRAME_WIDTH_MAX, FRAME_HEIGHT_MAX)  // 600KB

#define WDMA_1_BASE_ADDR          SRAM_2_START_ADDR
#define WDMA_1_BASE_SIZE          0

#define JPEG_BASE_SIZE_EXP(w, h) \
    (((623 + (size_t)(w / 16) * (size_t)(h / 16) * 128 + 35) >> 2) << 2)  // 4x quantization

#define JPEG_BASE_ADDR         ALIGN_32BIT(WDMA_1_BASE_ADDR + WDMA_1_BASE_SIZE)
#define JPEG_BASE_SIZE         JPEG_BASE_SIZE_EXP(FRAME_WIDTH_MAX, FRAME_HEIGHT_MAX)  // 150KB

#define JPEG_FILL_BASE_ADDR    ALIGN_32BIT(JPEG_BASE_ADDR + JPEG_BASE_SIZE)
#define JPEG_FILL_SIZE         0x1000  // 4KB

#define SRAM_2_START_ADDR_FREE (JPEG_FILL_BASE_ADDR + JPEG_FILL_SIZE)
#define SRAM_2_SIZE_FREE       (SRAM_2_SIZE - (SRAM_2_START_ADDR_FREE - SRAM_2_START_ADDR))

#endif
