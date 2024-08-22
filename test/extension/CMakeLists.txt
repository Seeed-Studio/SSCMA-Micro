cmake_minimum_required(VERSION 3.12.4)
set(srcs)
set(incs)
set(files)

file(GLOB srcs "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_test(
        NAME test_extension
        SRCS ${srcs}
        INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}
        EMBED_FILES ${CMAKE_CURRENT_LIST_DIR}/test_data)
