#include <jacobi/book/book.hpp>

#include <algorithm>
#include <cstdlib>
#include <random>

#include <range/v3/view/zip.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/join.hpp>

#include <jacobi/book/price_level.hpp>
#include <jacobi/book/order_refs_index.hpp>
#include <jacobi/book/map/orders_table.hpp>

#include <gtest/gtest.h>

namespace jacobi::book
{

namespace /* anonymous */
{

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

    using order_refs_index_t =
        order_refs_index_std_unordered_map_t< test_order_ref_value_t,
                                              price_level_t::reference_t >;

    order_refs_index_t order_refs_index{};
    price_levels_factory_t price_levels_factory{};
};

//
// minimum_book_traits_t
//

struct minimum_book_traits_t
{
    using bsn_counter_t = std_bsn_counter_t;
    using impl_data_t   = book_impl_data_t;
    using sell_orders_table_t =
        map::std_map_orders_table_t< book_impl_data_t, trade_side::sell >;
    using buy_orders_table_t =
        map::std_map_orders_table_t< book_impl_data_t, trade_side::buy >;
};

template < typename Book >
void print_book( std::string_view desc, const Book & book )
{
    fmt::print( "\n{}: bsn={}\n"
                "======== ========   ======== ========\n"
                "{}\n"
                "======== ========   ======== ========\n",
                desc,
                book.bsn(),
                book );
}

// NOLINTNEXTLINE
TEST( JacobiBook, SampleStdBookIsEmpty )
{
    std_book_t< minimum_book_traits_t > book;

    print_book( "Empty book", book );

    ASSERT_TRUE( book.sell().empty() );
    ASSERT_TRUE( book.buy().empty() );

    {
        const auto bbo = book.bbo();
        ASSERT_FALSE( static_cast< bool >( bbo.bid ) );
        ASSERT_FALSE( static_cast< bool >( bbo.offer ) );
    }

    {
        const auto sells = book.sell().levels_range();
        const auto buys  = book.buy().levels_range();

        ASSERT_EQ( 0, std::distance( sells.begin(), sells.end() ) );
        ASSERT_EQ( 0, std::distance( buys.begin(), buys.end() ) );
    }
}

// NOLINTNEXTLINE
TEST( JacobiBook, BSNCounter )
{
    std_book_t< minimum_book_traits_t > book;

    ASSERT_EQ( book.bsn(), bsn_t{ 0ULL } );

    order_t order{ order_id_t{ 10 }, order_qty_t{ 100 }, order_price_t{ 333 } };

    book.add_order( order, trade_side::buy );
    ASSERT_EQ( book.bsn(), bsn_t{ 1ULL } );

    book.execute_order( order.id, order_qty_t{ 10 } );
    ASSERT_EQ( book.bsn(), bsn_t{ 2ULL } );

    book.reduce_order( order.id, order_qty_t{ 80 } );
    ASSERT_EQ( book.bsn(), bsn_t{ 3ULL } );

    book.modify_order( order );
    ASSERT_EQ( book.bsn(), bsn_t{ 4ULL } );

    book.delete_order( order.id );
    ASSERT_EQ( book.bsn(), bsn_t{ 5ULL } );

    book.add_order( order, trade_side::buy );
    ASSERT_EQ( book.bsn(), bsn_t{ 6ULL } );

    order.id++;
    book.add_order( order, trade_side::buy );
    ASSERT_EQ( book.bsn(), bsn_t{ 7ULL } );

    order.id++;
    book.add_order( order, trade_side::buy );
    ASSERT_EQ( book.bsn(), bsn_t{ 8ULL } );

    order.id++;
    book.add_order( order, trade_side::buy );
    ASSERT_EQ( book.bsn(), bsn_t{ 9ULL } );
}

// NOLINTNEXTLINE
TEST( JacobiBook, TopPriceAndQty )
{
    std_book_t< minimum_book_traits_t > book;

    auto make_order = [ id_counter = order_id_t{ 1 } ]( auto price,
                                                        auto qty ) mutable -> auto
    {
        return order_t{ .id    = id_counter++,
                        .qty   = order_qty_t{ qty },
                        .price = order_price_t{ price } };
    };

    book.add_order( make_order( 1000LL, 111U ), trade_side::sell );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_FALSE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 1000LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 111UL );
    }

    book.add_order( make_order( 99LL, 222U ), trade_side::buy );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 99LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 222U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 1000LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 111U );
    }

    book.add_order( make_order( 99LL, 100U ), trade_side::buy );
    book.add_order( make_order( 1000LL, 100U ), trade_side::sell );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 99LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 322U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 1000LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 211U );
    }

    book.add_order( make_order( 199LL, 111U ), trade_side::buy );
    book.add_order( make_order( 999LL, 222U ), trade_side::sell );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 199LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 111U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 999LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 222U );
    }

    const auto ob = make_order( 299LL, 333U );
    const auto os = make_order( 899LL, 123U );

    book.add_order( ob, trade_side::buy );
    book.add_order( os, trade_side::sell );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 299LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 333U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 899LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 123U );
    }

    book.execute_order( ob.id, order_qty_t{ 100 } );
    book.execute_order( os.id, order_qty_t{ 1 } );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 299LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 233U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 899LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 122U );
    }

    book.reduce_order( ob.id, order_qty_t{ 1 } );
    book.reduce_order( os.id, order_qty_t{ 100 } );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 299LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 232U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 899LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 22U );
    }

    book.modify_order( ob );
    book.modify_order( os );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 299LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 333U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 899LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 123U );
    }

    book.delete_order( ob.id );
    book.delete_order( os.id );
    print_book( "Book", book );
    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 199LL );
        ASSERT_EQ( type_safe::get( book.buy().top_price_qty().value() ), 111U );

        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 999LL );
        ASSERT_EQ( type_safe::get( book.sell().top_price_qty().value() ), 222U );
    }
}

