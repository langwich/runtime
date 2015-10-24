cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME bitmap)
set(SOURCE src/bitmap.c)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common)
set_target_properties(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-DDEBUG")

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET(bitmap_basic_tests ${MODULE_NAME})
ADD_TEST_TARGET(bitmap_replay ${MODULE_NAME} OFF)

