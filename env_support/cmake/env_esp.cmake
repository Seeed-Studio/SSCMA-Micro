
message(STATUS "Building for ESP environment")
file(GLOB_RECURSE srcs ${SSCMA_ROOT_DIR}/sscma/*.c ${SSCMA_ROOT_DIR}/sscma/*.cpp ${LVGL_ROOT_DIR}/sscma/*.S)

set(includes ${SSCMA_ROOT_DIR}/sscma)
set(require "json" "mbedtls" "esp_timer")
set(priv_requires "driver")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES ${require}
                       PRIV_REQUIRES ${priv_requires}
                       )

target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_MA_PORTING_ESPRESSIF=1)
target_compile_definitions(${COMPONENT_LIB} PUBLIC CONFIG_MA_TFLITE_OP_ALL=1)