//
// JacobiBookBuildingFixture
//

class JacobiBookBuildingFixture : public ::testing::TestWithParam< unsigned >
{
protected:
    order_t make_order( order_price_t price )
    {
        order_qty_t qty{ 1U + static_cast< unsigned >( std::rand() ) % 50U };
        return order_t{ .id    = order_id_t{ 1 },  // will be assigned later
                        .qty   = qty,
                        .price = order_price_t{ price } };
    }

    std::vector< order_t > make_line_of_orders( order_price_t p, std::size_t n )
    {
        std::vector< order_t > res;

        std::generate_n(
            std::back_inserter( res ), n, [ & ] { return make_order( p ); } );

        return res;
    }

    struct order_ref_t
    {
        unsigned ts;
        unsigned i;
        unsigned j;
    };

    std::array< std::vector< std::vector< order_t > >, 2 > orders;
    std::array< std::vector< std::vector< order_t > >, 2 > etalon_orders;
    std::vector< order_ref_t > order_refs;

    void SetUp() override
    {
        std::srand( GetParam() );

        // clang-format off

        // Sells
        orders[0] = std::vector< std::vector< order_t > > {
            make_line_of_orders( order_price_t{ 150 }, 1 ),
            make_line_of_orders( order_price_t{ 109 }, 1 ),
            make_line_of_orders( order_price_t{ 108 }, 2 ),
            make_line_of_orders( order_price_t{ 107 }, 3 ),
            make_line_of_orders( order_price_t{ 106 }, 6 ),
            make_line_of_orders( order_price_t{ 105 }, 4 ),
            make_line_of_orders( order_price_t{ 104 }, 9 ),
            make_line_of_orders( order_price_t{ 103 }, 8 ),
            make_line_of_orders( order_price_t{ 102 }, 9 ),
            make_line_of_orders( order_price_t{ 101 }, 11 ),
            make_line_of_orders( order_price_t{ 100 }, 12 ),
        };
        // Buys
        orders[1] = std::vector< std::vector< order_t > > {
            make_line_of_orders( order_price_t{ 99 }, 11 ),
            make_line_of_orders( order_price_t{ 98 }, 8 ),
            make_line_of_orders( order_price_t{ 97 }, 8 ),
            make_line_of_orders( order_price_t{ 96 }, 9 ),
            make_line_of_orders( order_price_t{ 95 }, 7 ),
            make_line_of_orders( order_price_t{ 94 }, 6 ),
            make_line_of_orders( order_price_t{ 93 }, 5 ),
            make_line_of_orders( order_price_t{ 92 }, 4 ),
            make_line_of_orders( order_price_t{ 91 }, 3 ),
            make_line_of_orders( order_price_t{ 90 }, 2 ),
            make_line_of_orders( order_price_t{ 50 }, 2 ),
        };
        // clang-format on

        for( unsigned ts = 0U; ts < orders.size(); ++ts )
        {
            for( unsigned i = 0U; i < orders[ ts ].size(); ++i )
            {
                for( unsigned j = 0U; j < orders[ ts ][ i ].size(); ++j )
                {
                    order_refs.push_back(
                        order_ref_t{ .ts = ts, .i = i, .j = j } );
                }
            }
        }

        std::random_device rd;
        std::mt19937 g( rd() );
        std::shuffle( begin( order_refs ), end( order_refs ), g );

        // Reassign ids so that they match with how the order would be added.
        order_id_t id_counter{ 1 };
        for( const auto ref : this->order_refs )
        {
            orders[ ref.ts ][ ref.i ][ ref.j ].id = id_counter++;
        }

        // Now we need to sort orders within a level according to the id.
        // that will follow the order in which the level would be built.
        for( unsigned ts = 0U; ts < orders.size(); ++ts )
        {
            for( unsigned i = 0U; i < orders[ ts ].size(); ++i )
            {
                auto line = orders[ ts ][ i ];
                std::sort(
                    begin( line ),
                    end( line ),
                    []( auto a, auto b )
                    { return type_safe::get( a.id ) < type_safe::get( b.id ); } );
                etalon_orders[ ts ].push_back( line );
            }
        }
    }

