#pragma once

#include <sstream>
#include <algorithm>
#include <utility>

#include <jacobi/snapshots/events_snapshots.hpp>

#include <jacobi/book/book.hpp>
#include <jacobi/book/price_level.hpp>
#include <jacobi/book/soa_price_level.hpp>
#include <jacobi/book/chunked_price_level.hpp>
#include <jacobi/book/chunked_soa_price_level.hpp>

namespace jacobi::bench
{

//
// handle_single_event()
//

/**
    switch ( static_cast<book_operation>( ev.op_code ) )

 * @brief Run a benchmark for single book.
 */
template < typename Book_Type >
void handle_single_event( Book_Type & book,
                          const snapshots::update_record_image_t & ev )
{
    using snapshots::book_operation;
    switch( static_cast< book_operation >( ev.op_code ) )
    {
        case book_operation::add_order:
            book.add_order( ev.u.add_order.make_order(), ev.trade_side() );
            break;
        case book_operation::exec_order:
            book.execute_order( ev.order_id(), ev.u.exec_order.exec_qty() );
            break;
        case book_operation::reduce_order:
            book.reduce_order( ev.order_id(), ev.u.reduce_order.canceled_qty() );
            break;
        case book_operation::modify_order:
            book.modify_order( ev.u.modify_order.make_order() );
            break;
        case book_operation::delete_order:
            book.delete_order( ev.order_id() );
            break;
    }
}

//
// get_events_file_name()
//

inline std::string get_events_file_name()
{
    const char * jacobi_benchmark_events_file =
        std::getenv( "JACOBI_BENCHMARK_EVENTS_FILE" );

    if( !jacobi_benchmark_events_file
        || 0 == std::strlen( jacobi_benchmark_events_file ) )
    {
        throw std::runtime_error{ "Failed: JACOBI_BENCHMARK_EVENTS_FILE "
                                  "environment variable must be set" };
    }

    return jacobi_benchmark_events_file;
}

//
// get_events_range()
//

inline std::pair< std::size_t, std::size_t > get_events_range(
    std::optional< std::pair< std::size_t, std::size_t > > default_range =
        std::nullopt )
{
    const char * jacobi_benchmark_events_range =
        std::getenv( "JACOBI_BENCHMARK_EVENTS_RANGE" );
    if( !jacobi_benchmark_events_range
        || 0 == std::strlen( jacobi_benchmark_events_range ) )
    {
        if( !default_range )
        {
            throw std::runtime_error{ "Failed: JACOBI_BENCHMARK_EVENTS_RANGE "
                                      "environment variable must be set" };
        }
        else
        {
            return *default_range;
        }
    }

    std::size_t a, b;
    {
        std::stringstream ss( jacobi_benchmark_events_range );
        char delimiter;

        // Reads int, then char (comma), then int
        if( !( ss >> a >> delimiter >> b
               && ( ',' == delimiter || ' ' == delimiter ) ) )
        {
            throw std::runtime_error{ "Failed: JACOBI_BENCHMARK_EVENTS_RANGE "
                                      "environment variable must be set "
                                      "and have a format: 'N,M'" };
        }
    }

    return std::make_pair( a, b );
}

//
// get_hot_storage_size()
//

inline std::size_t get_hot_storage_size()
{
    std::size_t hot_storage_size = 32;

    const char * jacobi_benchmark_hot_storage_size =
        std::getenv( "JACOBI_BENCHMARK_HOT_STORAGE_SIZE" );

    if( jacobi_benchmark_hot_storage_size
        && 0 != std::strlen( jacobi_benchmark_hot_storage_size ) )
    {
        std::stringstream ss( jacobi_benchmark_hot_storage_size );
        ss >> hot_storage_size;
        if( !ss
            || hot_storage_size
                   != std::clamp< std::size_t >( hot_storage_size, 8, 4096 ) )
        {
            throw std::runtime_error{ "Failed: JACOBI_BENCHMARK_HOT_STORAGE_SIZE "
                                      "environment variable must be 8..1024 "
                                      "or undefined" };
        }
    }

    return hot_storage_size;
}

//
// get_profiling_mode()
//

inline int get_profiling_mode()
{
    int profiling_mode = 0;
    const char * jacobi_benchmark_profile_mode =
        std::getenv( "JACOBI_BENCHMARK_PROFILE_MODE" );

    if( jacobi_benchmark_profile_mode
        && 1 == std::strlen( jacobi_benchmark_profile_mode ) )
    {
        profiling_mode = jacobi_benchmark_profile_mode[ 0 ] - '0';

        if( 1 > profiling_mode || 5 < profiling_mode )
        {
            profiling_mode = 0;
        }
    }

    return profiling_mode;
}

// clang-format off

// ===================================================================
// Anatomy of book traits in benchmark
//
// **TRAITS_TYPE**<
//     **IMPL_DATA_TYPE**<
//         PLEVEL_FACTORY< PLEVEL< LIST_CONTAINER > >,
//         ORDERS_REF_ONDEX >,
//     BSN_COUNTER >
//
//     Here we essentially can choose variables in the above template
//     instantiation
//
//     1. BSN_COUNTER:
//        I.  std_bsn_counter_t
//        II. void_bsn_counter_t
//
//     2. PLEVEL_FACTORY + PLEVEL:
//        I.   std_price_levels_factory_t + std_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//        II.  shared_list_container_price_levels_factory_t
//               + shared_list_container_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//        III. std_price_levels_factory_t
//              A. soa_price_level_t< std_vector_soa_price_level_traits_t >
//              B. boost_smallvec_soa_price_level_traits_t< 16 >
//        IV.  chunked_price_levels_factory_t + chunked_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//        V.   chunked_soa_price_levels_factory_t + chunked_soa_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//
//     3. ORDERS_REF_ONDEX:
//        I.   order_refs_index_std_unordered_map_t
//        II.  order_refs_index_tsl_robin_map_t
//        III. order_refs_index_boost_unordered_flat_map_t
//        IV.  order_refs_index_absl_flat_hash_map_t
//
// In total we have 2*(5*2)*4 = 80 combinations.
//
// Note: BSN_COUNTER has no meaningfull effect
//      which means that the numer of benchmarks can be halfed.


#define JACOBI_GENERATE_BENCHMARKS_XXX_TYPES( traits_type, impl_data_type ) \
struct bsn1_plvl11_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl11_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl11_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl11_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl12_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl12_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl12_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl12_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl21_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl21_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl21_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl21_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl22_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl22_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl22_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl22_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl30_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl30_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl30_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl30_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl31_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl31_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl31_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl31_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl41_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl41_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl41_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl41_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl42_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl42_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl42_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl42_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl51_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl51_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl51_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl51_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn1_plvl52_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl52_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl52_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn1_plvl52_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl11_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl11_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl11_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl11_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::std_list_traits_t > >,                                               \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl12_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl12_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl12_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl12_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::std_price_levels_factory_t< book::std_price_level_t< book::plf_list_traits_t > >,                                               \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl21_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl21_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl21_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl21_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::std_list_traits_t > >,           \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl22_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl22_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl22_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl22_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::shared_list_container_price_levels_factory_t< book::shared_list_container_price_level_t< book::plf_list_traits_t > >,           \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl30_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl30_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl30_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl30_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::std_vector_soa_price_level_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
/* =================================================================== */                                                                            \
struct bsn2_plvl31_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl31_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl31_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl31_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::soa_price_levels_factory_t< book::soa_price_level_t< book::boost_smallvec_soa_price_level_traits_t< 16 > > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                \
{};                                                                                                                                               \
/* =================================================================== */                                                                            \
struct bsn2_plvl41_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl41_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl41_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl41_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::std_chunk_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl42_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl42_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl42_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl42_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_price_levels_factory_t< book::chunked_price_level_t< book::plf_chunk_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::void_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
/* =================================================================== */                                                                            \
struct bsn2_plvl51_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl51_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl51_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl51_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::std_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
/* =================================================================== */                                                                            \
struct bsn2_plvl52_refIX1_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_std_unordered_map_t >,                                                                                         \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl52_refIX2_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_tsl_robin_map_t >,                                                                                             \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl52_refIX3_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_boost_unordered_flat_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};                                                                                                                                               \
                                                                                                                                                  \
struct bsn2_plvl52_refIX4_t                                                                                                                       \
    : traits_type<                                                                                                              \
        impl_data_type<                                                                                                   \
            book::chunked_soa_price_levels_factory_t< book::chunked_soa_price_level_t< book::plf_list_traits_t > >,                                   \
            book::order_refs_index_absl_flat_hash_map_t >,                                                                                  \
        book::std_bsn_counter_t >                                                                                                                 \
{};

// clang-format on

}  // namespace jacobi::bench
