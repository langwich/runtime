cmake_minimum_required(VERSION 3.2)
project(runtime)

set(MODULE_NAME malloc)

add_subdirectory(lib)
add_subdirectory(freelist)
add_subdirectory(bitmap)
add_subdirectory(bytemap)
add_subdirectory(binning)
add_subdirectory(merging)
