cmake_minimum_required(VERSION 3.21)

# ====================================================================
# Declare project
# ====================================================================

set(TARGET_PROJECT jacobibook)
project(${TARGET_PROJECT} LANGUAGES CXX VERSION ${JACOBI_VERSION})

# ====================================================================
# Set sources
# ====================================================================

list(APPEND TARGET_PUBLIC_HEADERS
    include/jacobi/book/utils/chunk_list.hpp
    include/jacobi/book/utils/lemire_hash.hpp

    include/jacobi/book/book.hpp
    include/jacobi/book/chunked_price_level.hpp
    include/jacobi/book/chunked_soa_price_level.hpp
    include/jacobi/book/order_refs_index.hpp
    include/jacobi/book/orders_table_crtp_base.hpp
    include/jacobi/book/price_level.hpp
    include/jacobi/book/price_level_fwd.hpp
    include/jacobi/book/soa_price_level.hpp
    include/jacobi/book/vocabulary_types.hpp

    include/jacobi/book/map/orders_table.hpp

    include/jacobi/book/linear/orders_table.hpp

    include/jacobi/book/mixed/lru/orders_table.hpp

    include/jacobi/book/mixed/hot_cold/orders_table.hpp
    include/jacobi/book/mixed/hot_cold/book_init_params.hpp
)

# ====================================================================
# TARGET
# ====================================================================
add_library(${TARGET_PROJECT} INTERFACE ${TARGET_PUBLIC_HEADERS} )
add_library(jacobi::book ALIAS ${TARGET_PROJECT})

target_include_directories(${TARGET_PROJECT}
                           INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
)

# Set dependencies here:
target_link_libraries(${TARGET_PROJECT}
                      INTERFACE
                      fmt::fmt
                      type_safe::type_safe
                      plf_list::plf_list
                      tsl::robin_map
                      Boost::headers
                      range-v3::range-v3
                      abseil::abseil

                      jacobi::jacobi
)
# ====================================================================
