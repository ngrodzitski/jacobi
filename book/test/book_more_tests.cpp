/**
 * @file More book building tests
 *
 * Test don't do lots of checks.
 * The purpose is to pass all asserts within implementation (in debug mode).
 */
#include <jacobi/book/book.hpp>

#include <cstdlib>

#include <jacobi/book/price_level.hpp>
#include <jacobi/book/soa_price_level.hpp>
#include <jacobi/book/chunked_price_level.hpp>
#include <jacobi/book/chunked_soa_price_level.hpp>
#include <jacobi/book/order_refs_index.hpp>

#if defined( JACOBI_BOOK_TESTS_SPLIT_MAP_ONES_1 ) \
    || defined( JACOBI_BOOK_TESTS_SPLIT_MAP_ONES_2 )
#    include <jacobi/book/map/orders_table.hpp>
#elif defined( JACOBI_BOOK_TESTS_SPLIT_LINEAR_V1_ONES )  \
    || defined( JACOBI_BOOK_TESTS_SPLIT_LINEAR_V2_ONES ) \
    || defined( JACOBI_BOOK_TESTS_SPLIT_LINEAR_V3_ONES )
#    include <jacobi/book/linear/orders_table.hpp>
#elif defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_LRU_ONES )
#    include <jacobi/book/mixed/lru/orders_table.hpp>
#elif defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES )
#    include <jacobi/book/mixed/hot_cold/orders_table.hpp>
#    include <jacobi/book/mixed/hot_cold/book_init_params.hpp>
#endif
#include <jacobi/snapshots/events_snapshots.hpp>

#include <gtest/gtest.h>

namespace jacobi::book
{

namespace /* anonymous */
{

//
// book_impl_data_t
//

template < typename Price_Level_Factory,
           template < typename V, typename R >
           class Order_Refs_Index >
struct book_impl_data_t
{
    using price_levels_factory_t = Price_Level_Factory;
    using price_level_t          = typename Price_Level_Factory::price_level_t;

    struct test_order_ref_value_t
    {
        test_order_ref_value_t() = default;
        explicit test_order_ref_value_t( price_level_t::reference_t r )
            : ref{ r }
        {
        }

        price_level_t::reference_t ref;
        trade_side ts{};

        [[nodiscard]] order_t access_order() const noexcept
        {
            return ref.make_order();
        }

        [[nodiscard]] price_level_t::reference_t *
        access_order_reference() noexcept
        {
            return &ref;
        }

        void set_trade_side( trade_side t ) noexcept { ts = t; };
        [[nodiscard]] trade_side get_trade_side() const noexcept { return ts; }
    };

    static_assert( std::is_trivially_copyable_v< test_order_ref_value_t > );

    using order_refs_index_t =
        Order_Refs_Index< test_order_ref_value_t,
                          typename price_level_t::reference_t >;

    order_refs_index_t order_refs_index{};
    price_levels_factory_t price_levels_factory{};
};

//
// book_traits_t
//

template < typename Impl_Data,
           template < typename I, trade_side Trade_Side_Indicator >
           class Orders_Table >
struct book_traits_t
{
    using bsn_counter_t       = std_bsn_counter_t;
    using impl_data_t         = Impl_Data;
    using sell_orders_table_t = Orders_Table< impl_data_t, trade_side::sell >;
    using buy_orders_table_t  = Orders_Table< impl_data_t, trade_side::buy >;
};

//
// JacobiBookBuildingTests
//

template < typename T >
class JacobiBookBuildingTests : public ::testing::Test
{
protected:
    void SetUp() override {}

