cmake_minimum_required(VERSION 3.8.2)

set(component_hash "3147391d946bb4b6c68edd901f2add6ac1f31f8c")

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/.component_hash")
    set(fetch_script "${SSCMA_ROOT_DIR}/scripts/fetch_eigen.sh")

    message(STATUS "component_hash: ${component_hash}")
    message(STATUS "fetch_script: ${fetch_script}")

    execute_process(
        COMMAND ${fetch_script} ${component_hash} ${CMAKE_CURRENT_LIST_DIR}
        RESULT_VARIABLE result
    )

    if(NOT result EQUAL "0")
        message(FATAL_ERROR "fetch eigen source failed")
    endif()

    file(GLOB PATCHES "${CMAKE_CURRENT_LIST_DIR}/patches/*.patch")

    foreach(patch IN LISTS PATCHES)
        message(STATUS "patch: ${patch}")
        execute_process(
            COMMAND git apply ${patch}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/eigen
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL "0")
            message(FATAL_ERROR "git apply failed")
        endif()
    endforeach()

endif()


set(EIGEN_DIR "${CMAKE_CURRENT_LIST_DIR}/eigen")

set(EIGEN_INCLUDE_DIRS "${EIGEN_DIR}" "${EIGEN_DIR}/Eigen")

set(INCS_EIGEN "${EIGEN_INCLUDE_DIRS}")

set(SRCS_EIGEN "")

if(POSIX_PLATFORM)
add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${EIGEN_INCLUDE_DIRS}"
)
endif()

message(STATUS "INCS_EIGEN: ${INCS_EIGEN}")


