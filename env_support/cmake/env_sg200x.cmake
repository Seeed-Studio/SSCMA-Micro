file(GLOB_RECURSE SOURCES ${SSCMA_ROOT_DIR}/sscma/*.c ${SSCMA_ROOT_DIR}/sscma/*.cpp ${LVGL_ROOT_DIR}/sscma/*.S)


add_compile_options(-DCONFIG_MA_ENGINE_CVINN=1)

component_register(
    COMPONENT_NAME sscma
    SRCS ${SOURCES}
    PRIVATE_INCLUDE_DIRS ${SSCMA_ROOT_DIR}/sscma
    PRIVATE_REQUIREDS cviruntime
)