# Root CMake for JACOBI (Just Another Collection Of Book Implementations) project.
cmake_minimum_required(VERSION 3.21)

project(jacobi CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ------------------------------------------------------------------------------
# Various helpers:
set(JACOBI_CMAKE_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake-scripts)
include("${JACOBI_CMAKE_SCRIPTS_DIR}/common_options.cmake")

set(COMPILER_ENABLE_UNUSED_X_AS_ERROR OFF)
include("${JACOBI_CMAKE_SCRIPTS_DIR}/compiler_flags.cmake")

include( "${JACOBI_CMAKE_SCRIPTS_DIR}/find_program_required.cmake")
include( "${JACOBI_CMAKE_SCRIPTS_DIR}/static_analysis.cmake" )
include( "${JACOBI_CMAKE_SCRIPTS_DIR}/code_coverage.cmake" )

# ------------------------------------------------------------------------------
# Build options
option(JACOBI_INSTALL           "Generate install target"  ON)
option(JACOBI_BUILD_TESTS       "Build tests"              ON)
option(JACOBI_BUILD_BENCHMARKS  "Build benchmarks"         ON)
option(JACOBI_GCC_CODE_COVERAGE "Build with code coverage" OFF)
# ------------------------------------------------------------------------------

message(STATUS "JACOBI_INSTALL:           ${JACOBI_INSTALL}")
message(STATUS "JACOBI_BUILD_TEST:        ${JACOBI_BUILD_TESTS}")
message(STATUS "JACOBI_GCC_CODE_COVERAGE: ${JACOBI_GCC_CODE_COVERAGE}")

if (JACOBI_GCC_CODE_COVERAGE)
    make_code_coverage_targets(
        NAME CodeCoverage
        HTML_TITLE "Code coverage report for jacobi library"
        FILTERS
            '${CMAKE_SOURCE_DIR}/book/src/.*'
            '${CMAKE_SOURCE_DIR}/book/include/.*'
    )
endif()

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(tsl-robin-map REQUIRED)
find_package(type_safe REQUIRED)
find_package(absl REQUIRED)

# Use adhusted plf_list with trivially-copyable iterators.
# https://github.com/mattreecebentley/plf_list/pull/35
add_subdirectory(custom_deps/plf_list)
# instead of
# find_package(plf_list REQUIRED)

# ====================================================================
# Version
# ====================================================================
include(${JACOBI_CMAKE_SCRIPTS_DIR}/extract_version.cmake)
extract_version(
    "jacobi/include/jacobi/version.hpp"
    # Define extracted version as following:
    JACOBI_VERSION_MAJOR
    JACOBI_VERSION_MINOR
    JACOBI_VERSION_PATCH
)

set(JACOBI_VERSION ${JACOBI_VERSION_MAJOR}.${JACOBI_VERSION_MINOR}.${JACOBI_VERSION_PATCH})
# ABI compatibility version.
# A safer approach is to declare compatibility on the level of minor version.
# If a major version compatibility is chosen then consider changing
# a COMPATIBILITY type in install section (SameMinorVersion => SameMajorVersion).
set(JACOBI_SOVERSION ${JACOBI_VERSION_MAJOR}.${JACOBI_VERSION_MINOR})

message(STATUS "jacobi VERSION  : " ${JACOBI_VERSION})
message(STATUS "jacobi SOVERSION: " ${JACOBI_SOVERSION})

# ===================================
add_subdirectory(jacobi)
add_subdirectory(book)

if (JACOBI_BUILD_TESTS OR JACOBI_BUILD_BENCHMARKS)
    add_subdirectory(snapshots)
endif ()

if (JACOBI_BUILD_TESTS)
    include(CTest)
    enable_testing()
    message(STATUS "Tests are enabled")

    find_package(GTest MODULE REQUIRED)

    add_subdirectory(book/test)
endif ()

if (JACOBI_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
    find_package(CLI11 REQUIRED)
    add_subdirectory(benchmarks)
endif ()


