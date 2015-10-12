cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME mark-and-compact)
set(SOURCE src/gc.c)
#set(TEST_TARGETS test_basics test_ptr_fields)
set(TEST_TARGETS test_basics)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc-common)

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET(${TEST_TARGETS} ${MODULE_NAME})
