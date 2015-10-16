cmake_minimum_required(VERSION 3.2)
project(runtime)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DMARK_AND_SWEEP -Wall")

set(MODULE_NAME mark_and_sweep)
set(SOURCE src/mark_and_sweep.c)
set(TEST_TARGETS ms_test_basics ms_test_ptr_fields)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common gc_mark_and_sweep)

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})