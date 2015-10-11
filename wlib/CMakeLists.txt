cmake_minimum_required(VERSION 3.3)
project(runtime)

set(MODULE_NAME wlib)
set(SOURCE src/wich.c)

add_library(${MODULE_NAME} ${SOURCE})

INSTALL_LIBRARY(${MODULE_NAME})
