cmake_minimum_required(VERSION 3.8.2)

set(FLASHDB_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


file(GLOB SRCS_FLASHDB
    "${FLASHDB_DIR}/*.c"
    "${FLASHDB_DIR}/*.cpp"
)

set(INCS_FLASHDB
    "${FLASHDB_DIR}"
    CACHE INTERNAL "flashdb include directories"
)

add_library(flashdb STATIC ${SRCS_FLASHDB})
target_include_directories(flashdb PRIVATE ${INCS_FLASHDB})
