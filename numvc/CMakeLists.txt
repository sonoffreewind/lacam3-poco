cmake_minimum_required(VERSION 3.16)
project(numvc)

find_package(Threads REQUIRED)

# Directly specify source files as they are in the current directory
set(SRCS numvc.cpp)

add_library(${PROJECT_NAME} STATIC ${SRCS})

# Set compile options based on the build type
target_compile_options(${PROJECT_NAME} PUBLIC
    $<$<CONFIG:Debug>:-g -O0 -Wall>          # Debug: No optimization + debug info
    $<$<CONFIG:Release>:-O3 -Wall -DNDEBUG>  # Release: Maximum optimization
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Header files are in the current directory
target_include_directories(${PROJECT_NAME} PUBLIC .)


target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
