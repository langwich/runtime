cmake_minimum_required(VERSION 3.3)
project(runtime)

set(MODULE_NAME wlib)
set(SOURCE src/wich.c src/persistent_vector.c)

set(TEST_TARGETS persistent_vec)

add_library(${MODULE_NAME} ${SOURCE})
set_target_properties(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-DPLAIN")
INSTALL_LIBRARY(${MODULE_NAME})

add_library("${MODULE_NAME}_refcounting" ${SOURCE} src/refcounting.c)
set_target_properties("${MODULE_NAME}_refcounting" PROPERTIES COMPILE_FLAGS "-DREFCOUNTING")
INSTALL_LIBRARY("${MODULE_NAME}_refcounting")

add_library("${MODULE_NAME}_mark_and_compact" ${SOURCE})
set_target_properties("${MODULE_NAME}_mark_and_compact" PROPERTIES COMPILE_FLAGS "-DMARK_AND_COMPACT")
INSTALL_LIBRARY("${MODULE_NAME}_mark_and_compact")

add_library("${MODULE_NAME}_mark_and_sweep" ${SOURCE})
set_target_properties("${MODULE_NAME}_mark_and_sweep" PROPERTIES COMPILE_FLAGS "-DMARK_AND_SWEEP")
INSTALL_LIBRARY("${MODULE_NAME}_mark_and_sweep")

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})