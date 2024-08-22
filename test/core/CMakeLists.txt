set(srcs)
set(include_dirs)
set(files)

list(APPEND srcs
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
)

list(APPEND include_dirs
    ${CMAKE_CURRENT_LIST_DIR}/
)


list(APPEND files
    ${CMAKE_CURRENT_LIST_DIR}/yolo_meter.tflite
    ${CMAKE_CURRENT_LIST_DIR}/mnist.tflite
    ${CMAKE_CURRENT_LIST_DIR}/meter.bmp 
)


add_test(
        NAME test_core 
        SRCS ${srcs}
        INCLUDE_DIRS ${include_dirs}
        
        EMBED_FILES ${files})