    using book_type_t = std_book_t< T >;
    book_type_t book;
};

using JacobiBookBuildingTestsTypes = ::testing::Types<
//
// book_traits_t<
//     book_impl_data_t<
//         PLEVEL_FACTORY< PLEVEL< LIST_CONTAINER > >,
//         ORDERS_REF_ONDEX > >
//
//     We essentially can choose 3 variables in the above template
//     instantiation
//
//     1. PLEVEL_FACTORY + PLEVEL:
//        I.   std_price_levels_factory_t + std_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//        II.  shared_list_container_price_levels_factory_t
//               + shared_list_container_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//        III. soa_price_levels_factory_t +
//              A. soa_price_level_t< std_vector_soa_price_level_traits_t >
//              B. boost_smallvec_soa_price_level_traits_t< 16 >
//             Goes only with
//                ORDERS_TABLE = order_refs_index_boost_unordered_flat_map_t
//        IV.  chunked_price_levels_factory_t + chunked_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//             Goes only with
//                ORDERS_TABLE = order_refs_index_boost_unordered_flat_map_t
//        V.   chunked_soa_price_levels_factory_t + chunked_soa_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//             Goes only with
//                ORDERS_TABLE = order_refs_index_boost_unordered_flat_map_t
//
//     2. ORDERS_REF_ONDEX:
//        I.   order_refs_index_std_unordered_map_t
//        II.  order_refs_index_tsl_robin_map_t
//        III. order_refs_index_boost_unordered_flat_map_t
//        IV.  order_refs_index_absl_flat_hash_map_t
//
//     3  ORDERS_TABLE
//        I.   map::std_map_container_traits_t
//        II.  map::absl_map_container_traits_t
//        III. linear::v1::orders_table_t
//        IV.  linear::v2::orders_table_t
//        V.   linear::v3::orders_table_t
//        VI.  mixed::lru::orders_table_t
//        VII. mixed::hot_cold::orders_table_t
//
// In total we have (2 + 2)*4*7 + 1*1*7 + 1*2*7 + 1*2*7 = 147  combinations.

#if defined( JACOBI_BOOK_TESTS_SPLIT_MAP_ONES_1 )
    // =========================================================================
    // MAP (std):
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::std_map_orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::std_map_orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::std_map_orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::std_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::std_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::std_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::std_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::std_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::std_map_orders_table_t >

#elif defined( JACOBI_BOOK_TESTS_SPLIT_MAP_ONES_2 )
    // ==================================
    // MAP (absl):
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::absl_map_orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::absl_map_orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::absl_map_orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   map::absl_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::absl_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::absl_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::absl_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::absl_map_orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        map::absl_map_orders_table_t >

#elif defined( JACOBI_BOOK_TESTS_SPLIT_LINEAR_V1_ONES )
    // =========================================================================
    // LINEAR V1:
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v1::orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v1::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v1::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v1::orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v1::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v1::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v1::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v1::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v1::orders_table_t >

#elif defined( JACOBI_BOOK_TESTS_SPLIT_LINEAR_V2_ONES )
    // =========================================================================
    // LINEAR V2:
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v2::orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v2::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v2::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v2::orders_table_t >

#elif defined( JACOBI_BOOK_TESTS_SPLIT_LINEAR_V3_ONES )
    // =========================================================================
    // LINEAR V3:
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v3::orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v3::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v3::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   linear::v3::orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v3::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v3::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v3::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v3::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        linear::v3::orders_table_t >

#elif defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_LRU_ONES )
    // =========================================================================
    // MIXED LRU:
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::orders_table_t >

