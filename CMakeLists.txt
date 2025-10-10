cmake_minimum_required(VERSION 3.12)
project(MINM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/init.cpp
    src/main.cpp
)

include_directories(include)

add_executable(minm ${SOURCES})

target_compile_features(minm PRIVATE cxx_std_17)
target_include_directories(minm PRIVATE include)

set_target_properties(minm PROPERTIES
    CXX_EXTENSIONS OFF
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(minm PRIVATE -O2 -DNDEBUG)
else()
    target_compile_options(minm PRIVATE -g -O0)
endif()