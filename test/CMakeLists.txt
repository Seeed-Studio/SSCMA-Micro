cmake_minimum_required(VERSION 3.20)

project(test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(SSCMA_ENGINE_TFLITE ON)

get_filename_component(SSCMA_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

message(STATUS "SSCMA_ROOT_DIR: ${SSCMA_ROOT_DIR}")

set(SSCMA_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/ma_config.h")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

include(${SSCMA_ROOT_DIR}/CMakeLists.txt)

set(TEST_MAIN ${CMAKE_CURRENT_LIST_DIR}/test_main.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

function(add_test)
    set(one_value_args NAME)
    set(multi_value_args SRCS INCLUDE_DIRS PRIVATE_INCLUDE_DIRS REQUIREDS PRIVATE_REQUIREDS LIBRARY_DIRS EMBED_FILES)

    cmake_parse_arguments(TEST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    message(STATUS "Adding ${TEST_NAME}")

    message(STATUS "private_include_dirs: ${TEST_PRIVATE_INCLUDE_DIRS}")
    message(STATUS "include_dirs: ${TEST_INCLUDE_DIRS}")
    message(STATUS "srcs: ${TEST_SRCS}")

    foreach(lib_dir IN LISTS TEST_LIBRARY_DIRS)
        message(STATUS "Linking ${lib_dir}")
        link_directories(${lib_dir})
    endforeach()

    foreach(inc IN LISTS TEST_INCLUDE_DIRS)
        message(STATUS "Including ${inc}")
        include_directories(${inc})
    endforeach()

    foreach(file IN LISTS TEST_EMBED_FILES)
        message(STATUS "Including ${file}")
        file(COPY ${file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)
    endforeach(file IN LISTS TEST_EMBED_FILES)
    
    
    if(DEFINED TEST_SRCS)
        add_executable(${TEST_NAME} ${TEST_SRCS} ${TEST_MAIN})
        set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
        target_include_directories(${TEST_NAME} PRIVATE ${TEST_PRIVATE_INCLUDE_DIRS})
        target_link_libraries(${TEST_NAME} PUBLIC ${TEST_REQUIREDS})
        target_link_libraries(${TEST_NAME} PUBLIC sscma tflm gtest mosquitto)
        target_link_libraries(${TEST_NAME} PRIVATE ${TEST_PRIVATE_REQUIREDS})
    endif()

endfunction()

include(core/CMakeLists.txt)
include(model/CMakeLists.txt)
include(math/CMakeLists.txt)
include(utils/CMakeLists.txt)
include(misc/CMakeLists.txt)
include(extension/CMakeLists.txt)
include(porting/posix/CMakeLists.txt)

