cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_STANDARD 23)

project(TestPlugin VERSION 0.1)

add_library(TestPlugin SHARED src/main.cpp)

set_target_properties(TestPlugin PROPERTIES
    VERSION 1.0
    PUBLIC_HEADER "include/main.h"
)