    std::array< trade_side, 2 > tsides{ trade_side::sell, trade_side::buy };

    // Create a book with default fill with orders.
    auto build_default_book()
    {
        std_book_t< minimum_book_traits_t > book;

        for( const auto ref : this->order_refs )
        {
            book.add_order( this->orders[ ref.ts ][ ref.i ][ ref.j ],
                            tsides[ ref.ts ] );
        }
        print_book( "Full book (default)", book );

        return book;
    }

    void compare_to_etalon( const auto & etalon_lines,
                            auto book_lines_rng,
                            std::string_view desc )
    {
        ASSERT_EQ( etalon_lines.size(), ranges::distance( book_lines_rng ) )
            << desc;

        for( const auto [ i, book_level ] :
             ranges::views::zip( ranges::views::iota( 0 ), book_lines_rng ) )
        {
            auto etalon_line = etalon_lines[ i ];

            ASSERT_EQ( etalon_line.size(), book_level.orders_count() )
                << "i=" << i << "; " << desc;

            for( const auto [ j, book_order ] : ranges::views::zip(
                     ranges::views::iota( 0 ), book_level.orders_range() ) )
            {
                const auto order = etalon_line[ j ];

                ASSERT_EQ( order.id, book_order.id )
                    << "i=" << i << ", j=" << j << "; " << desc << "; "
                    << fmt::format( "order={}; book_order={}", order, book_order );
                ASSERT_EQ( order.qty, book_order.qty )
                    << "i=" << i << ", j=" << j << "; " << desc << "; "
                    << fmt::format( "order={}; book_order={}", order, book_order );
                ASSERT_EQ( order.price, book_order.price )
                    << "i=" << i << ", j=" << j << "; " << desc << "; "
                    << fmt::format( "order={}; book_order={}", order, book_order );
            }
        }
    };
};

INSTANTIATE_TEST_SUITE_P( SRand,
                          JacobiBookBuildingFixture,
                          ::testing::Values( 2026U,
                                             20260101U,
                                             0xAABBCCDDU,
                                             0x20171215U,
                                             0x87654321U,
                                             std::time( nullptr ) ) );

