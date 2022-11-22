cmake_minimum_required(VERSION 3.19)
set(STRTYPE_TEST_PROJECT strtype_tests CACHE INTERNAL "")
project(${STRTYPE_TEST_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY "https://github.com/catchorg/Catch2.git"
  GIT_TAG v3.2.0
)
FetchContent_MakeAvailable(catch2)

add_executable(${STRTYPE_TEST_PROJECT} tests.cpp)
target_link_libraries(${STRTYPE_TEST_PROJECT} PUBLIC strtype Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${STRTYPE_TEST_PROJECT})