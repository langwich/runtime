cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME mark-and-compact)
set(SOURCE src/gc.c test/test_vec.c)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} cunit lib)

include_directories(src)

add_executable(test_vec test/test_vec.c)
target_link_libraries(test_vec mark-and-compact)

target_include_directories (${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
