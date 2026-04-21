cmake_minimum_required(VERSION 3.16)
project(problem_064 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(code src/main.cpp)
target_compile_features(code PRIVATE cxx_std_20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(code PRIVATE -O2 -pipe -s -fno-omit-frame-pointer)
endif()
