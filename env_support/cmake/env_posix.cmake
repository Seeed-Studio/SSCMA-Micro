message(STATUS "Building for custom environment")


if(SSCMA_CONF_PATH)
    get_filename_component(SSCMA_CONF_DIR ${SSCMA_CONF_PATH} DIRECTORY)
endif(SSCMA_CONF_PATH)


file(GLOB_RECURSE CORE_SOURCES ${SSCMA_ROOT_DIR}/sscma/core/*.c ${SSCMA_ROOT_DIR}/sscma/core/*.cpp)

file(GLOB_RECURSE SREVER_SOURCES ${SSCMA_ROOT_DIR}/sscma/server/*.c ${SSCMA_ROOT_DIR}/sscma/server/*.cpp)

file(GLOB_RECURSE CLIENT_SOURCES ${SSCMA_ROOT_DIR}/sscma/client/*.c ${SSCMA_ROOT_DIR}/sscma/client/*.cpp)

file(GLOB_RECURSE PORT_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/posix/*.c ${SSCMA_ROOT_DIR}/sscma/porting/posix/*.cpp)

set(SOURCES ${CORE_SOURCES} ${SREVER_SOURCES} ${CLIENT_SOURCES} ${PORT_SOURCES} ${SSCMA_ROOT_DIR}/sscma/porting/osal/ma_osal_pthread.cpp)

set(INCS 
        ${SSCMA_ROOT_DIR}/sscma
        ${SSCMA_ROOT_DIR}/sscma/porting/posix
)

if(SSCMA_SHARED_LIB)
add_library(sscma SHARED ${SOURCES})
else()
add_library(sscma STATIC ${SOURCES})
endif()

target_include_directories(sscma PUBLIC ${INCS})

if(SSCMA_CONF_PATH)
  target_include_directories(sscma PUBLIC ${SSCMA_CONF_DIR})
endif()

if(SSCMA_ENGINE_TFLITE)
include(${SSCMA_ROOT_DIR}/3rdparty/tflite-micro/CMakeLists.txt)
include(${SSCMA_ROOT_DIR}/3rdparty/json/CMakeLists.txt)
target_link_libraries(sscma PUBLIC tflm)
target_link_libraries(sscma PUBLIC cjson)
endif()

