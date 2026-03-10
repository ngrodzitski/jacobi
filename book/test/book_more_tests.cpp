/**
 * @file More book building tests
 *
 * Test don't do lots of checks.
 * The purpose is to pass all asserts within implementation (in debug mode).
 */
#include <jacobi/book/book.hpp>

#include <cstdlib>

#if defined( JACOBI_BOOK_TEST_PLVL11 ) || defined( JACOBI_BOOK_TEST_PLVL12 )    \
    || defined( JACOBI_BOOK_TEST_PLVL13 ) || defined( JACOBI_BOOK_TEST_PLVL21 ) \
    || defined( JACOBI_BOOK_TEST_PLVL22 )
#    include <jacobi/book/price_level.hpp>
#elif defined( JACOBI_BOOK_TEST_PLVL30 ) || defined( JACOBI_BOOK_TEST_PLVL31 )
#    include <jacobi/book/soa_price_level.hpp>
#elif defined( JACOBI_BOOK_TEST_PLVL41 ) || defined( JACOBI_BOOK_TEST_PLVL42 )
#    include <jacobi/book/chunked_price_level.hpp>
#elif defined( JACOBI_BOOK_TEST_PLVL51 ) || defined( JACOBI_BOOK_TEST_PLVL52 ) \
    || defined( JACOBI_BOOK_TEST_PLVL53 )
#    include <jacobi/book/chunked_soa_price_level.hpp>
#else
#    error "JACOBI_BOOK_TEST_PLVL?? must be defined"
#endif

#include <jacobi/book/order_refs_index.hpp>

#if defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_MAP )
#    include <jacobi/book/map/orders_table.hpp>
#    include <jacobi/book/map/intrusive/orders_table.hpp>
#elif defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_LINEAR )
#    include <jacobi/book/linear/orders_table.hpp>
#elif defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_MIXED )
#    include <jacobi/book/mixed/lru/orders_table.hpp>
#    include <jacobi/book/mixed/hot_cold/orders_table.hpp>
#    include <jacobi/book/mixed/hot_cold/book_init_params.hpp>
#else
#    error "JACOBI_BOOK_TEST_LEVELS_STORAGE_?? must be defined"
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

#if defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_MAP )
#elif defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_LINEAR )
#elif defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_MIXED )
#else
#    error "JACOBI_BOOK_TEST_LEVELS_STORAGE_?? must be defined"
#endif

#if defined( JACOBI_BOOK_TEST_PLVL11 )
using price_level_factory_type_t =
    std_price_levels_factory_t< std_price_level_t< std_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL12 )
using price_level_factory_type_t =
    std_price_levels_factory_t< std_price_level_t< plf_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL13 )
using price_level_factory_type_t = std_intrusive_list_price_levels_factory_t;
#elif defined( JACOBI_BOOK_TEST_PLVL21 )
using price_level_factory_type_t = shared_list_container_price_levels_factory_t<
    shared_list_container_price_level_t< std_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL22 )
using price_level_factory_type_t = shared_list_container_price_levels_factory_t<
    shared_list_container_price_level_t< plf_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL30 )
using price_level_factory_type_t = soa_price_levels_factory_t<
    soa_price_level_t< std_vector_soa_price_level_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL31 )
using price_level_factory_type_t = soa_price_levels_factory_t<
    soa_price_level_t< boost_smallvec_soa_price_level_traits_t< 16 > > >;
#elif defined( JACOBI_BOOK_TEST_PLVL41 )
using price_level_factory_type_t = chunked_price_levels_factory_t<
    chunked_price_level_t< std_chunk_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL42 )
using price_level_factory_type_t = chunked_price_levels_factory_t<
    chunked_price_level_t< plf_chunk_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL51 )
using price_level_factory_type_t = chunked_soa_price_levels_factory_t<
    chunked_soa_price_level_t< std_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL52 )
using price_level_factory_type_t = chunked_soa_price_levels_factory_t<
    chunked_soa_price_level_t< plf_list_traits_t > >;
#elif defined( JACOBI_BOOK_TEST_PLVL53 )
using price_level_factory_type_t = std_intrusive_chunked_soa_price_level_factory_t;
#else
#    error "JACOBI_BOOK_TEST_PLVL?? must be defined"
#endif

#if defined( JACOBI_BOOK_TEST_REFIX1 )
template < typename Value, typename Order_Reference >
using order_refs_index_type_t =
    order_refs_index_std_unordered_map_t< Value, Order_Reference >;
#elif defined( JACOBI_BOOK_TEST_REFIX2 )
template < typename Value, typename Order_Reference >
using order_refs_index_type_t =
    order_refs_index_tsl_robin_map_t< Value, Order_Reference >;
#elif defined( JACOBI_BOOK_TEST_REFIX3 )
template < typename Value, typename Order_Reference >
using order_refs_index_type_t =
    order_refs_index_boost_unordered_flat_map_t< Value, Order_Reference >;
#elif defined( JACOBI_BOOK_TEST_REFIX4 )
template < typename Value, typename Order_Reference >
using order_refs_index_type_t =
    order_refs_index_absl_flat_hash_map_t< Value, Order_Reference >;
