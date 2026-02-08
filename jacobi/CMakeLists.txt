cmake_minimum_required(VERSION 3.21)

# Define where cmake-scripts dir is located.
# in case of using this lib as a submodule it might be located somewhere else
# and not next to a root `CMakeLists.txt` file of the project.
if (NOT JACOBI_CMAKE_SCRIPTS_DIR)
    # If not explicitly set use a default location (in the root dir):
    set(JACOBI_CMAKE_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/cmake-scripts")
endif ()

# ====================================================================
# Declare project
# ====================================================================

set(TARGET_PROJECT jacobi)
project(${TARGET_PROJECT} LANGUAGES CXX VERSION ${JACOBI_VERSION})

# ====================================================================
# Set sources
# ====================================================================

list(APPEND TARGET_PUBLIC_HEADERS
    include/jacobi/version.hpp
)

# ====================================================================
# TARGET
# ====================================================================
add_library(${TARGET_PROJECT} INTERFACE ${TARGET_PUBLIC_HEADERS})
add_library(jacobi::jacobi ALIAS ${TARGET_PROJECT})

target_include_directories(${TARGET_PROJECT}
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${TARGET_PROJECT}
                           INTERFACE
                           LIB_JACOBI
)
# ====================================================================
