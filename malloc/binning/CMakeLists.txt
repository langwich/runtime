cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME binning)
set(SOURCE src/binning.c)
set(TEST_TARGETS binning_basic_test binning_test_trace)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common)

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})