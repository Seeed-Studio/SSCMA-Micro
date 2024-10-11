
message(STATUS "Building for ESP environment")


file(GLOB_RECURSE CORE_SOURCES ${SSCMA_ROOT_DIR}/sscma/core/*.c ${SSCMA_ROOT_DIR}/sscma/core/*.cpp)

file(GLOB_RECURSE SREVER_SOURCES ${SSCMA_ROOT_DIR}/sscma/server/*.c ${SSCMA_ROOT_DIR}/sscma/server/*.cpp ${SSCMA_ROOT_DIR}/sscma/server/*.hpp)

file(GLOB_RECURSE CLIENT_SOURCES ${SSCMA_ROOT_DIR}/sscma/client/*.c ${SSCMA_ROOT_DIR}/sscma/client/*.cpp)

file(GLOB_RECURSE DRIVER_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/drivers/bd/*.c ${SSCMA_ROOT_DIR}/sscma/porting/drivers/bd/*.cpp)

file(GLOB_RECURSE PORT_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/espressif/*.c ${SSCMA_ROOT_DIR}/sscma/porting/espressif/*.cpp ${SSCMA_ROOT_DIR}/sscma/porting/espressif/esp32s3/drivers/bd/*.c ${SSCMA_ROOT_DIR}/sscma/porting/espressif/esp32s3/*.c ${SSCMA_ROOT_DIR}/sscma/porting/espressif/esp32s3/*.cpp ${SSCMA_ROOT_DIR}/sscma/porting/osal/ma_osal_freertos.cpp)

file(GLOB_RECURSE LFS_SOURCES ${SSCMA_ROOT_DIR}/3rdparty/littlefs/littlefs/lfs.c ${SSCMA_ROOT_DIR}/3rdparty/littlefs/littlefs/lfs_util.c)

file(GLOB_RECURSE JPEGENC_SOURCES ${SSCMA_ROOT_DIR}/3rdparty/JPEGENC/*.c ${SSCMA_ROOT_DIR}/3rdparty/JPEGENC/*.cpp)

set(srcs ${CORE_SOURCES} ${SREVER_SOURCES} ${CLIENT_SOURCES} ${PORT_SOURCES} ${DRIVER_SOURCES} ${LFS_SOURCES} ${JPEGENC_SOURCES})

set(includes 
        ${SSCMA_ROOT_DIR}/sscma
        ${SSCMA_ROOT_DIR}/sscma/porting
        ${SSCMA_ROOT_DIR}/sscma/porting/espressif
        ${SSCMA_ROOT_DIR}/sscma/porting/espressif/esp32s3
        ${SSCMA_ROOT_DIR}/sscma/porting/drivers
        ${SSCMA_ROOT_DIR}/sscma/porting/drivers/bd
        ${SSCMA_ROOT_DIR}/sscma/server
        ${SSCMA_ROOT_DIR}/sscma/server/at
        ${SSCMA_ROOT_DIR}/sscma/server/at/codec
        ${SSCMA_ROOT_DIR}/sscma/server/at/callback
        ${SSCMA_ROOT_DIR}/3rdparty/JPEGENC
        ${SSCMA_ROOT_DIR}/3rdparty/littlefs/littlefs

)

set(require "json" "mbedtls" "esp_timer")
set(priv_requires "driver" esp-tflite-micro esp32-camera esp_timer freertos driver spi_flash esp_partition efuse)

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES ${require}
                       PRIV_REQUIRES ${priv_requires}
                       )

target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_MA_PORTING_ESPRESSIF=1)
target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_MA_TFLITE_OP_ALL=1)
