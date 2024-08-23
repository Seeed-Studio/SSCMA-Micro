
file(GLOB_RECURSE CORE_SOURCES ${SSCMA_ROOT_DIR}/sscma/core/*.c ${SSCMA_ROOT_DIR}/sscma/core/*.cpp)

file(GLOB_RECURSE SREVER_SOURCES ${SSCMA_ROOT_DIR}/sscma/server/*.c ${SSCMA_ROOT_DIR}/sscma/server/*.cpp)

file(GLOB_RECURSE CLIENT_SOURCES ${SSCMA_ROOT_DIR}/sscma/client/*.c ${SSCMA_ROOT_DIR}/sscma/client/*.cpp)

file(GLOB_RECURSE PORT_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/posix/*.c ${SSCMA_ROOT_DIR}/sscma/porting/posix/*.cpp  ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/*.c ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/*.cpp  ${SSCMA_ROOT_DIR}/sscma/porting/osal/ma_osal_pthread.cpp)

file(GLOB_RECURSE BOARD_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/sg200x/*.c ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/sg200x/*.cpp)

set(SOURCES ${CORE_SOURCES} ${SREVER_SOURCES} ${CLIENT_SOURCES} ${PORT_SOURCES})

set(INCS 
        ${SSCMA_ROOT_DIR}/sscma
        ${SSCMA_ROOT_DIR}/sscma/porting/posix
        ${SSCMA_ROOT_DIR}/sscma/porting/sophgo
        ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/sg200x
        ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/sg200x/recamera
)

add_compile_options(-DCONFIG_MA_ENGINE_CVINN=1)

include(${SSCMA_ROOT_DIR}/3rdparty/json/CMakeLists.txt)
include(${SSCMA_ROOT_DIR}/3rdparty/JPEGENC/CMakeLists.txt)

component_register(
    COMPONENT_NAME sscma
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCS}
    PRIVATE_REQUIREDS cjson mosquitto ssl crypto cviruntime video jpegenc
) 