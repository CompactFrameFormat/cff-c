cmake_minimum_required(VERSION 3.10)
project(cff_usage_example C)

# Set C standard
set(CMAKE_C_STANDARD 99)

# Add the CFF library source files
set(CFF_SOURCES
    ../src/cff.c
    ../src/cff.h
)

# Create the example executable
add_executable(usage_example 
    usage_example.c
    ${CFF_SOURCES}
)

# Include directories
target_include_directories(usage_example PRIVATE ../src)

# Enable warnings
if(MSVC)
    target_compile_options(usage_example PRIVATE /W4)
else()
    target_compile_options(usage_example PRIVATE -Wall -Wextra -pedantic)
endif()

# Set output directory
set_target_properties(usage_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
) 