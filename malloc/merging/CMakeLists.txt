cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME merging)
set(SOURCE src/merging.c)

add_library(${MODULE_NAME} ${SOURCE})
target_link_libraries(${MODULE_NAME} malloc_common)

INSTALL_LIBRARY(${MODULE_NAME})
