cmake_minimum_required(VERSION 3.10)
project(PioneerV4 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g)
endif()


file(GLOB_RECURSE SOURCES "src/*.cpp")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -flto)
endif()

add_executable(PioneerV4 ${SOURCES})