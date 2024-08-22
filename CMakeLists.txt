cmake_minimum_required(VERSION 3.12.4)

if(NOT DEFINED SSCMA_ROOT_DIR)
set(SSCMA_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

if(ESP_PLATFORM)
include(${SSCMA_ROOT_DIR}/env_support/cmake/env_esp.cmake)
elseif(SOPHGO_PLATFORM)
include(${SSCMA_ROOT_DIR}/env_support/cmake/env_sophgo.cmake)
else()
include(${SSCMA_ROOT_DIR}/env_support/cmake/env_posix.cmake)
endif()
