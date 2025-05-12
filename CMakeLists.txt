cmake_minimum_required(VERSION 3.10)
project(PioneerV4 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_compile_options(-g)

file(GLOB_RECURSE SOURCES "src/*.cpp")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -flto -mavx512f -funroll-loops -march=native)
    add_link_options(-fprofile-use)
endif()

add_executable(PioneerV4 ${SOURCES})