cmake_minimum_required(VERSION 3.8.2)

set(JPEGENC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


file(GLOB SRCS_JPEGENC
    "${JPEGENC_DIR}/*.c"
    "${JPEGENC_DIR}/*.cpp"
)

set(INCS_JPEGENC
    "${JPEGENC_DIR}"
    CACHE INTERNAL "jpegenc include directories"
)

add_library(jpegenc STATIC ${SRCS_JPEGENC})
target_include_directories(jpegenc PRIVATE ${INCS_JPEGENC})
