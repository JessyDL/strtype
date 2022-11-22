cmake_minimum_required(VERSION 3.19)
set(STRTYPE_PROJECT strtype CACHE INTERNAL "")
project(${STRTYPE_PROJECT} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF )
set(CXX_EXTENSIONS OFF )
set(CMAKE_CXX_STANDARD 20)

OPTION(STRTYPE_TESTS "enables the tests" OFF)

add_library(${STRTYPE_PROJECT} INTERFACE ${PROJECT_SOURCE_DIR}/include/strtype/strtype.hpp)
target_include_directories(${STRTYPE_PROJECT} INTERFACE ${PROJECT_SOURCE_DIR}/include)

if(${STRTYPE_TESTS})
    add_subdirectory(tests)
endif()