TEST_P( JacobiBookBuildingFixture, BuildWithAdds )
{
    auto book = build_default_book();

    ASSERT_EQ( book.bsn(), bsn_t{ this->order_refs.size() } );

    const auto bbo = book.bbo();
    ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
    ASSERT_TRUE( static_cast< bool >( bbo.offer ) );

    ASSERT_EQ( type_safe::get( bbo.bid.value() ), 99LL );
    ASSERT_EQ( type_safe::get( bbo.offer.value() ), 100LL );

    this->compare_to_etalon( this->etalon_orders[ 0 ],
                             book.sell().levels_range() | ranges::views::reverse,
                             "SELL" );
    this->compare_to_etalon(
        this->etalon_orders[ 1 ], book.buy().levels_range(), "BUY" );
}

TEST_P( JacobiBookBuildingFixture, BuildWithAddDeleteAdd )
{
    auto book = build_default_book();

    for( const auto ref : this->order_refs )
    {
        book.delete_order( this->orders[ ref.ts ][ ref.i ][ ref.j ].id );
    }

    {
        const auto bbo = book.bbo();
        ASSERT_FALSE( static_cast< bool >( bbo.bid ) );
        ASSERT_FALSE( static_cast< bool >( bbo.offer ) );
    }
    ASSERT_EQ( book.bsn(), bsn_t{ 2 * this->order_refs.size() } );

    for( const auto ref : this->order_refs )
    {
        book.add_order( this->orders[ ref.ts ][ ref.i ][ ref.j ],
                        tsides[ ref.ts ] );
    }

    ASSERT_EQ( book.bsn(), bsn_t{ 3 * this->order_refs.size() } );

    {
        const auto bbo = book.bbo();
        ASSERT_TRUE( static_cast< bool >( bbo.bid ) );
        ASSERT_TRUE( static_cast< bool >( bbo.offer ) );
        ASSERT_EQ( type_safe::get( bbo.bid.value() ), 99LL );
        ASSERT_EQ( type_safe::get( bbo.offer.value() ), 100LL );
    }

    this->compare_to_etalon( this->etalon_orders[ 0 ],
                             book.sell().levels_range() | ranges::views::reverse,
                             "SELL" );
    this->compare_to_etalon(
        this->etalon_orders[ 1 ], book.buy().levels_range(), "BUY" );
}

TEST_P( JacobiBookBuildingFixture, ExecuteByFullQty )
{
    auto book = build_default_book();

    while( !book.empty() )
    {
        if( auto & sells = book.sell(); !sells.empty() )
        {
            const auto order = sells.first_order();

            book.execute_order( order.id, order.qty );
        }

        if( auto & buys = book.buy(); !buys.empty() )
        {
            const auto order = buys.first_order();

            book.execute_order( order.id, order.qty );
        }
        ASSERT_LE( book.bsn(), bsn_t{ 2 * this->order_refs.size() } );
    }

    ASSERT_EQ( book.bsn(), bsn_t{ 2 * this->order_refs.size() } );
}

TEST_P( JacobiBookBuildingFixture, ReduceAll )
{
    auto book = build_default_book();

    auto local_etalon_orders = this->etalon_orders;
    auto all_etalon_orders =
        local_etalon_orders | ranges::views::join | ranges::views::join;
    for( order_t & order : all_etalon_orders )
    {
        order_qty_t canceled_qty{ std::rand() % type_safe::get( order.qty ) };
        order.qty -= canceled_qty;

        if( canceled_qty >= order_qty_t{ 0 } )
        {
            book.reduce_order( order.id, canceled_qty );
        }
    }

    print_book( "After reduce", book );

    this->compare_to_etalon( local_etalon_orders[ 0 ],
                             book.sell().levels_range() | ranges::views::reverse,
                             "SELL" );
    this->compare_to_etalon(
        local_etalon_orders[ 1 ], book.buy().levels_range(), "BUY" );
}

TEST_P( JacobiBookBuildingFixture, IdempotentModifies )
{
    auto book = build_default_book();

    auto all_etalon_orders =
        this->etalon_orders | ranges::views::join | ranges::views::join;

    // Modifying orders in an order that is in etalon
    // will effectevely do a full circle rotation and put all orders
    // in the same places.
    for( const order_t order : all_etalon_orders )
    {
        book.modify_order( order );
    }

    print_book( "After modifies", book );

    this->compare_to_etalon( this->etalon_orders[ 0 ],
                             book.sell().levels_range() | ranges::views::reverse,
                             "SELL" );
    this->compare_to_etalon(
        this->etalon_orders[ 1 ], book.buy().levels_range(), "BUY" );
}

