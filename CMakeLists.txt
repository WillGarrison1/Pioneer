cmake_minimum_required(VERSION 3.10)
project(Pioneer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Without this, a plain `cmake ..` matches neither branch below and the engine builds with no
# optimization flags at all -- no -O3, no -march, no -flto.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    message(STATUS "No CMAKE_BUILD_TYPE specified; defaulting to Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -mbmi2 -mssse3)
endif()


file(GLOB_RECURSE SOURCES "src/*.cpp")

# Optimized build that keeps asserts live. Useful for validating search invariants (ply bounds,
# accumulator indices) at close-to-real speed and search depth -- a plain Debug build is far too
# slow to reach the depths where those invariants actually get stressed.
option(PIONEER_ASSERTS "Optimized build with asserts enabled" OFF)

# Profile-guided optimization. PGO is a two-stage process:
#   1. GENERATE -- build an instrumented binary, run a representative workload (`bench`), which
#      writes profile counters to disk on clean exit.
#   2. USE      -- rebuild feeding those counters back in, so the compiler knows which branches
#      are hot, what to inline, and how to lay out code.

set(PIONEER_PGO "OFF" CACHE STRING "PGO stage: OFF, GENERATE, or USE")
set_property(CACHE PIONEER_PGO PROPERTY STRINGS OFF GENERATE USE)
set(PIONEER_PGO_DIR "${CMAKE_BINARY_DIR}/pgo-data" CACHE PATH "Where Clang PGO profile data lives")
set(PIONEER_PGO_EXTRA "" CACHE STRING "Extra flags for the PGO USE stage (for experimentation)")
separate_arguments(PIONEER_PGO_EXTRA)

set(PIONEER_RELEASE_FLAGS
    -O3 -march=native -mtune=native -flto -g -fno-exceptions
    -Wall -Wextra -Wcast-qual -mbmi2 -funroll-loops -fno-rtti)
set(PIONEER_RELEASE_LINK -flto -static -pthread -lstdc++)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(${PIONEER_RELEASE_FLAGS})
    if(NOT PIONEER_ASSERTS)
        add_compile_options(-DNDEBUG)
    endif()
    add_link_options(${PIONEER_RELEASE_LINK})

    if(NOT PIONEER_PGO STREQUAL "OFF")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            if(PIONEER_PGO STREQUAL "GENERATE")
                # -fprofile-update=atomic is required here, not optional: the searcher runs on a
                # worker thread alongside the UCI thread, and non-atomic counter updates race and
                # silently corrupt the profile.
                add_compile_options(-fprofile-generate -fprofile-update=atomic)
                add_link_options(-fprofile-generate -fprofile-update=atomic)
            elseif(PIONEER_PGO STREQUAL "USE")
                # -fno-peel-loops / -fno-tracer: GCC's loop peeling and tracing become actively
                # counterproductive under profile feedback on search-style code (deep recursion,
                # unpredictable branches). This is the flag set Stockfish uses for its own GCC
                # PGO builds, on the same kind of workload -- preferred over guessing.
                # -fprofile-correction: tolerate counter inconsistencies from threaded execution.
                # -Wmissing-profile is left on deliberately: a missing profile makes PGO a net loss.
                add_compile_options(-fprofile-use -fprofile-partial-training -fno-peel-loops -fno-tracer -fprofile-correction ${PIONEER_PGO_EXTRA})
                add_link_options(-fprofile-use -fno-peel-loops -fno-tracer -fprofile-correction ${PIONEER_PGO_EXTRA})
            endif()

        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Clang writes .profraw files that must be merged with llvm-profdata before use.
            file(MAKE_DIRECTORY "${PIONEER_PGO_DIR}")
            if(PIONEER_PGO STREQUAL "GENERATE")
                add_compile_options(-fprofile-generate=${PIONEER_PGO_DIR})
                add_link_options(-fprofile-generate=${PIONEER_PGO_DIR})
            elseif(PIONEER_PGO STREQUAL "USE")
                add_compile_options(-fprofile-use=${PIONEER_PGO_DIR}/pioneer.profdata)
                add_link_options(-fprofile-use=${PIONEER_PGO_DIR}/pioneer.profdata)
            endif()

        else()
            message(WARNING "PIONEER_PGO set but compiler '${CMAKE_CXX_COMPILER_ID}' is unsupported; ignoring.")
        endif()

        message(STATUS "PGO stage: ${PIONEER_PGO}")
    endif()
endif()

add_executable(Pioneer ${SOURCES})

file(GLOB NNUE_BIN_FILES "${CMAKE_SOURCE_DIR}/src/nnue/bin/*")

add_custom_command(TARGET Pioneer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Pioneer>/nnue_bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NNUE_BIN_FILES} $<TARGET_FILE_DIR:Pioneer>/nnue_bin
)