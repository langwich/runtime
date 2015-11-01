cmake_minimum_required(VERSION 3.3)
project(runtime)

enable_testing()

# common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -g")

# setup installation path
set(INSTALL_NAME wich)
set(INSTALL_DIR "/usr/local/${INSTALL_NAME}")
set(CMAKE_INSTALL_PREFIX ${INSTALL_DIR})

set(ENV{WICHRUNTIME} ${CMAKE_SOURCE_DIR})

################# I n c l u d e s #################

# the INCLUDE_DIRECTORIES property needs to be
# inherited by subdirectories
set(INCLUDE_DIRS
		cunit
		gc
		gc/mark_and_compact/src
		gc/mark_and_sweep/src
		gc/scavenger/src
		malloc
		malloc/lib/src
		malloc/freelist/src
		malloc/bitmap/src
		malloc/bytemap/src
		malloc/binning/src
		malloc/merging/src
		vm/src
		wlib/src)
include_directories(${INCLUDE_DIRS})
# install all headers in directories listed above
foreach(INCLUDE_DIR ${INCLUDE_DIRS})
	file(GLOB HEADERS ${INCLUDE_DIR}/*.h)
	install(FILES ${HEADERS}
			DESTINATION include)
endforeach()

################# F u n c t i o n s #################

# use this if you want to create test executable
# TEST_SOURCE could be a list of test source files
# NOTE: assuming all test files are in the "test" folder!!
function(ADD_TEST_TARGET TARGET_NAMES MODULE_NAME)
	foreach(TARGET_NAME ${TARGET_NAMES})
		add_executable(${TARGET_NAME} test/${TARGET_NAME}.c)
		target_link_libraries(${TARGET_NAME} ${MODULE_NAME} cunit)
		if(NOT ${ARGC} GREATER 2)
			add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME}
					WORKING_DIRECTORY /tmp/wich-build/malloc/)
		endif()
	endforeach()
endfunction(ADD_TEST_TARGET)

# use this if you want to install a library
# to /usr/local/wich/lib
function(INSTALL_LIBRARY LIB_NAME)
	install(TARGETS ${LIB_NAME}
			ARCHIVE DESTINATION lib)
endfunction()

################# S u b d i r s #################

# add all subdirectories
set(SUB_DIRS malloc wlib cunit gc vm)
foreach(SUB_DIR ${SUB_DIRS})
	add_subdirectory(${SUB_DIR})
endforeach()