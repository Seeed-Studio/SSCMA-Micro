cmake_minimum_required(VERSION 3.12.4)

file(GLOB test_porting_posix_srcs "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

set(MA_TEST_PORTING_POSIX_SRCS ${test_porting_posix_srcs})
