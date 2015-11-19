cmake_minimum_required(VERSION 3.2)
project(runtime)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DMARK_AND_COMPACT -Wall")

set(MODULE_NAME vm)
set(SOURCE src/vm.c src/wloader.c src/wrun.c)
set(TEST_TARGETS test_vm test_vm_samples)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common mark_and_compact gc_mark_and_compact wlib_mark_and_compact)
INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})
