cmake_minimum_required(VERSION 3.19)
set(STRENUM_TEST_PROJECT strenum_tests CACHE INTERNAL "")
project(${STRENUM_TEST_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

add_executable(${STRENUM_TEST_PROJECT} tests.cpp)
target_link_libraries(${STRENUM_TEST_PROJECT} PUBLIC strenum)