TEST_P( JacobiBookBuildingFixture, ModifiesMoveAllLevelsMoveAwayFromBBO )
{
    auto test_body = [ & ]( order_price_t levels_diff )
    {
        std_book_t< minimum_book_traits_t > book;

        std::array< trade_side, 2 > tsides{ trade_side::sell, trade_side::buy };
        for( const auto ref : this->order_refs )
        {
            book.add_order( this->orders[ ref.ts ][ ref.i ][ ref.j ],
                            tsides[ ref.ts ] );
        }
        print_book( "Full book", book );

        auto local_etalon_orders = this->etalon_orders;
        auto sell_etalon_orders  = local_etalon_orders[ 0 ] | ranges::views::join;
        auto buy_etalon_orders   = local_etalon_orders[ 1 ] | ranges::views::join;

        for( order_t & order : sell_etalon_orders )
        {
            order.price += levels_diff;
            book.modify_order( order );
        }
        for( order_t & order : buy_etalon_orders )
        {
            order.price -= levels_diff;
            book.modify_order( order );
        }

        {
            const auto bbo = book.bbo();
            ASSERT_EQ( type_safe::get( bbo.bid.value() + levels_diff ), 99LL );
            ASSERT_EQ( type_safe::get( bbo.offer.value() - levels_diff ), 100LL );
        }

        print_book( "After modifies", book );

        this->compare_to_etalon(
            local_etalon_orders[ 0 ],
            book.sell().levels_range() | ranges::views::reverse,
            fmt::format( "SELL +/- {}", levels_diff ) );
        this->compare_to_etalon( local_etalon_orders[ 1 ],
                                 book.buy().levels_range(),
                                 fmt::format( "BUY +/- {}", levels_diff ) );
    };

    test_body( order_price_t{ 1 } );
    test_body( order_price_t{ 2 } );
    test_body( order_price_t{ 3 } );
    test_body( order_price_t{ 9 } );
    test_body( order_price_t{ 10 } );
    test_body( order_price_t{ 11 } );
    test_body( order_price_t{ 50 } );
    test_body( order_price_t{ 100 } );
    test_body( order_price_t{ 1000 } );
}

TEST_P( JacobiBookBuildingFixture, ModifySingle )
{
    auto book = build_default_book();

    const auto levels = book.sell().levels_range();

    const auto & lvl0 = *( levels.begin() );
    const auto & lvl3 = *std::next( levels.begin(), 3 );

    ASSERT_NE( lvl0.price(), lvl3.price() );

    const auto lvl0_orders_count_before = lvl0.orders_count();
    const auto lvl3_orders_count_before = lvl3.orders_count();
    const auto lvl0_orders_qty_before   = lvl0.orders_qty();
    const auto lvl3_orders_qty_before   = lvl3.orders_qty();

    auto order = book.sell().first_order();

    ASSERT_EQ( lvl0.price(), order.price );

    order.price = lvl3.price();
    print_book( "After modify", book );

    const order_qty_t order_qty_before = order.qty;
    const order_qty_t additional_qty{ 99 };
    order.qty += additional_qty;

    book.modify_order( order );

    const auto lvl0_orders_count_after = lvl0.orders_count();
    const auto lvl3_orders_count_after = lvl3.orders_count();
    const auto lvl0_orders_qty_after   = lvl0.orders_qty();
    const auto lvl3_orders_qty_after   = lvl3.orders_qty();

    ASSERT_EQ( lvl0_orders_count_after, lvl0_orders_count_before - 1 );
    ASSERT_EQ( lvl3_orders_count_after, lvl3_orders_count_before + 1 );

    ASSERT_EQ( lvl0_orders_qty_after, lvl0_orders_qty_before - order_qty_before );
    ASSERT_EQ( lvl3_orders_qty_after, lvl3_orders_qty_before + order.qty );
}

}  // anonymous namespace

}  // namespace jacobi::book
