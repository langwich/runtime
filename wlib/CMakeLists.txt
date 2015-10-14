cmake_minimum_required(VERSION 3.3)
project(runtime)

set(MODULE_NAME wlib)
set(SOURCE src/wich.c src/persistent_vector.c)

set(TEST_TARGETS persistent_vec)

add_library(${MODULE_NAME} ${SOURCE})

INSTALL_LIBRARY(${MODULE_NAME})

ADD_TEST_TARGET("${TEST_TARGETS}" ${MODULE_NAME})