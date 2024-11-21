cmake_minimum_required(VERSION 3.8.2)

set(component_hash "acc76239bee01d8e9c858ae2cab296704e52d916")

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/.component_hash")
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

    file(GLOB PATCHES "${CMAKE_CURRENT_LIST_DIR}/patches/*.patch")

    foreach(patch IN LISTS PATCHES)
        message(STATUS "patch: ${patch}")
        execute_process(
            COMMAND git apply ${patch}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/cJSON
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL "0")
            message(FATAL_ERROR "git apply failed")
        endif()
    endforeach()

endif()



set(CJSON_DIR "${CMAKE_CURRENT_LIST_DIR}/cJSON")
file(GLOB SRCS_CJSON
    "${CJSON_DIR}/cJSON.c"
    "${CJSON_DIR}/cJSON_Utils.c"
)

set(INCS_CJSON
    ${CJSON_DIR}
)

if(POSIX_PLATFORM)
add_library(cjson STATIC ${SRCS_CJSON})
target_include_directories(cjson PUBLIC ${INCS_CJSON})
endif()

message(STATUS "INCS_CJSON: ${INCS_CJSON}")

