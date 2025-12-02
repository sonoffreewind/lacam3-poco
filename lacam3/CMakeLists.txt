cmake_minimum_required(VERSION 3.16)
project(lacam3)

find_package(Threads REQUIRED)

# =========================================================
# Create multiple libraries with different planner implementations
# =========================================================
set(LIBRARY_VARIANTS fixed base poco)

foreach(variant ${LIBRARY_VARIANTS})
    # Get source files and replace planner.cpp
    file(GLOB VARIANT_SRCS "./src/*.cpp")
    list(APPEND VARIANT_SRCS "./${variant}/planner.cpp")

    # Create library
    add_library(lacam3${variant} STATIC ${VARIANT_SRCS})

    # Configure library
    target_compile_options(lacam3${variant} PUBLIC
        $<$<CONFIG:Debug>:-g -O0 -Wall>
        $<$<CONFIG:Release>:-O3 -Wall -DNDEBUG>
    )

    target_compile_features(lacam3${variant} PUBLIC cxx_std_17)
    target_include_directories(lacam3${variant} PUBLIC ./include ./${variant})
    target_link_libraries(lacam3${variant} PUBLIC Threads::Threads)
endforeach()

# Create poco_lb variant with USE_MVC_LB macro
file(GLOB POCO_LB_SRCS "./src/*.cpp")
list(APPEND POCO_LB_SRCS "./poco/planner.cpp")
add_library(lacam3pocolb STATIC ${POCO_LB_SRCS})
target_compile_definitions(lacam3pocolb PRIVATE USE_MVC_LB)
target_compile_options(lacam3pocolb PUBLIC
    $<$<CONFIG:Debug>:-g -O0 -Wall>
    $<$<CONFIG:Release>:-O3 -Wall -DNDEBUG>
)
target_compile_features(lacam3pocolb PUBLIC cxx_std_17)
target_include_directories(lacam3pocolb PUBLIC ./include ./poco)
target_link_libraries(lacam3pocolb PUBLIC Threads::Threads)

target_include_directories(lacam3poco PRIVATE ${CMAKE_SOURCE_DIR}/poco/include ${CMAKE_SOURCE_DIR}/numvc)
target_include_directories(lacam3pocolb PRIVATE ${CMAKE_SOURCE_DIR}/poco/include ${CMAKE_SOURCE_DIR}/numvc)