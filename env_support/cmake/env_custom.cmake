message(STATUS "Building for custom environment")


if(SSCMA_CONF_PATH)
    get_filename_component(SSCMA_CONF_DIR ${SSCMA_CONF_PATH} DIRECTORY)
endif(SSCMA_CONF_PATH)

file(GLOB_RECURSE SOURCES ${SSCMA_ROOT_DIR}/sscma/*.c ${SSCMA_ROOT_DIR}/sscma/*.cpp ${LVGL_ROOT_DIR}/sscma/*.S)

if(SSCMA_SHARED_LIB)
add_library(sscma SHARED ${SOURCES})
else()
add_library(sscma STATIC ${SOURCES})
endif()

target_include_directories(sscma PUBLIC ${SSCMA_ROOT_DIR}/sscma)

if(SSCMA_CONF_PATH)
  target_include_directories(sscma PUBLIC ${SSCMA_CONF_DIR})
endif()

if(SSCMA_ENGINE_TFLITE)
include(${SSCMA_ROOT_DIR}/3rdparty/tflite-micro/CMakeLists.txt)
target_link_libraries(sscma PUBLIC tflm)
endif()

