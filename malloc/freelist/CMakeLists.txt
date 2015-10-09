cmake_minimum_required(VERSION 3.2)
project(malloc)

set(MODULE_NAME freelist)

BUILD_MODULE(${MODULE_NAME} OFF)

ADD_TEST_TARGET(test_basic_sanity ${MODULE_NAME})
ADD_TEST_TARGET(test_size_adjust ${MODULE_NAME})
ADD_TEST_TARGET(test_replay ${MODULE_NAME})
