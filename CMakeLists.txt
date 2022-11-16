cmake_minimum_required(VERSION 3.19)
set(STRENUM_PROJECT strenum CACHE INTERNAL "")
project(${STRENUM_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF )
set(CXX_EXTENSIONS OFF )
set(CMAKE_CXX_STANDARD 20)

OPTION(STRENUM_TESTS "enables the tests" ON)

add_library(${STRENUM_PROJECT} INTERFACE ${PROJECT_SOURCE_DIR}/include/strenum/strenum.hpp)
target_include_directories(${STRENUM_PROJECT} INTERFACE ${PROJECT_SOURCE_DIR}/include)

if(${STRENUM_TESTS})
    add_subdirectory(tests)
endif()