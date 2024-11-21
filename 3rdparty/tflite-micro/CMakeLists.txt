cmake_minimum_required(VERSION 3.8.2)

set(component_hash "be11bd79e4a8b28c9ee92c6f02ca0e85414fb768")
set(fetch_script "${SSCMA_ROOT_DIR}/scripts/fetch_tflite_micro.sh")

message(STATUS "component_hash: ${component_hash}")
message(STATUS "fetch_script: ${fetch_script}")

execute_process(
    COMMAND ${fetch_script} ${component_hash}
    RESULT_VARIABLE result
)

if(NOT result EQUAL "0")
    message(FATAL_ERROR "fetch_tflite_micro.sh failed")
endif()

set(TFLM_DIR "${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite")
set(TFLM_SIGNAL_DIR "${CMAKE_CURRENT_LIST_DIR}/signal")
set(TFLM_THIRDPARTY_DIR "${CMAKE_CURRENT_LIST_DIR}/third_party")

file(GLOB SRCS_TFLM
    "${TFLM_DIR}/micro/*.cc"
    "${TFLM_DIR}/micro/*.c"
    "${TFLM_DIR}/micro/kernels/*.cc"
    "${TFLM_DIR}/micro/kernels/*.c"
    "${TFLM_DIR}/micro/memory_planner/*.cc"
    "${TFLM_DIR}/micro/memory_planner/*.c"
    "${TFLM_DIR}/micro/arena_allocator/*.cc"
    "${TFLM_DIR}/micro/arena_allocator/*.c"
    "${TFLM_DIR}/micro/tflite_bridge/*.cc"
    "${TFLM_DIR}/micro/tflite_bridge/*.c"
    "${TFLM_DIR}/kernels/*.cc"
    "${TFLM_DIR}/kernels/*.c"
    "${TFLM_DIR}/kernels/internal/*.cc"
    "${TFLM_DIR}/kernels/internal/*.c"
    "${TFLM_DIR}/kernels/internal/reference/*.cc"
    "${TFLM_DIR}/kernels/internal/reference/*.c"
    "${TFLM_DIR}/core/c/*.cc"
    "${TFLM_DIR}/core/c/*.c"
    "${TFLM_DIR}/core/api/*.cc"
    "${TFLM_DIR}/core/api/*.c"
    "${TFLM_DIR}/schema/*.cc"
    "${TFLM_SIGNAL_DIR}/micro/kernels/*.c"
    "${TFLM_SIGNAL_DIR}/micro/kernels/*.cc"
    "${TFLM_SIGNAL_DIR}/src/*.c"
    "${TFLM_SIGNAL_DIR}/src/*.cc"
    "${TFLM_SIGNAL_DIR}/src/kiss_fft_wrappers/*.c"
    "${TFLM_SIGNAL_DIR}/src/kiss_fft_wrappers/*.cc"
)

set(INCS_TFLM 
    ${CMAKE_CURRENT_LIST_DIR}
    ${TFLM_SIGNAL_DIR}
    ${TFLM_SIGNAL_DIR}/src
    ${TFLM_SIGNAL_DIR}/micro/kernels
    ${TFLM_SIGNAL_DIR}/src/kiss_fft_wrappers
    ${TFLM_THIRDPARTY_DIR}/flatbuffers/include
    ${TFLM_THIRDPARTY_DIR}/gemmlowp
    ${TFLM_THIRDPARTY_DIR}/ruy
    ${TFLM_THIRDPARTY_DIR}/kissfft
)

if(POSIX_PLATFORM)
add_library(tflm STATIC ${SRCS_TFLM})
target_include_directories(tflm PUBLIC ${INCS_TFLM})
target_compile_options(tflm PRIVATE -fno-unwind-tables -ffunction-sections -fdata-sections -fmessage-length=0 -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON -O3 -Wsign-compare -Wdouble-promotion -Wunused-variable -Wmissing-field-initializers -Wunused-function -Wswitch -Wvla -Wall -Wextra -Wstrict-aliasing -Wno-unused-parameter -Wno-nonnull -Wno-shadow)
target_compile_options(tflm PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -std=c++11 -fno-rtti -fno-exceptions -fno-threadsafe-statics -fno-unwind-tables -ffunction-sections -fdata-sections -fmessage-length=0 -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON -O3 -Werror -Wsign-compare -Wdouble-promotion -Wunused-variable -Wmissing-field-initializers -Wunused-function -Wswitch -Wvla -Wall -Wextra -Wstrict-aliasing -Wno-unused-parameter -Wno-return-type -Wno-strict-aliasing >)
target_compile_options(tflm INTERFACE $<$<IN_LIST:-DTF_LITE_STATIC_MEMORY,$<TARGET_PROPERTY:tflm,COMPILE_OPTIONS>>:-DTF_LITE_STATIC_MEMORY>)
target_link_libraries(tflm PRIVATE -lm)
endif()

message(STATUS "INCS_TFLM: ${INCS_TFLM}")