    // =========================================================================
    // MIXED LRU V2:
    ,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::v2::orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::v2::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::v2::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::lru::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::lru::v2::orders_table_t >

#elif defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES )
    // =========================================================================
    // MIXED HOT/COLD:
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::hot_cold::orders_table_t >,

    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< std_price_levels_factory_t<
                                         std_price_level_t< plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::hot_cold::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             std_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::hot_cold::orders_table_t >,

    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_std_unordered_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_tsl_robin_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< shared_list_container_price_levels_factory_t<
                                         shared_list_container_price_level_t<
                                             plf_list_traits_t > >,
                                     order_refs_index_absl_flat_hash_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t< book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                                         std_vector_soa_price_level_traits_t > >,
                                     order_refs_index_boost_unordered_flat_map_t >,
                   mixed::hot_cold::orders_table_t >,
    book_traits_t<
        book_impl_data_t< soa_price_levels_factory_t< soa_price_level_t<
                              boost_smallvec_soa_price_level_traits_t< 16 > > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::hot_cold::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< std_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::hot_cold::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_price_levels_factory_t<
                              chunked_price_level_t< plf_chunk_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::hot_cold::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< std_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::hot_cold::orders_table_t >,
    book_traits_t<
        book_impl_data_t< chunked_soa_price_levels_factory_t<
                              chunked_soa_price_level_t< plf_list_traits_t > >,
                          order_refs_index_boost_unordered_flat_map_t >,
        mixed::hot_cold::orders_table_t >
#endif
    >;

TYPED_TEST_SUITE( JacobiBookBuildingTests, JacobiBookBuildingTestsTypes );

template < typename Book >
void print_book( std::string_view desc, const Book & book )
{
    fmt::print( "\n{}: bsn={}\n"
                "======== ========   ======== ========\n"
                "{:32}\n"
                "======== ========   ======== ========\n",
                desc,
                book.bsn(),
                book );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, IssueN0001 )
{
    // clang-format off
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D5989ull }, order_qty_t{ 200 }, order_price_t{ 314 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Aull }, order_qty_t{ 200 }, order_price_t{ 313 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Bull }, order_qty_t{ 200 }, order_price_t{ 312 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Cull }, order_qty_t{ 200 }, order_price_t{ 311 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Dull }, order_qty_t{ 200 }, order_price_t{ 310 } });

    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D598Eull }, order_qty_t{ 200 }, order_price_t{ 309 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D598Full }, order_qty_t{ 200 }, order_price_t{ 308 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5990ull }, order_qty_t{ 200 }, order_price_t{ 307 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5991ull }, order_qty_t{ 200 }, order_price_t{ 306 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5992ull }, order_qty_t{ 200 }, order_price_t{ 305 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5993ull }, order_qty_t{ 200 }, order_price_t{ 304 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5994ull }, order_qty_t{ 200 }, order_price_t{ 303 } });

    {
        auto rng = this->book.buy().levels_range();
        const auto n = std::distance( rng.begin(), rng.end() );

        ASSERT_EQ( n, 7 );

        auto it = rng.begin();
        EXPECT_EQ( it->price(), order_price_t{ 309 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 308 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 307 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 306 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 305 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 304 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 303 } );

    }

    {
        auto rng = this->book.sell().levels_range();
        const auto n = std::distance( rng.begin(), rng.end() );

        ASSERT_EQ( n, 5 );

        auto it = rng.begin();
        EXPECT_EQ( it->price(), order_price_t{ 310 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 311 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 312 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 313 } );

        ++it;
        EXPECT_EQ( it->price(),order_price_t{ 314 } );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, IssueN0002 )
{
    order_id_t id{ 01ull };
    for( auto p = 332; p > 268; --p )
    {
        this->book.add_order( order_t{id++, order_qty_t{ 200 }, order_price_t{ p } }, trade_side::buy);
    }
    this->book.add_order( order_t{id, order_qty_t{ 200 }, order_price_t{ 268 } }, trade_side::buy);
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, SmallBook )
{
    // clang-format off
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D5989ull }, order_qty_t{ 200 }, order_price_t{ 314 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Aull }, order_qty_t{ 200 }, order_price_t{ 313 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Bull }, order_qty_t{ 200 }, order_price_t{ 312 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Cull }, order_qty_t{ 200 }, order_price_t{ 311 } });
    this->book.template add_order< trade_side::sell >( order_t{order_id_t{ 0x238427E074D598Dull }, order_qty_t{ 200 }, order_price_t{ 310 } });

    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D598Eull }, order_qty_t{ 200 }, order_price_t{ 309 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D598Full }, order_qty_t{ 200 }, order_price_t{ 308 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5990ull }, order_qty_t{ 200 }, order_price_t{ 307 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5991ull }, order_qty_t{ 200 }, order_price_t{ 306 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5992ull }, order_qty_t{ 200 }, order_price_t{ 305 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5993ull }, order_qty_t{ 200 }, order_price_t{ 304 } });
    this->book.template add_order< trade_side::buy >( order_t{order_id_t{ 0x238427E074D5994ull }, order_qty_t{ 200 }, order_price_t{ 303 } });

    print_book( "Small Book", this->book );

    this->book.modify_order( order_t{order_id_t{ 0x238427E074D5989ull }, order_qty_t{ 300 }, order_price_t{ 414 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D598Aull }, order_qty_t{ 300 }, order_price_t{ 413 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D598Bull }, order_qty_t{ 300 }, order_price_t{ 412 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D598Cull }, order_qty_t{ 300 }, order_price_t{ 411 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D598Dull }, order_qty_t{ 300 }, order_price_t{ 410 } });

    this->book.modify_order( order_t{order_id_t{ 0x238427E074D598Eull }, order_qty_t{ 100 }, order_price_t{ 209 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D598Full }, order_qty_t{ 100 }, order_price_t{ 208 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D5990ull }, order_qty_t{ 100 }, order_price_t{ 207 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D5991ull }, order_qty_t{ 100 }, order_price_t{ 206 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D5992ull }, order_qty_t{ 100 }, order_price_t{ 205 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D5993ull }, order_qty_t{ 100 }, order_price_t{ 204 } });
    this->book.modify_order( order_t{order_id_t{ 0x238427E074D5994ull }, order_qty_t{ 100 }, order_price_t{ 203 } });

    print_book( "Small Book", this->book );
    this->book.reduce_order( order_id_t{ 0x238427E074D5989ull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D598Aull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D598Bull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D598Cull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D598Dull }, order_qty_t{ 10 } );

    this->book.reduce_order( order_id_t{ 0x238427E074D598Eull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D598Full }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D5990ull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D5991ull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D5992ull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D5993ull }, order_qty_t{ 10 } );
    this->book.reduce_order( order_id_t{ 0x238427E074D5994ull }, order_qty_t{ 10 } );
    // clang-format on

    print_book( "Small Book", this->book );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, BiggerBook )
{
#include "common_big_book.ipp"

    print_book( "Bigger Book", this->book );
}

