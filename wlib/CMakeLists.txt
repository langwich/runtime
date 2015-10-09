cmake_minimum_required(VERSION 3.3)
project(runtime)

set(MODULE_NAME wlib)
set(SOURCE src/wich.c)

add_library(${MODULE_NAME} ${SOURCE})

target_include_directories (${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(wlib cunit)
