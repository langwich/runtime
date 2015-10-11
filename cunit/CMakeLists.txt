cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME cunit)
set(SOURCE cunit.c replay.c)

add_library(${MODULE_NAME} ${SOURCE})
INSTALL_LIBRARY(${MODULE_NAME})
