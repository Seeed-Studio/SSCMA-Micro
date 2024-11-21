set(JPEGENC_DIR "${CMAKE_CURRENT_LIST_DIR}/")
file(GLOB SRCS_JPEGENC
    "${JPEGENC_DIR}/JPEGENC.cpp"
)

set(INCS_JPEGENC
    ${JPEGENC_DIR}
)

if(POSIX_PLATFORM)
add_library(jpegenc STATIC ${SRCS_JPEGENC})
target_include_directories(jpegenc PUBLIC ${INCS_JPEGENC})
endif()

message(STATUS "INCS_JPEGENC: ${INCS_JPEGENC}")


