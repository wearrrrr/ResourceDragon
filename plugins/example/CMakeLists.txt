cmake_minimum_required(VERSION 3.10)

project(ExamplePlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)

# Find the SDK headers
set(SDK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../SDK)

# Create the plugin as a shared library
add_library(example_plugin SHARED
    example_plugin.cpp
)

# Set include directories
target_include_directories(example_plugin PRIVATE
    ${SDK_INCLUDE_DIR}
)

# Set output properties
set_target_properties(example_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "example_plugin"
)

# set_target_properties(example_plugin PROPERTIES
#     COMPILE_FLAGS "-g"
# )

# Platform-specific settings
if(WIN32)
    set_target_properties(example_plugin PROPERTIES
        SUFFIX ".dll"
    )
else()
    set_target_properties(example_plugin PROPERTIES
        SUFFIX ".so"
    )
endif()

# Set output directory to the plugins folder
set_target_properties(example_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../plugins
)
