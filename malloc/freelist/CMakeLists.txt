cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME freelist)
set(SOURCE src/freelist.c)
set(TEST_TARGETS test_basic_sanity test_replay test_size_adjust)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} lib)

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})

