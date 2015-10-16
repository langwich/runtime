cmake_minimum_required(VERSION 3.2)
project(runtime)

if(SINGLE_MODE)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lglapi")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lglapi")
endif(SINGLE_MODE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")

set(MODULE_NAME gc)
set(SOURCE gc.c)

add_subdirectory(mark_and_compact)
add_subdirectory(mark_and_sweep)

add_library("${MODULE_NAME}_mark_and_compact" ${SOURCE})
set_target_properties("${MODULE_NAME}_mark_and_compact" PROPERTIES COMPILE_FLAGS "-DMARK_AND_COMPACT")
INSTALL_LIBRARY("${MODULE_NAME}_mark_and_compact")

add_library("${MODULE_NAME}_mark_and_sweep" ${SOURCE})
set_target_properties("${MODULE_NAME}_mark_and_sweep" PROPERTIES COMPILE_FLAGS "-DMARK_AND_SWEEP")
INSTALL_LIBRARY("${MODULE_NAME}_mark_and_sweep")
