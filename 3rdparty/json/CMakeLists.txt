cmake_minimum_required(VERSION 3.8.2)

set(component_hash "acc76239bee01d8e9c858ae2cab296704e52d916")
set(fetch_script "${SSCMA_ROOT_DIR}/scripts/fetch_cjson.sh")

message(STATUS "component_hash: ${component_hash}")
message(STATUS "fetch_script: ${fetch_script}")

execute_process(
    COMMAND ${fetch_script} ${component_hash}
    RESULT_VARIABLE result
)

if(NOT result EQUAL "0")
    message(FATAL_ERROR "fetch_script.sh failed")
endif()

set(CJSON_DIR "${CMAKE_CURRENT_LIST_DIR}/cJSON")
file(GLOB SRCS_CJSON
    "${CJSON_DIR}/cJSON.c"
    "${CJSON_DIR}/cJSON_Utils.c"
)

set(INCS_CJSON
    ${CJSON_DIR}
)

if(SG200X_PLATFORM)
component_register(
    COMPONENT_NAME cjson
    SRCS ${SRCS_CJSON}
    INCLUDE_DIRS ${INCS_CJSON}
)
else()
add_library(cjson STATIC ${SRCS_CJSON})
target_include_directories(cjson PUBLIC ${INCS_CJSON})
message(STATUS "INCS_CJSON: ${INCS_CJSON}")
endif()

