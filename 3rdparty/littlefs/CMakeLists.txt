cmake_minimum_required(VERSION 3.8.2)

set(component_hash "d01280e64934a09ba16cac60cf9d3a37e228bb66")

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/.component_hash")
    set(fetch_script "${SSCMA_ROOT_DIR}/scripts/fetch_littlefs.sh")

    message(STATUS "component_hash: ${component_hash}")
    message(STATUS "fetch_script: ${fetch_script}")

    execute_process(
        COMMAND ${fetch_script} ${component_hash} ${CMAKE_CURRENT_LIST_DIR}
        RESULT_VARIABLE result
    )

    if(NOT result EQUAL "0")
        message(FATAL_ERROR "fetch littlefs source failed")
    endif()

    file(GLOB PATCHES "${CMAKE_CURRENT_LIST_DIR}/patches/*.patch")

    foreach(patch IN LISTS PATCHES)
        message(STATUS "patch: ${patch}")
        execute_process(
            COMMAND git apply ${patch}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/littlefs
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL "0")
            message(FATAL_ERROR "git apply failed")
        endif()
    endforeach()

endif()


set(LITTLEFS_DIR "${CMAKE_CURRENT_LIST_DIR}/littlefs")

set(LITTLEFS_INCLUDE_DIRS "${LITTLEFS_DIR}" "${LITTLEFS_DIR}")

set(INCS_LITTLEFS "${LITTLEFS_INCLUDE_DIRS}")

set(SRCS_LITTLEFS
    "${LITTLEFS_DIR}/lfs.c"
    "${LITTLEFS_DIR}/lfs_util.c"
    "${LITTLEFS_DIR}/bd/lfs_emubd.c"
    "${LITTLEFS_DIR}/bd/lfs_filebd.c"
    "${LITTLEFS_DIR}/bd/lfs_rambd.c"
)

set(DEFS_LITTLEFS "")
if(SSCMA_FILESYSTEM_LITTLEFS_THREADSAFE)
    list(APPEND DEFS_LITTLEFS "-DLFS_THREADSAFE")
endif()

if(POSIX_PLATFORM)
add_library(littlefs STATIC ${SRCS_LITTLEFS})
target_include_directories(littlefs PUBLIC ${INCS_LITTLEFS})
target_compile_definitions(littlefs PUBLIC ${DEFS_LITTLEFS})
set_target_properties(littlefs PROPERTIES LINKER_LANGUAGE C)
endif()

message(STATUS "INCS_LITTLEFS: ${INCS_LITTLEFS}")