//
// handle_single_event()
//

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

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, RealisticBook )
{
    const auto events = snapshots::read_events_from_file(
        JACOBI_TEST_DATA_DIR "/sample_single_book.jacobi_data" );

    for( const auto ev : events )
    {
        handle_single_event( this->book, ev );
    }

    print_book( "Final Book", this->book );
}

#if defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES )

#    include "book_more_tests_mixed_hot_cold.ipp"

#endif  // defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES )

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, IssueN0003 )
{
    const auto events =
        snapshots::read_events_from_file( JACOBI_TEST_DATA_DIR "/issueN0003.bin" );

    ASSERT_FALSE( events.empty() );

    for( auto i = 0ULL; i < events.size() - 1; ++i )
    {
        // fmt::print( "i={}\n", i);
        handle_single_event( this->book, events[ i ] );
    }

    print_book( "Before last event", this->book );

    handle_single_event( this->book, events.back() );

    print_book( "After last event", this->book );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, IssueN0004 )
{
    const auto events =
        snapshots::read_events_from_file( JACOBI_TEST_DATA_DIR "/issueN0004.bin" );

    ASSERT_FALSE( events.empty() );

    for( auto i = 0ULL; i < events.size() - 1; ++i )
    {
        // fmt::print( "i={}\n", i);
        handle_single_event( this->book, events[ i ] );
    }

    print_book( "Before last event", this->book );

    handle_single_event( this->book, events.back() );

    print_book( "After last event", this->book );
}

}  // anonymous namespace

}  // namespace jacobi::book
