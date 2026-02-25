#include <jacobi/book/vocabulary_types.hpp>

#include <map>

#include <gtest/gtest.h>

namespace jacobi::book
{

namespace /* anonymous */
{

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderIdCustomHash )
{
    order_id_custom_hash_t h;

    EXPECT_NE( h( order_id_t{ 1ULL } ), 1ULL );
    EXPECT_NE( h( order_id_t{ 1ULL } ), h( order_id_t{ 20260101ULL } ) );
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, TradeSideToIndex )
{
    EXPECT_EQ( 0, trade_side_to_index( trade_side::buy ) );
    EXPECT_EQ( 1, trade_side_to_index( trade_side::sell ) );
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsComparator )
{
    {
        order_price_operations_t< trade_side::buy >::cmp_t cmp;
        EXPECT_TRUE( cmp( order_price_t{ 10 }, order_price_t{ 9 } ) );
        EXPECT_TRUE( cmp( order_price_t{ 100 }, order_price_t{ 50 } ) );

        EXPECT_FALSE( cmp( order_price_t{ 42 }, order_price_t{ 42 } ) );

        EXPECT_FALSE( cmp( order_price_t{ 42 }, order_price_t{ 100 } ) );
        EXPECT_FALSE( cmp( order_price_t{ 11 }, order_price_t{ 12 } ) );
    }

    {
        order_price_operations_t< trade_side::sell >::cmp_t cmp;
        EXPECT_FALSE( cmp( order_price_t{ 10 }, order_price_t{ 9 } ) );
        EXPECT_FALSE( cmp( order_price_t{ 100 }, order_price_t{ 50 } ) );

        EXPECT_FALSE( cmp( order_price_t{ 42 }, order_price_t{ 42 } ) );

        EXPECT_TRUE( cmp( order_price_t{ 42 }, order_price_t{ 100 } ) );
        EXPECT_TRUE( cmp( order_price_t{ 11 }, order_price_t{ 12 } ) );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsLessThan )
{
    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_TRUE( op.lt( order_price_t{ 10 }, order_price_t{ 9 } ) );
        EXPECT_TRUE( op.lt( order_price_t{ 100 }, order_price_t{ 50 } ) );

        EXPECT_FALSE( op.lt( order_price_t{ 42 }, order_price_t{ 42 } ) );

        EXPECT_FALSE( op.lt( order_price_t{ 42 }, order_price_t{ 100 } ) );
        EXPECT_FALSE( op.lt( order_price_t{ 11 }, order_price_t{ 12 } ) );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_FALSE( op.lt( order_price_t{ 10 }, order_price_t{ 9 } ) );
        EXPECT_FALSE( op.lt( order_price_t{ 100 }, order_price_t{ 50 } ) );

        EXPECT_FALSE( op.lt( order_price_t{ 42 }, order_price_t{ 42 } ) );

        EXPECT_TRUE( op.lt( order_price_t{ 42 }, order_price_t{ 100 } ) );
        EXPECT_TRUE( op.lt( order_price_t{ 11 }, order_price_t{ 12 } ) );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsLessOrEqual )
{
    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_TRUE( op.le( order_price_t{ 10 }, order_price_t{ 9 } ) );
        EXPECT_TRUE( op.le( order_price_t{ 100 }, order_price_t{ 50 } ) );

        EXPECT_TRUE( op.le( order_price_t{ 42 }, order_price_t{ 42 } ) );

        EXPECT_FALSE( op.le( order_price_t{ 42 }, order_price_t{ 100 } ) );
        EXPECT_FALSE( op.le( order_price_t{ 11 }, order_price_t{ 12 } ) );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_FALSE( op.le( order_price_t{ 10 }, order_price_t{ 9 } ) );
        EXPECT_FALSE( op.le( order_price_t{ 100 }, order_price_t{ 50 } ) );

        EXPECT_TRUE( op.le( order_price_t{ 42 }, order_price_t{ 42 } ) );

        EXPECT_TRUE( op.le( order_price_t{ 42 }, order_price_t{ 100 } ) );
        EXPECT_TRUE( op.le( order_price_t{ 11 }, order_price_t{ 12 } ) );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsDistance )
{
    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_EQ( op.distance( order_price_t{ 110 }, order_price_t{ 120 } ),
                   order_price_t{ -10 } );

        EXPECT_EQ( op.distance( order_price_t{ 110 }, order_price_t{ 110 } ),
                   order_price_t{ 0 } );

        EXPECT_EQ( op.distance( order_price_t{ 110 }, order_price_t{ 100 } ),
                   order_price_t{ 10 } );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_EQ( op.distance( order_price_t{ 110 }, order_price_t{ 120 } ),
                   order_price_t{ 10 } );

        EXPECT_EQ( op.distance( order_price_t{ 110 }, order_price_t{ 110 } ),
                   order_price_t{ 0 } );

        EXPECT_EQ( op.distance( order_price_t{ 110 }, order_price_t{ 100 } ),
                   order_price_t{ -10 } );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsSafeU64Distance )
{
    constexpr auto i64_max  = std::numeric_limits< std::int64_t >::max();
    constexpr auto i64_min  = std::numeric_limits< std::int64_t >::min();
    constexpr auto ui64_max = std::numeric_limits< std::uint64_t >::max();

    {
        order_price_operations_t< trade_side::buy > op;

        EXPECT_EQ(
            op.safe_u64_distance( order_price_t{ 110 }, order_price_t{ 110 } ),
            0 );

        EXPECT_EQ(
            op.safe_u64_distance( order_price_t{ 110 }, order_price_t{ 100 } ),
            10 );

        EXPECT_EQ( op.safe_u64_distance( order_price_t{ i64_max },
                                         order_price_t{ i64_min } ),
                   ui64_max );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_EQ(
            op.safe_u64_distance( order_price_t{ 110 }, order_price_t{ 110 } ),
            0 );

        EXPECT_EQ(
            op.safe_u64_distance( order_price_t{ 90 }, order_price_t{ 100 } ),
            10 );

        EXPECT_EQ( op.safe_u64_distance( order_price_t{ i64_min },
                                         order_price_t{ i64_max } ),
                   ui64_max );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsAdvanceForward )
{
    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_EQ( op.advance_forward( order_price_t{ 100 }, order_price_t{ 33 } ),
                   order_price_t{ 133 } );

        EXPECT_EQ( op.advance_forward( order_price_t{ 100 }, order_price_t{ -1 } ),
                   order_price_t{ 99 } );

        EXPECT_EQ( op.advance_forward( order_price_t{ 100 } ),
                   order_price_t{ 101 } );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_EQ( op.advance_forward( order_price_t{ 100 }, order_price_t{ 33 } ),
                   order_price_t{ 67 } );

        EXPECT_EQ( op.advance_forward( order_price_t{ 100 }, order_price_t{ -1 } ),
                   order_price_t{ 101 } );

        EXPECT_EQ( op.advance_forward( order_price_t{ 100 } ),
                   order_price_t{ 99 } );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsAdvanceBackward )
{
    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_EQ(
            op.advance_backward( order_price_t{ 100 }, order_price_t{ 33 } ),
            order_price_t{ 67 } );

        EXPECT_EQ(
            op.advance_backward( order_price_t{ 100 }, order_price_t{ -1 } ),
            order_price_t{ 101 } );

        EXPECT_EQ( op.advance_backward( order_price_t{ 100 } ),
                   order_price_t{ 99 } );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_EQ(
            op.advance_backward( order_price_t{ 100 }, order_price_t{ 33 } ),
            order_price_t{ 133 } );

        EXPECT_EQ(
            op.advance_backward( order_price_t{ 100 }, order_price_t{ -1 } ),
            order_price_t{ 99 } );

        EXPECT_EQ( op.advance_backward( order_price_t{ 100 } ),
                   order_price_t{ 101 } );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsMin )
{
    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_EQ( op.min( order_price_t{ 100 }, order_price_t{ 33 } ),
                   order_price_t{ 100 } );

        EXPECT_EQ( op.min( order_price_t{ -100 }, order_price_t{ -1 } ),
                   order_price_t{ -1 } );

        EXPECT_EQ( op.min( order_price_t{ 100 }, order_price_t{ 100 } ),
                   order_price_t{ 100 } );
    }

    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_EQ( op.min( order_price_t{ 100 }, order_price_t{ 33 } ),
                   order_price_t{ 33 } );

        EXPECT_EQ( op.min( order_price_t{ -100 }, order_price_t{ -1 } ),
                   order_price_t{ -100 } );

        EXPECT_EQ( op.min( order_price_t{ 100 }, order_price_t{ 100 } ),
                   order_price_t{ 100 } );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsMax )
{
    {
        order_price_operations_t< trade_side::sell > op;
        EXPECT_EQ( op.max( order_price_t{ 100 }, order_price_t{ 33 } ),
                   order_price_t{ 100 } );

        EXPECT_EQ( op.max( order_price_t{ -100 }, order_price_t{ -1 } ),
                   order_price_t{ -1 } );

        EXPECT_EQ( op.max( order_price_t{ 100 }, order_price_t{ 100 } ),
                   order_price_t{ 100 } );
    }

    {
        order_price_operations_t< trade_side::buy > op;
        EXPECT_EQ( op.max( order_price_t{ 100 }, order_price_t{ 33 } ),
                   order_price_t{ 33 } );

        EXPECT_EQ( op.max( order_price_t{ -100 }, order_price_t{ -1 } ),
                   order_price_t{ -100 } );

        EXPECT_EQ( op.max( order_price_t{ 100 }, order_price_t{ 100 } ),
                   order_price_t{ 100 } );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBookVocabularyTypes, OrderPriceOperationsComparatorWithMap )
{
    using buy_map_t =
        std::map< order_price_t,
                  int,
                  order_price_operations_t< trade_side::buy >::cmp_t >;

    buy_map_t buy_map{};
    buy_map[ order_price_t{ 10 } ] = 1;
    buy_map[ order_price_t{ 9 } ]  = 2;

    EXPECT_EQ( buy_map.begin()->first, order_price_t{ 10 } );
    EXPECT_EQ( buy_map.rbegin()->first, order_price_t{ 9 } );

    using sell_map_t =
        std::map< order_price_t,
                  int,
                  order_price_operations_t< trade_side::sell >::cmp_t >;

    sell_map_t sell_map{};
    sell_map[ order_price_t{ 10 } ] = 1;
    sell_map[ order_price_t{ 9 } ]  = 2;

    EXPECT_EQ( sell_map.begin()->first, order_price_t{ 9 } );
    EXPECT_EQ( sell_map.rbegin()->first, order_price_t{ 10 } );
}

}  // anonymous namespace

}  // namespace jacobi::book
