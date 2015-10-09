cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME lib)
set(SOURCE morecore.c bitset.c test_bitset_basics.c)

add_library(${MODULE_NAME} ${SOURCE})

set_target_properties(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-mpopcnt -mlzcnt -O0 -DDEBUG")

target_include_directories (${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(lib cunit)
