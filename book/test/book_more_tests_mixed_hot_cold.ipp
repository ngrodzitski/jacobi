#if !defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES )
#    error \
        "This file is intended for tests specific to mixed_cold_hot implementation"
#endif  // !defined( JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES )

using jacobi::book::mixed::hot_cold::hot_range_selection;

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingTests, MixedHotColdInitialHotLevels )
{
    auto & book = this->book;
    print_book( "Book", book );

    ASSERT_EQ( 1, book.buy().hot_levels_count() );
    ASSERT_EQ( 1, book.sell().hot_levels_count() );

    ASSERT_EQ( 0, book.buy().cold_levels_count() );
    ASSERT_EQ( 0, book.sell().cold_levels_count() );

    EXPECT_LT( type_safe::get( book.buy().top_level().price() ),
               type_safe::get( book.sell().top_level().price() ) );

    // ==============================================================
    // We expect the following picture:
    //               [4]     HIGHEST   ...
    //               [3]     HIGHEST   ...
    //               [2]     HIGHEST   ...
    //               [1]  B  HIGHEST   ...
    //               [0]  A  HIGHEST   ...
    //                        ***
    //                  ... -LOWEST  A  [0]
    //                  ... -LOWEST  B  [1]
    //                  ... -LOWEST     [2]
    //                  ... -LOWEST     [3]
    //                  ... -LOWEST     [3]

    {
        auto rng = book.buy()
                       .template hot_levels_range<
                           hot_range_selection::storage_head_and_further >();
        auto level_diff_not_one_it = std::ranges::adjacent_find(
            rng,
            []( const auto & a, const auto & b )
            {
                return order_price_t{ 1 } != a.price() - b.price();
                // See the picture above
                // It shows why we do `a -b` for Buy side.
            } );

        auto n = std::distance( begin( rng ), level_diff_not_one_it );

        EXPECT_EQ( std::size( rng ), n );
    }

    {
        auto rng = book.sell()
                       .template hot_levels_range<
                           hot_range_selection::storage_head_and_further >();
        auto level_diff_not_one_it = std::ranges::adjacent_find(
            rng,
            []( const auto & a, const auto & b )
            {
                return order_price_t{ 1 } != b.price() - a.price();
                // See the picture above
                // It shows why we do `a -b` for Buy side.
            } );

        auto n = std::distance( begin( rng ), level_diff_not_one_it );

        EXPECT_EQ( std::size( rng ), n );
    }
}

constexpr std::size_t hot_storage_size = 8;

//
// JacobiBookBuildingMixedHotColdTests
//

template < typename T >
class JacobiBookBuildingMixedHotColdTests : public testing::Test
{
protected:
    //
    // custom_params_t
    //
    using book_init_params_t = book::mixed::hot_cold::std_book_init_params_t< T >;

    using book_type_t = std_book_t< T >;
    book_type_t book{ book_init_params_t{ hot_storage_size } };

    JacobiBookBuildingMixedHotColdTests() {}

    template < typename Range >
    auto range_size( Range r )
    {
        return std::distance( r.begin(), r.end() );
    }

    template < typename Table >
    auto get_hot_levels_range( Table & table )
    {
        return table.template hot_levels_range<
            hot_range_selection::best_price_and_further >();
    }

    template < typename Table >
    auto get_hot_levels_range_all( Table & table )
    {
        return table.template hot_levels_range<
            hot_range_selection::storage_head_and_further >();
    }

    template < typename Table >
    auto get_cold_levels_range( Table & table )
    {
        return table.cold_levels_range();
    }

