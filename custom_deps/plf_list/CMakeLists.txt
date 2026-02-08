# plf_list header-only library wrapper
# Acts as a workaround to make iterator types trivial.
# https://github.com/mattreecebentley/plf_list/pull/35

add_library(plf_list_local INTERFACE)

target_include_directories(
    plf_list_local
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plf_list>
    $<INSTALL_INTERFACE:include>
)

add_library(plf_list::plf_list ALIAS plf_list_local)

target_compile_features(plf_list_local INTERFACE cxx_std_11)
