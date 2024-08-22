cmake_minimum_required(VERSION 3.12.4)

file(GLOB srcs "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_test(
        NAME test_math
        SRCS ${srcs}
        INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}
        EMBED_FILES ${files})