    void set_sample_data()
    {
        std::uint64_t buy_orders_id{ 0x100 };
        order_price_t buy_price{ 1000 };

        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 11 }, buy_price } );

        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 1 }, buy_price } );

        ASSERT_EQ( book.buy().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.buy() ) ),
                   book.buy().hot_levels_count() );

        buy_price--;
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 222 }, buy_price } );
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 22 }, buy_price } );
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 2 }, buy_price } );

        ASSERT_EQ( book.buy().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.buy() ) ),
                   book.buy().hot_levels_count() );

        buy_price--;
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 3333 }, buy_price } );
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 333 }, buy_price } );
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 33 }, buy_price } );
        book.template add_order< trade_side::buy >( order_t{
            order_id_t{ buy_orders_id++ }, order_qty_t{ 3 }, buy_price } );

        ASSERT_EQ( book.buy().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.buy() ) ),
                   book.buy().hot_levels_count() );

        while( book.buy().hot_levels_count() != 1 + hot_storage_size / 2 )
        {
            buy_price--;
            book.template add_order< trade_side::buy >( order_t{
                order_id_t{ buy_orders_id++ }, order_qty_t{ 5 }, buy_price } );
        }

        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.buy() ) ),
                   1 + hot_storage_size / 2 );

        while( book.buy().cold_levels_count() < 4 )
        {
            buy_price--;
            book.template add_order< trade_side::buy >( order_t{
                order_id_t{ buy_orders_id++ }, order_qty_t{ 5 }, buy_price } );
        }

        ASSERT_EQ( book.buy().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.buy() ) ),
                   book.buy().hot_levels_count() );

        std::uint64_t sell_orders_id{ 0x200 };
        order_price_t sell_price{ 1100 };

        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 11 }, sell_price } );
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 1 }, sell_price } );

        ASSERT_EQ( book.sell().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.sell() ) ),
                   book.sell().hot_levels_count() );

        sell_price += order_price_t{ 1 };

        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 222 }, sell_price } );
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 22 }, sell_price } );
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 2 }, sell_price } );

        ASSERT_EQ( book.sell().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.sell() ) ),
                   book.sell().hot_levels_count() );

        sell_price += order_price_t{ 1 };
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 3333 }, sell_price } );
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 333 }, sell_price } );
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 33 }, sell_price } );
        book.template add_order< trade_side::sell >( order_t{
            order_id_t{ sell_orders_id++ }, order_qty_t{ 3 }, sell_price } );

        ASSERT_EQ( book.sell().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.sell() ) ),
                   book.sell().hot_levels_count() );

        while( book.sell().hot_levels_count() != 1 + hot_storage_size / 2 )
        {
            sell_price++;
            book.template add_order< trade_side::sell >( order_t{
                order_id_t{ sell_orders_id++ }, order_qty_t{ 5 }, sell_price } );
        }

        ASSERT_EQ( book.sell().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.sell() ) ),
                   book.sell().hot_levels_count() );

        while( book.sell().cold_levels_count() < 4 )
        {
            sell_price++;
            book.template add_order< trade_side::sell >( order_t{
                order_id_t{ sell_orders_id++ }, order_qty_t{ 5 }, sell_price } );
        }

        ASSERT_EQ( book.sell().hot_levels_count(), 1 + hot_storage_size / 2 );
        ASSERT_EQ( this->range_size( this->get_hot_levels_range( book.sell() ) ),
                   book.sell().hot_levels_count() );

        print_book( "INITIAL BOOK", this->book );
    }
};

