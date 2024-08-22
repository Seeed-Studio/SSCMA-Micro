set(JPEGENC_DIR "${CMAKE_CURRENT_LIST_DIR}/")
file(GLOB SRCS_JPEGENC
    "${JPEGENC_DIR}/JPEGENC.cpp"
)

set(INCS_JPEGENC
    ${JPEGENC_DIR}
)

if(SOPHGO_PLATFORM)
component_register(
    COMPONENT_NAME jpegenc
    SRCS ${SRCS_JPEGENC}
    INCLUDE_DIRS ${INCS_JPEGENC}
)
else()
add_library(jpegenc STATIC ${SRCS_JPEGENC})
target_include_directories(jpegenc PUBLIC ${INCS_JPEGENC})
message(STATUS "INCS_JPEGENC: ${INCS_JPEGENC}")
endif()

