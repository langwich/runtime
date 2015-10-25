cmake_minimum_required(VERSION 3.2)
project(runtime)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DSCAVENGER -Wall")

set(MODULE_NAME scavenger)
set(SOURCE src/scavenger.c test/bitset.c)
set(TEST_TARGETS sc_test_basics sc_test_ptr_fields sc_test_random_graph)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common gc_scavenger)

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})
