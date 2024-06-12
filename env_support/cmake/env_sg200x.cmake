
file(GLOB_RECURSE CORE_SOURCES ${SSCMA_ROOT_DIR}/sscma/core/*.c ${SSCMA_ROOT_DIR}/sscma/core/*.cpp)

file(GLOB_RECURSE SREVER_SOURCES ${SSCMA_ROOT_DIR}/sscma/server/*.c ${SSCMA_ROOT_DIR}/sscma/server/*.cpp)

file(GLOB_RECURSE CLIENT_SOURCES ${SSCMA_ROOT_DIR}/sscma/client/*.c ${SSCMA_ROOT_DIR}/sscma/client/*.cpp)

file(GLOB_RECURSE PORT_SOURCES ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/*.c ${SSCMA_ROOT_DIR}/sscma/porting/sophgo/*.cpp)

set(SOURCES ${CORE_SOURCES} ${SREVER_SOURCES} ${CLIENT_SOURCES} ${PORT_SOURCES})

set(INCS 
        ${SSCMA_ROOT_DIR}/sscma
        ${SSCMA_ROOT_DIR}/sscma/porting/sophgo
)

add_compile_options(-DCONFIG_MA_ENGINE_CVINN=1)

include(${SSCMA_ROOT_DIR}/3rdparty/json/CMakeLists.txt)

component_register(
    COMPONENT_NAME sscma
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCS}
    PRIVATE_REQUIREDS cviruntime cjson
)