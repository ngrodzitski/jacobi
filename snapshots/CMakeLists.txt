cmake_minimum_required(VERSION 3.21)

# ====================================================================
# Declare project
# ====================================================================

set(TARGET_PROJECT jacobisnapshots)
project(${TARGET_PROJECT} LANGUAGES CXX VERSION ${JACOBI_VERSION})

# ====================================================================
# Set sources
# ====================================================================

list(APPEND TARGET_PUBLIC_HEADERS
    include/jacobi/snapshots/events_snapshots.hpp
    include/jacobi/snapshots/packed_struct_begin.ipp
    include/jacobi/snapshots/packed_struct_end.ipp
)

list(APPEND TARGET_SRC
    src/jacobi/snapshots/events_snapshots.cpp
)

# ====================================================================
# TARGET
# ====================================================================
add_library(${TARGET_PROJECT} STATIC ${TARGET_PUBLIC_HEADERS} ${TARGET_SRC})
add_library(jacobi::snapshots ALIAS ${TARGET_PROJECT})

target_include_directories(${TARGET_PROJECT}
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    ${TARGET_PROJECT}
    jacobi::book
)
# ====================================================================
