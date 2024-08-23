message(STATUS "Building for custom environment")


if(SSCMA_CONF_PATH)
    get_filename_component(SSCMA_CONF_DIR ${SSCMA_CONF_PATH} DIRECTORY)
endif(SSCMA_CONF_PATH)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")


file(GLOB_RECURSE CORE_SSCMA_SOURCES ${SSCMA_ROOT_DIR}/sscma/core/*.c ${SSCMA_ROOT_DIR}/sscma/core/*.cpp)

file(GLOB_RECURSE SREVER_SSCMA_SOURCES ${SSCMA_ROOT_DIR}/sscma/server/*.c ${SSCMA_ROOT_DIR}/sscma/server/*.cpp)

file(GLOB_RECURSE CLIENT_SSCMA_SOURCES ${SSCMA_ROOT_DIR}/sscma/client/*.c ${SSCMA_ROOT_DIR}/sscma/client/*.cpp)

file(GLOB_RECURSE PORT_SSCMA_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/posix/*.c ${SSCMA_ROOT_DIR}/sscma/porting/posix/*.cpp ${SSCMA_ROOT_DIR}/sscma/porting/osal/ma_osal_pthread.cpp)

set(EXTENSION_SSCMA_SOURCES "")

# if(SSCMA_EXTENSION_BYTETRACK)
#     file(GLOB_RECURSE BYTETRACK_SSCMA_SOURCES ${SSCMA_ROOT_DIR}/sscma/extension/bytetrack/*.c ${SSCMA_ROOT_DIR}/sscma/extension/bytetrack/*.cpp)
#     list(APPEND EXTENSION_SSCMA_SOURCES ${BYTETRACK_SSCMA_SOURCES})
# endif()

set(SSCMA_SOURCES
    ${CORE_SSCMA_SOURCES}
    ${SREVER_SSCMA_SOURCES}
    ${CLIENT_SSCMA_SOURCES}
    ${PORT_SSCMA_SOURCES}
    ${EXTENSION_SSCMA_SOURCES}
   )

set(SSCMA_INCLUDES 
    ${SSCMA_ROOT_DIR}/sscma
    ${SSCMA_ROOT_DIR}/sscma/porting/posix
)

if(SSCMA_SHARED_LIB)
    add_library(sscma SHARED ${SSCMA_SOURCES})
else()
    add_library(sscma STATIC ${SSCMA_SOURCES})
endif()

target_include_directories(sscma PUBLIC ${SSCMA_INCLUDES})

if(SSCMA_CONF_PATH)
    target_include_directories(sscma PUBLIC ${SSCMA_CONF_DIR})
endif()

if(SSCMA_ENGINE_TFLITE)
    include(${SSCMA_ROOT_DIR}/3rdparty/tflite-micro/CMakeLists.txt)
    include(${SSCMA_ROOT_DIR}/3rdparty/json/CMakeLists.txt)
    target_link_libraries(sscma PUBLIC tflm)
    target_link_libraries(sscma PUBLIC cjson)
endif()

if (SSCMA_FILESYSTEM_LITTLEFS)
    include(${SSCMA_ROOT_DIR}/3rdparty/littlefs/CMakeLists.txt)
    target_link_libraries(sscma PUBLIC littlefs)
endif()

if(SSCMA_EXTENSION_BYTETRACK)
    set(SSCMA_REQUIRE_LIB_EIGEN 1)
endif()

if(SSCMA_REQUIRE_LIB_EIGEN)
    include(${SSCMA_ROOT_DIR}/3rdparty/eigen/CMakeLists.txt)
    target_link_libraries(sscma PUBLIC Eigen3::Eigen)
endif()
