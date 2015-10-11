cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME lib)
set(SOURCE src/morecore.c src/bitset.c)
set(TEST_TARGETS test_bitset_basics)

add_library(${MODULE_NAME} ${SOURCE})
set_target_properties(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-mpopcnt -mlzcnt -O0 -DDEBUG")
target_link_libraries(lib)

INSTALL_LIBRARY(${MODULE_NAME})
ADD_TEST_TARGET(${TEST_TARGETS} ${MODULE_NAME})

