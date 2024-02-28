#ifndef _DRV_SHARED_CFG_H_
#define _DRV_SHARED_CFG_H_

#define FRAME_WIDTH_MAX         640
#define FRAME_HEIGHT_MAX        480

#define HW1_ADDR_BASE           0x36000000
#define HW1_ADDR_SIZE           0

#define JPEG_10X_SIZE_EXP(w, h) (((623 + (size_t)(w / 16) * (size_t)(h / 16) * 128 + 35) >> 2) << 2)
#define JPEG_BASE_ADDR          (HW1_ADDR_BASE + HW1_ADDR_SIZE)
#define JPEG_SIZE_MAX           JPEG_10X_SIZE_EXP(FRAME_WIDTH_MAX, FRAME_HEIGHT_MAX)  // 150KB

#define JPEG_SZ_BASE_ADDR       (JPEG_BASE_ADDR + JPEG_SIZE_MAX)
#define JPEG_SZ_SIZE            0x1000  // 4KB

#define YUV422_SIZE_EXP(w, h)   (w * h * 3 / 2)
#define YUV422_BASE_ADDR        0x3418F800
#define YUV422_SIZE_MAX         YUV422_SIZE_EXP(FRAME_WIDTH_MAX, FRAME_HEIGHT_MAX)  // 450KB

#endif
