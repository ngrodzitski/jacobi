#include <jacobi/book/mixed/lru/orders_table.hpp>

#include <numeric>

#include <jacobi/book/price_level.hpp>
#include <jacobi/book/order_refs_index.hpp>

#include <gtest/gtest.h>

namespace jacobi::book::mixed::lru
{

namespace /* anonymous */
{

// NOLINTNEXTLINE
TEST( JacobiBookMixedLruDetailsLruKickList, Simple )
{
    details::lru_kick_list_t lru{ 5 };

    {
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 5ULL, v.size() );
        EXPECT_EQ( 0, lru.lru_index() );
        EXPECT_EQ( v, ( std::vector{ 0, 1, 2, 3, 4 } ) );
    }

    {
        lru.use_index( 0 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 5ULL, v.size() );
        EXPECT_EQ( 1, lru.lru_index() );
        EXPECT_EQ( v, ( std::vector{ 1, 2, 3, 4, 0 } ) );
    }

    {
        lru.use_index( 1 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 5ULL, v.size() );
        EXPECT_EQ( v, ( std::vector{ 2, 3, 4, 0, 1 } ) );
    }

    {
        lru.use_index( 2 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 5ULL, v.size() );
        EXPECT_EQ( 3, lru.lru_index() );
        EXPECT_EQ( v, ( std::vector{ 3, 4, 0, 1, 2 } ) );
    }

    {
        lru.use_index( 3 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 5ULL, v.size() );
        EXPECT_EQ( 4, lru.lru_index() );
        EXPECT_EQ( v, ( std::vector{ 4, 0, 1, 2, 3 } ) );
    }

    {
        lru.use_index( 4 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 5ULL, v.size() );
        EXPECT_EQ( 0, lru.lru_index() );
        EXPECT_EQ( v, ( std::vector{ 0, 1, 2, 3, 4 } ) );
    }

    {
        lru.free_index( 4 );
        EXPECT_EQ( 4, lru.lru_index() );
        const auto v = lru.make_lru_dump();
        EXPECT_EQ( v, ( std::vector{ 4, 0, 1, 2, 3 } ) );
    }

    {
        lru.free_index( 2 );
        EXPECT_EQ( 2, lru.lru_index() );
        const auto v = lru.make_lru_dump();
        EXPECT_EQ( v, ( std::vector{ 2, 4, 0, 1, 3 } ) );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookMixedLruDetailsLruKickList, MaxSize254 )
{
    details::lru_kick_list_t lru{ 254 };

    {
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 254ULL, v.size() );
        EXPECT_EQ( 0, lru.lru_index() );

        std::vector< int > expected_vec( 254 );
        std::iota( begin( expected_vec ), end( expected_vec ), 0 );
        EXPECT_EQ( v, expected_vec );
    }

    {
        lru.use_index( 0 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 254ULL, v.size() );
        EXPECT_EQ( 1, lru.lru_index() );

        std::vector< int > expected_vec( 254 );
        std::iota( begin( expected_vec ), end( expected_vec ), 1 );
        expected_vec.back() = 0;
        EXPECT_EQ( v, expected_vec );
    }

    {
        lru.free_index( 0 );
        const auto v = lru.make_lru_dump();
        ASSERT_EQ( 254ULL, v.size() );
        EXPECT_EQ( 0, lru.lru_index() );

        std::vector< int > expected_vec( 254 );
        std::iota( begin( expected_vec ), end( expected_vec ), 0 );
        EXPECT_EQ( v, expected_vec );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookMixedLruDetailsLruKickList, MultipleUseSameIndex )  // NOLINT
{
    {
        details::lru_kick_list_t lru{ 5 };
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 2, 3, 4 } ) );
        lru.use_index( 0 );
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 1, 2, 3, 4, 0 } ) );
        lru.use_index( 0 );
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 1, 2, 3, 4, 0 } ) );
    }

    {
        details::lru_kick_list_t lru{ 5 };

        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 2, 3, 4 } ) );
        lru.use_index( 2 );
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 3, 4, 2 } ) );
        lru.use_index( 2 );
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 3, 4, 2 } ) );
    }

    {
        details::lru_kick_list_t lru{ 5 };

        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 2, 3, 4 } ) );
        lru.use_index( 4 );
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 2, 3, 4 } ) );
        lru.use_index( 4 );
        ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 2, 3, 4 } ) );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookMixedLruDetailsLruKickList, Shuffle )  // NOLINT
{
    details::lru_kick_list_t lru{ 5 };

    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 0, 1, 2, 3, 4 } ) );

    lru.use_index( 0 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 1, 2, 3, 4, 0 } ) );

    lru.use_index( 3 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 1, 2, 4, 0, 3 } ) );

    lru.use_index( 1 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 2, 4, 0, 3, 1 } ) );

    lru.use_index( 3 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 2, 4, 0, 1, 3 } ) );

    lru.use_index( 1 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 2, 4, 0, 3, 1 } ) );

    lru.use_index( 4 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 2, 0, 3, 1, 4 } ) );

    lru.use_index( 0 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 2, 3, 1, 4, 0 } ) );

    lru.use_index( 1 );
    ASSERT_EQ( lru.make_lru_dump(), ( std::vector{ 2, 3, 4, 0, 1 } ) );
}

//
// book_impl_data_t
//

struct book_impl_data_t
{
    using price_level_t          = std_price_level_t< std_list_traits_t >;
    using price_levels_factory_t = std_price_levels_factory_t< price_level_t >;

    struct test_order_ref_value_t
    {
        explicit test_order_ref_value_t( price_level_t::reference_t r )
            : ref{ r }
        {
        }

        price_level_t::reference_t ref;
        [[nodiscard]] order_t access_order() const noexcept
        {
            return ref.make_order();
        }
        [[nodiscard]] price_level_t::reference_t *
        access_order_reference() noexcept
        {
            return &ref;
        }

        // Not used in these tests,
        // but we should comply with Order_Refs_Index_Value_Concept:
        void set_trade_side( trade_side ) noexcept
        {
            assert( false && "Should not be invoked by tests in this file" );
        };
        [[nodiscard]] trade_side get_trade_side() const noexcept
        {
            assert( false && "Should not be invoked by tests in this file" );
            return trade_side::sell;
        }
    };

    using order_refs_index_t =
        order_refs_index_std_unordered_map_t< test_order_ref_value_t,
                                              price_level_t::reference_t >;

    order_refs_index_t order_refs_index{};
    price_levels_factory_t price_levels_factory{};
};

using test_orders_table_sell_t =
    orders_table_t< book_impl_data_t, trade_side::sell >;

#define JACOBI_BOOK_TYPE_ORDERS_TABLE JacobiBookMixedLruOrdersTableSell
#define JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE \
    JacobiBookMixedLruOrdersTableSellFixture
#define JACOBI_BOOK_ORDERS_TABLE_TYPE test_orders_table_sell_t
#include "../../map/orders_table_reusable_set_of_tests.ipp"
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
#undef JACOBI_BOOK_ORDERS_TABLE_TYPE

}  // anonymous namespace

}  // namespace jacobi::book::mixed::lru
