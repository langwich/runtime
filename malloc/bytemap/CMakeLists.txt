cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME bytemap)
set(SOURCE src/bytemap.c)
set(TEST_TARGETS bytemap_basic_tests bytemap_replay)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common)
set_target_properties(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-DDEBUG")

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME} OFF)

