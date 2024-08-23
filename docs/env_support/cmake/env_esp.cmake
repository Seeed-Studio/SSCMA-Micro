
message(STATUS "Building for ESP environment")


file(GLOB_RECURSE CORE_SOURCES ${SSCMA_ROOT_DIR}/sscma/core/*.c ${SSCMA_ROOT_DIR}/sscma/core/*.cpp)

file(GLOB_RECURSE SREVER_SOURCES ${SSCMA_ROOT_DIR}/sscma/server/*.c ${SSCMA_ROOT_DIR}/sscma/server/*.cpp)

file(GLOB_RECURSE CLIENT_SOURCES ${SSCMA_ROOT_DIR}/sscma/client/*.c ${SSCMA_ROOT_DIR}/sscma/client/*.cpp)

file(GLOB_RECURSE PORT_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/espressif/*.c ${SSCMA_ROOT_DIR}/sscma/porting/espressif/*.cpp  ${SSCMA_ROOT_DIR}/sscma/porting/osal/ma_osal_freertos.cpp)

set(srcs ${CORE_SOURCES} ${SREVER_SOURCES} ${CLIENT_SOURCES} ${PORT_SOURCES})

set(includes 
        ${SSCMA_ROOT_DIR}/sscma
        ${SSCMA_ROOT_DIR}/sscma/porting/espressif
)

set(require "json" "mbedtls" "esp_timer")
set(priv_requires "driver")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES ${require}
                       PRIV_REQUIRES ${priv_requires}
                       )

target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_MA_PORTING_ESPRESSIF=1)
target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_MA_TFLITE_OP_ALL=1)
