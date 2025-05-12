cmake_minimum_required(VERSION 3.10)
project(PioneerV4 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -pg -Wall)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-g -pg)
endif()

add_executable(PioneerV4 ${SOURCES})