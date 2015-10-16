cmake_minimum_required(VERSION 3.2)
project(runtime)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DMARK_AND_COMPACT -Wall")

set(MODULE_NAME mark_and_compact)
set(SOURCE src/mark_and_compact.c test/bitset.c)
set(TEST_TARGETS test_ptr_fields test_basics test_random_graph)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common wlib gc_mark_and_compact)

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})
