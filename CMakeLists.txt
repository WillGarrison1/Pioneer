cmake_minimum_required(VERSION 3.10)
project(PioneerV4 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -mbmi2)
endif()


file(GLOB_RECURSE SOURCES "src/*.cpp")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native -mtune=native -flto -fno-exceptions -Wall -Wextra -Wcast-qual -mbmi2 -DNDEBUG -funroll-loops -fno-rtti)
    add_link_options(-flto -static -pthread -lstdc++ -Wl,--no-as-needed)
endif()

add_executable(PioneerV4 ${SOURCES})

add_custom_command(TARGET PioneerV4 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/nnue/bin/nnue.bin
        $<TARGET_FILE_DIR:PioneerV4>/nnue.bin
)