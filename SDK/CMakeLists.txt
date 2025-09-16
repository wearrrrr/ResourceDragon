cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)

project(ResourceDragon_SDK VERSION 0.1)

add_library(ResourceDragon_SDK STATIC sdk.cpp)

set_target_properties(ResourceDragon_SDK PROPERTIES
    VERSION 1.0
)
