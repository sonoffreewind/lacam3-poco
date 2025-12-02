cmake_minimum_required(VERSION 3.16)
project(lacam3-poco-numvc)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(./numvc)
add_subdirectory(./third_party/argparse)
add_subdirectory(./poco)
add_subdirectory(./lacam3)

set(LIBRARY_VARIANTS fixed base poco poco_lb)

foreach(variant ${LIBRARY_VARIANTS})
  add_executable(lacam3_${variant} main.cpp)
  target_compile_features(lacam3_${variant} PUBLIC cxx_std_17)
  target_compile_options(lacam3_${variant} PRIVATE -Wall -Wextra)
  target_compile_options(lacam3_${variant} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall>
    $<$<CONFIG:Release>:-O3 -Wall -DNDEBUG>
  )
  target_link_options(lacam3_${variant} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -fsanitize=leak>)
endforeach()

target_link_libraries(lacam3_fixed lacam3fixed argparse)
target_link_libraries(lacam3_base  lacam3base  argparse)
target_link_libraries(lacam3_poco  lacam3poco poco numvc argparse)
target_link_libraries(lacam3_poco_lb  lacam3pocolb poco numvc argparse)

# -------------------------------------------------------
# Tests
# -------------------------------------------------------
enable_testing()
file(GLOB TEST_FILES "./tests/test_*.cpp")
foreach(file ${TEST_FILES})
  string(REGEX MATCH "test\_[^\.]+" name "${file}")
  add_executable(${name} ${file})
  # Link to the standard lacam3 library
  target_link_libraries(${name} lacam3poco poco numvc argparse)
  add_test(${name} ${name})
endforeach()
