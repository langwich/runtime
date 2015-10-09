cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME freelist)
set(SOURCE src/freelist.c)

add_library(${MODULE_NAME} ${SOURCE})

target_include_directories (${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(freelist cunit lib)
