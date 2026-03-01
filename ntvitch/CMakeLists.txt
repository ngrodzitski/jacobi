cmake_minimum_required(VERSION 3.21)

# ====================================================================
# Declare project
# ====================================================================

set(TARGET_PROJECT jacobintvitch)
project(${TARGET_PROJECT} LANGUAGES CXX VERSION ${JACOBI_VERSION})

# ====================================================================
# Set sources
# ====================================================================

list(APPEND TARGET_PUBLIC_HEADERS
    include/jacobi/ntvitch/ntvitch.hpp
    include/jacobi/ntvitch/ntvitch_fmt.hpp
    include/jacobi/ntvitch/packed_struct_begin.ipp
    include/jacobi/ntvitch/packed_struct_end.ipp
)

# ====================================================================
# TARGET
# ====================================================================
add_library(${TARGET_PROJECT} INTERFACE ${TARGET_PUBLIC_HEADERS})
add_library(jacobi::ntvitch ALIAS ${TARGET_PROJECT})

target_include_directories(${TARGET_PROJECT}
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
)

add_library(${TARGET_PROJECT}_fmt INTERFACE ${TARGET_PUBLIC_HEADERS})
add_library(jacobi::ntvitch_fmt ALIAS ${TARGET_PROJECT}_fmt)

target_include_directories(${TARGET_PROJECT}_fmt
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
)
target_link_libraries(${TARGET_PROJECT}_fmt  INTERFACE fmt::fmt )

# ====================================================================
