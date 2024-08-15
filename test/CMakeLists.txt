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

set(srcs "")
set(include_dirs "")
set(private_include_dirs "")

set(SSCMA_ENGINE_TFLITE ON)

get_filename_component(SSCMA_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

message(STATUS "SSCMA_ROOT_DIR: ${SSCMA_ROOT_DIR}")

set(SSCMA_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/ma_config.h")

include(${SSCMA_ROOT_DIR}/CMakeLists.txt)

include(math/CMakeLists.txt)
include(misc/CMakeLists.txt)
include(utils/CMakeLists.txt)
include(extension/CMakeLists.txt)
include(model/CMakeLists.txt)
include(filesystem/CMakeLists.txt)

list(APPEND srcs
    ${CMAKE_CURRENT_LIST_DIR}/test_main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_os.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_codec.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_engine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_data.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_storage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_pipeline.cpp
)

list(APPEND srcs
    ${MA_TEST_MATH_SRCS}
    ${MA_TEST_MISC_SRCS}
    ${MA_TEST_UTILS_SRCS}
    ${MA_TEST_EXTENSION_SRCS}
    ${MA_TEST_MODEL_SRCS}
    ${MA_TEST_FILESYSTEM_SRCS}
)

list(APPEND include_dirs
    ${CMAKE_CURRENT_LIST_DIR}/
)

message(STATUS "private_include_dirs: ${private_include_dirs}")
message(STATUS "include_dirs: ${include_dirs}")
message(STATUS "srcs: ${srcs}")

file(COPY ${CMAKE_CURRENT_LIST_DIR}/yolo_meter.tflite DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/mnist.tflite DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/meter.bmp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} ${srcs})
target_include_directories(${PROJECT_NAME} PUBLIC ${include_dirs})
target_include_directories(${PROJECT_NAME} PRIVATE ${private_include_dirs})
target_link_libraries(${PROJECT_NAME} PRIVATE sscma)
target_link_libraries(${PROJECT_NAME} PRIVATE tflm)
target_link_libraries(${PROJECT_NAME} PRIVATE gtest)
