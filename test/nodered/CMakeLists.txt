cmake_minimum_required(VERSION 3.12.4)

file(GLOB srcs "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/node/*.cpp")

set(incs
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/node
)

add_test(
        NAME test_node
        SRCS ${srcs}
        INCLUDE_DIRS ${incs}
        EMBED_FILES ${files})