#else
#    error "JACOBI_BOOK_TEST_REFIX? must be defined"
#endif

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
//        IV.  chunked_price_levels_factory_t + chunked_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//        V.   chunked_soa_price_levels_factory_t + chunked_soa_price_level_t
//              A. std_list_traits_t
//              B. plf_list_traits_t
//              C. intrusive list + nodes pool
//
//     2. ORDERS_REF_ONDEX:
//        I.   order_refs_index_std_unordered_map_t
//        II.  order_refs_index_tsl_robin_map_t
//        III. order_refs_index_boost_unordered_flat_map_t
//        IV.  order_refs_index_absl_flat_hash_map_t
//
//     3  ORDERS_TABLE
//        I.    map::std_map_orders_table_t
//        II.   map::absl_map_orders_table_t
//        III.  map::intrusive::std_orders_table_t
//        IV.   linear::v1::orders_table_t
//        V.    linear::v2::orders_table_t
//        VI.   linear::v3::orders_table_t
//        VII.  mixed::lru::orders_table_t
//        VIII. mixed::lru::v2::orders_table_t
//        IX.   mixed::hot_cold::orders_table_t
//
// In total we have 11 * 4 * 9 = 396  combinations.

#if defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_MAP )
    // =========================================================================
    // MAP (std):
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        map::std_map_orders_table_t >,
    // ==================================
    // MAP (absl):
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        map::absl_map_orders_table_t >,
    // ==================================
    // MAP (absl):
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        map::intrusive::std_orders_table_t >
#elif defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_LINEAR )
    // =========================================================================
    // LINEAR V1-3:
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        linear::v1::orders_table_t >,
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        linear::v2::orders_table_t >,
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        linear::v3::orders_table_t >
#elif defined( JACOBI_BOOK_TEST_LEVELS_STORAGE_MIXED )
    // =========================================================================
    // MIXED LRU:
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        mixed::lru::orders_table_t >,
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        mixed::lru::v2::orders_table_t >,
    // =========================================================================
    // MIXED HOT/COLD:
    book_traits_t<
        book_impl_data_t< price_level_factory_type_t, order_refs_index_type_t >,
        mixed::hot_cold::orders_table_t >
#else
#    error "JACOBI_BOOK_TEST_LEVELS_STORAGE_?? must be defined"
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
        handle_single_event( this->book, events[ i ] );
    }

    print_book( "Before last event", this->book );

    handle_single_event( this->book, events.back() );

    print_book( "After last event", this->book );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, AskBBO )
{
    this->book.template add_order< trade_side::sell >(
        order_t{ order_id_t{ 0x238427E074D5989ull },
                 order_qty_t{ 200 },
                 order_price_t{ 314 } } );
    this->book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 0x238427E074D598Eull },
                 order_qty_t{ 200 },
                 order_price_t{ 309 } } );

    const auto bbo = this->book.bbo();

    ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
    ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

    EXPECT_EQ( type_safe::get( bbo.offer.value() ), 314 );
    EXPECT_EQ( type_safe::get( bbo.bid.value() ), 309 );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, AskTopPriceQty )
{
    this->book.template add_order< trade_side::sell >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 200 }, order_price_t{ 314 } } );
    this->book.template add_order< trade_side::sell >(
        order_t{ order_id_t{ 2 }, order_qty_t{ 20 }, order_price_t{ 314 } } );
    this->book.template add_order< trade_side::sell >(
        order_t{ order_id_t{ 3 }, order_qty_t{ 2 }, order_price_t{ 314 } } );

    this->book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 7 }, order_qty_t{ 1 }, order_price_t{ 309 } } );
    this->book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 8 }, order_qty_t{ 100 }, order_price_t{ 309 } } );
    this->book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 9 }, order_qty_t{ 10 }, order_price_t{ 309 } } );

    const auto qs = this->book.sell().top_price_qty();
    const auto qb = this->book.buy().top_price_qty();

    ASSERT_TRUE( static_cast< bool >( qs ) );
    ASSERT_TRUE( static_cast< bool >( qb ) );

    EXPECT_EQ( qs.value(), order_qty_t{ 222 } );
    EXPECT_EQ( qb.value(), order_qty_t{ 111 } );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, AskFirstOrder )
{
    const order_t first_sell_order{ order_id_t{ 1 },
                                    order_qty_t{ 200 },
                                    order_price_t{ 314 } };
    this->book.template add_order< trade_side::sell >( first_sell_order );
    this->book.template add_order< trade_side::sell >(
        order_t{ order_id_t{ 2 }, order_qty_t{ 20 }, order_price_t{ 314 } } );
    this->book.template add_order< trade_side::sell >(
        order_t{ order_id_t{ 3 }, order_qty_t{ 2 }, order_price_t{ 314 } } );

    const order_t first_buy_order{ order_id_t{ 7 },
                                   order_qty_t{ 1 },
                                   order_price_t{ 309 } };
    this->book.template add_order< trade_side::buy >( first_buy_order );
    this->book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 8 }, order_qty_t{ 100 }, order_price_t{ 309 } } );
    this->book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 9 }, order_qty_t{ 10 }, order_price_t{ 309 } } );

    const auto s = this->book.sell().first_order();
    const auto b = this->book.buy().first_order();

    EXPECT_EQ( s.id, first_sell_order.id );
    EXPECT_EQ( s.price, first_sell_order.price );
    EXPECT_EQ( s.qty, first_sell_order.qty );

    EXPECT_EQ( b.id, first_buy_order.id );
    EXPECT_EQ( b.price, first_buy_order.price );
    EXPECT_EQ( b.qty, first_buy_order.qty );
}

}  // anonymous namespace

}  // namespace jacobi::book