TYPED_TEST_SUITE( JacobiBookBuildingMixedHotColdTests,
                  JacobiBookBuildingTestsTypes );

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideLongJump )
{
    auto & book = this->book;
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 111 }, order_qty_t{ 100 }, order_price_t{ 1200 } } );

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 112 }, order_qty_t{ 10 }, order_price_t{ 1200 } } );

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 113 }, order_qty_t{ 1 }, order_price_t{ 1200 } } );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1200 );

    print_book( "BOOK", book );
    {
        auto rng = book.buy()
                       .template hot_levels_range<
                           hot_range_selection::storage_head_and_further >();

        std::vector< int > expected_hot_levels_prices{ 1200 + 3, 1200 + 2,
                                                       1200 + 1, 1200,
                                                       1200 - 1, 1200 - 2,
                                                       1200 - 3, 1200 - 4 };

        ranges::for_each( rng,
                          [ &, i = 0ULL ]( const auto & lvl ) mutable
                          {
                              EXPECT_EQ( type_safe::get( lvl.price() ),
                                         expected_hot_levels_prices[ i ] )
                                  << i;
                              ++i;
                          } );

        EXPECT_EQ( book.buy().top_level().orders_count(), 3 );
        EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 111 );
    }

    // The following manipulation will not slide hot levels.
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 120 }, order_qty_t{ 2 }, order_price_t{ 1201 } } );

    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1201 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 1 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 2 );

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 121 }, order_qty_t{ 20 }, order_price_t{ 1201 } } );
    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1201 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 2 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 22 );

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 130 }, order_qty_t{ 42 }, order_price_t{ 1202 } } );
    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1202 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 1 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 42 );

    // Not even a new top price:
    book.template add_order< trade_side::buy >( order_t{
        order_id_t{ 140 }, order_qty_t{ 10'000 }, order_price_t{ 1199 } } );
    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1202 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 1 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 42 );

    // The max price we can operate with without altering hot levels.
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 150 }, order_qty_t{ 42 }, order_price_t{ 1203 } } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 151 }, order_qty_t{ 38 }, order_price_t{ 1203 } } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 152 }, order_qty_t{ 20 }, order_price_t{ 1203 } } );
    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1203 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 3 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 100 );

    book.template add_order< trade_side::buy >( order_t{
        order_id_t{ 1250 }, order_qty_t{ 1250 }, order_price_t{ 1250 } } );

    print_book( "Book", book );

    {
        auto rng = book.buy()
                       .template hot_levels_range<
                           hot_range_selection::storage_head_and_further >();

        std::vector< int > expected_hot_levels_prices{ 1250 + 3, 1250 + 2,
                                                       1250 + 1, 1250,
                                                       1250 - 1, 1250 - 2,
                                                       1250 - 3, 1250 - 4 };

        ranges::for_each( rng,
                          [ &, i = 0ULL ]( const auto & lvl ) mutable
                          {
                              EXPECT_EQ( type_safe::get( lvl.price() ),
                                         expected_hot_levels_prices[ i ] )
                                  << i;
                              ++i;
                          } );

        EXPECT_EQ( book.buy().top_level().orders_count(), 1 );
        EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 1250 );

        auto cold_rng =
            this->get_cold_levels_range( book.buy() )
            | ranges::views::transform(
                []( const auto & lvl ) { return type_safe::get( lvl.price() ); } );

        std::vector< order_price_t::value_type > cold_values{ cold_rng.begin(),
                                                              cold_rng.end() };

        ASSERT_EQ( cold_values.size(), 5 );
        EXPECT_EQ( cold_values[ 0 ], 1203 );
        EXPECT_EQ( cold_values[ 1 ], 1202 );
        EXPECT_EQ( cold_values[ 2 ], 1201 );
        EXPECT_EQ( cold_values[ 3 ], 1200 );
        EXPECT_EQ( cold_values[ 4 ], 1199 );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideShort )
{
    auto & book = this->book;
    this->set_sample_data();

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 9 }, order_price_t{ 1004 } } );
    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1004 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 1 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 9 );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 2 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ), 1004 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 9 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1000 );
        EXPECT_EQ( it->orders_count(), 2 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 11 + 1 );
    }

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 2 }, order_qty_t{ 8 }, order_price_t{ 1006 } } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 333 }, order_qty_t{ 12 }, order_price_t{ 1006 } } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 3 }, order_qty_t{ 7 }, order_price_t{ 1005 } } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 4 }, order_qty_t{ 3 }, order_price_t{ 1007 } } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 5 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ), 1007 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 3 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1006 );
        EXPECT_EQ( it->orders_count(), 2 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 20 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1005 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 7 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1004 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 9 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1000 );
        EXPECT_EQ( it->orders_count(), 2 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 11 + 1 );
    }

    // Jump 2 positions above current max.
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 5 }, order_qty_t{ 8 }, order_price_t{ 1009 } } );

    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 4 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ), 1009 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 8 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1007 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 3 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1006 );
        EXPECT_EQ( it->orders_count(), 2 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 20 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1005 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 7 );
    }

    // No movement in hot levels
    // 1006 has 2 orders and we delete 1 of them.
    book.delete_order( order_id_t{ 333 } );

    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 4 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ), 1009 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 8 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1007 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 3 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1006 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 8 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1005 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 7 );
    }

    // Remove current top.
    book.delete_order( order_id_t{ 5 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 3 );

        auto it = non_empty_hot_levels.begin();
        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1006 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 8 );

        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1005 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 7 );
    }

    // Remove current top $1007
    // this means a top level will be in a last quarter of the
    // hot lovels storage and that would cause
    // hot-storage to slide down.
    book.delete_order( order_id_t{ 4 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_GE( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ), 1006 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 8 );

        ASSERT_GE( this->range_size( non_empty_hot_levels ), 2 );
        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1005 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 7 );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 3 );
        it++;
        EXPECT_EQ( type_safe::get( it->price() ), 1004 );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 9 );
    }

    book.delete_order( order_id_t{ 1 } );
    print_book( "Book", book );
    book.delete_order( order_id_t{ 2 } );
    print_book( "Book", book );
    book.delete_order( order_id_t{ 3 } );
    print_book( "Book", book );

    EXPECT_EQ( type_safe::get( book.buy().top_price().value() ), 1000 );
    EXPECT_EQ( book.buy().top_level().orders_count(), 2 );
    EXPECT_EQ( type_safe::get( book.buy().top_level().orders_qty() ), 12 );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges1 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    this->set_sample_data();

    auto target_order_price = price_ops_t::min_value - order_price_t{ 3 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );

    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        EXPECT_EQ(
            type_safe::get(
                this->get_hot_levels_range_all( book.buy() ).begin()->price() ),
            type_safe::get( price_ops_t::min_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges2 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    this->set_sample_data();

    auto target_order_price = price_ops_t::min_value - order_price_t{ 2 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        EXPECT_EQ(
            type_safe::get(
                this->get_hot_levels_range_all( book.buy() ).begin()->price() ),
            type_safe::get( price_ops_t::min_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges3 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    this->set_sample_data();

    auto target_order_price = price_ops_t::min_value - order_price_t{ 1 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        EXPECT_EQ(
            type_safe::get(
                this->get_hot_levels_range_all( book.buy() ).begin()->price() ),
            type_safe::get( price_ops_t::min_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges4 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    this->set_sample_data();

    auto target_order_price = price_ops_t::min_value;

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        EXPECT_EQ(
            type_safe::get(
                this->get_hot_levels_range_all( book.buy() ).begin()->price() ),
            type_safe::get( price_ops_t::min_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges5 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    this->set_sample_data();

    auto target_order_price = price_ops_t::min_value - order_price_t{ 6 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        EXPECT_NE(
            type_safe::get(
                this->get_hot_levels_range_all( book.buy() ).begin()->price() ),
            type_safe::get( price_ops_t::min_value ) );
    }

    auto target_order_price_2 = target_order_price + order_price_t{ 4 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 2 }, order_qty_t{ 1 }, target_order_price_2 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 2 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price_2 ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );

        ++it;
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( target_order_price ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }

    {
        EXPECT_EQ(
            type_safe::get(
                this->get_hot_levels_range_all( book.buy() ).begin()->price() ),
            type_safe::get( price_ops_t::min_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges6 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    // Start with empty book;

    auto target_order_price = order_price_t{ 0 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    book.delete_order( order_id_t{ 1 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 0 );
    }
    {
        auto r =
            this->get_hot_levels_range_all( book.buy() ) | ranges::views::reverse;
        EXPECT_EQ( type_safe::get( r.begin()->price() ),
                   type_safe::get( price_ops_t::max_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges7 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    // Start with empty book;

    auto target_order_price = order_price_t{ 0 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 2 }, order_qty_t{ 1 }, price_ops_t::max_value } );

    print_book( "Book", book );

    book.delete_order( order_id_t{ 1 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( price_ops_t::max_value ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        auto r =
            this->get_hot_levels_range_all( book.buy() ) | ranges::views::reverse;
        EXPECT_EQ( type_safe::get( r.begin()->price() ),
                   type_safe::get( price_ops_t::max_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges8 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    // Start with empty book;

    auto target_order_price = order_price_t{ 0 };

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 2 },
                 order_qty_t{ 1 },
                 price_ops_t::max_value + order_price_t{ 3 } } );

    print_book( "Book", book );

    book.delete_order( order_id_t{ 1 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( price_ops_t::max_value + order_price_t{ 3 } ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        auto r =
            this->get_hot_levels_range_all( book.buy() ) | ranges::views::reverse;
        EXPECT_EQ( type_safe::get( r.begin()->price() ),
                   type_safe::get( price_ops_t::max_value ) );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookBuildingMixedHotColdTests, SlideNearEdges9 )
{
    auto & book       = this->book;
    using price_ops_t = order_price_operations_t< trade_side::buy >;

    // Start with empty book;

    auto target_order_price = price_ops_t::min_value;

    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 1 }, order_qty_t{ 1 }, target_order_price } );
    book.template add_order< trade_side::buy >(
        order_t{ order_id_t{ 2 },
                 order_qty_t{ 1 },
                 price_ops_t::min_value - order_price_t{ 1 } } );

    print_book( "Book", book );

    book.delete_order( order_id_t{ 1 } );
    print_book( "Book", book );

    {
        auto non_empty_hot_levels =
            this->get_hot_levels_range( book.buy() )
            | ranges::views::filter( []( const auto & lvl )
                                     { return !lvl.empty(); } );

        ASSERT_EQ( this->range_size( non_empty_hot_levels ), 1 );

        auto it = non_empty_hot_levels.begin();
        EXPECT_EQ( type_safe::get( it->price() ),
                   type_safe::get( price_ops_t::min_value - order_price_t{ 1 } ) );
        EXPECT_EQ( it->orders_count(), 1 );
        EXPECT_EQ( type_safe::get( it->orders_qty() ), 1 );
    }
    {
        auto r = this->get_hot_levels_range_all( book.buy() );
        EXPECT_EQ( type_safe::get( r.begin()->price() ),
                   type_safe::get( price_ops_t::min_value ) );
    }
}
