cmake_minimum_required(VERSION 3.0)

project(test)



set(srcs "")
set(include_dirs "")
set(private_include_dirs "")

set(SSCMA_ENGINE_TFLITE ON)

get_filename_component(SSCMA_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

message(STATUS "SSCMA_ROOT_DIR: ${SSCMA_ROOT_DIR}")

set(SSCMA_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/ma_config.h")

include(${SSCMA_ROOT_DIR}/CMakeLists.txt)

list(APPEND srcs 
    ${CMAKE_CURRENT_LIST_DIR}/test_main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_codec.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_engine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_data.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_server.cpp
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
target_link_libraries(${PROJECT_NAME} PRIVATE gtest)

