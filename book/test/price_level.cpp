#include <jacobi/book/price_level.hpp>
#include <jacobi/book/soa_price_level.hpp>
#include <jacobi/book/chunked_price_level.hpp>
#include <jacobi/book/chunked_soa_price_level.hpp>

#include <array>
#include <random>

#include <range/v3/view/zip.hpp>
#include <range/v3/view/iota.hpp>

#include <gtest/gtest.h>

namespace jacobi::book
{

namespace /* anonymous */
{

template < typename T >
class JacobiBookPriceLevelTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    T price_levels_factory;
};

using PriceLevelImplementations = ::testing::Types<
    std_price_levels_factory_t< std_price_level_t< std_list_traits_t > >,
    std_price_levels_factory_t< std_price_level_t< plf_list_traits_t > >,

    shared_list_container_price_levels_factory_t<
        shared_list_container_price_level_t< std_list_traits_t > >,
    shared_list_container_price_levels_factory_t<
        shared_list_container_price_level_t< plf_list_traits_t > >,

    soa_price_levels_factory_t<
        soa_price_level_t< std_vector_soa_price_level_traits_t > >,
    soa_price_levels_factory_t<
        soa_price_level_t< boost_smallvec_soa_price_level_traits_t< 8 > > >,
    soa_price_levels_factory_t<
        soa_price_level_t< boost_smallvec_soa_price_level_traits_t< 16 > > >,
    soa_price_levels_factory_t<
        soa_price_level_t< boost_smallvec_soa_price_level_traits_t< 32 > > >,

    chunked_price_levels_factory_t<
        chunked_price_level_t< std_chunk_list_traits_t > >,
    chunked_price_levels_factory_t<
        chunked_price_level_t< plf_chunk_list_traits_t > >,

    chunked_soa_price_levels_factory_t<
        chunked_soa_price_level_t< std_list_traits_t > >,
    chunked_soa_price_levels_factory_t<
        chunked_soa_price_level_t< plf_list_traits_t > > >;

TYPED_TEST_SUITE( JacobiBookPriceLevelTest, PriceLevelImplementations );

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookPriceLevelTest, PriceLevel )
{
    // Adds 3 orders to price levels.
    // tests move-constructor/assignment nd swap
    // removes orders.

    auto plvl =
        this->price_levels_factory.make_price_level( order_price_t{ 100LL } );

    using price_level_t    = decltype( plvl );
    using reference_type_t = price_level_t::reference_t;

    ASSERT_TRUE( plvl.empty() );
    ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
    ASSERT_EQ( order_qty_t{ 0UL }, plvl.orders_qty() );
    ASSERT_EQ( 0ULL, plvl.orders_count() );

    std::array< reference_type_t, 3 > refs;

    {
        const order_t order0{ .id    = order_id_t{ 999ULL },
                              .qty   = order_qty_t{ 32UL },
                              .price = order_price_t{ 100LL } };
        auto ref = plvl.add_order( order0 );

        ASSERT_EQ( order_price_t{ 100LL }, ref.price() );
        ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
        ASSERT_EQ( order_qty_t{ 32UL }, plvl.orders_qty() );
        ASSERT_EQ( 1ULL, plvl.orders_count() );

        const auto order = plvl.order_at( ref );
        ASSERT_EQ( order.id, order0.id );
        ASSERT_EQ( order.qty, order0.qty );
        ASSERT_EQ( order.price, order0.price );

        refs[ 0 ].copy_from( ref );
    }

    {
        const order_t order0{ .id    = order_id_t{ 123123ULL },
                              .qty   = order_qty_t{ 111UL },
                              .price = order_price_t{ 100LL } };
        auto ref = plvl.add_order( order0 );

        ASSERT_EQ( order_price_t{ 100LL }, ref.price() );
        ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
        ASSERT_EQ( order_qty_t{ 143UL }, plvl.orders_qty() );
        ASSERT_EQ( 2ULL, plvl.orders_count() );

        const auto order = plvl.order_at( ref );
        ASSERT_EQ( order.id, order0.id );
        ASSERT_EQ( order.qty, order0.qty );
        ASSERT_EQ( order.price, order0.price );

        refs[ 1 ].copy_from( ref );
    }

    {
        const order_t order0{ .id    = order_id_t{ 111222333ULL },
                              .qty   = order_qty_t{ 7UL },
                              .price = order_price_t{ 100LL } };
        auto ref = plvl.add_order( order0 );

        ASSERT_EQ( order_price_t{ 100LL }, ref.price() );
        ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
        ASSERT_EQ( order_qty_t{ 150UL }, plvl.orders_qty() );
        ASSERT_EQ( 3ULL, plvl.orders_count() );

        const auto order = plvl.order_at( ref );
        ASSERT_EQ( order.id, order0.id );
        ASSERT_EQ( order.qty, order0.qty );
        ASSERT_EQ( order.price, order0.price );

        refs[ 2 ].copy_from( ref );
    }

    {
        // Move ctor:
        price_level_t plvl2{ std::move( plvl ) };

        ASSERT_EQ( order_price_t{ 100LL }, plvl2.price() );
        ASSERT_EQ( order_qty_t{ 150UL }, plvl2.orders_qty() );
        ASSERT_EQ( 3ULL, plvl2.orders_count() );

        // Move assignment
        auto plvl3 =
            this->price_levels_factory.make_price_level( order_price_t{ 999LL } );

        ASSERT_EQ( order_price_t{ 999LL }, plvl3.price() );
        ASSERT_EQ( order_qty_t{ 0UL }, plvl3.orders_qty() );
        ASSERT_EQ( 0ULL, plvl3.orders_count() );

        plvl3 = std::move( plvl2 );
        ASSERT_EQ( order_price_t{ 100LL }, plvl3.price() );
        ASSERT_EQ( order_qty_t{ 150UL }, plvl3.orders_qty() );
        ASSERT_EQ( 3ULL, plvl3.orders_count() );

        // Swap:
        swap( plvl3, plvl );
        ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
        ASSERT_EQ( order_qty_t{ 150UL }, plvl.orders_qty() );
        ASSERT_EQ( 3ULL, plvl.orders_count() );
    }

    // Reduce qty
    const auto qty_before   = plvl.order_at( refs[ 1 ] ).qty;
    const auto order_before = refs[ 1 ].make_order();
    refs[ 1 ]               = plvl.reduce_qty( refs[ 1 ], order_qty_t{ 100 } );
    const auto qty_after    = plvl.order_at( refs[ 1 ] ).qty;
    const auto order_after  = refs[ 1 ].make_order();

    ASSERT_EQ( qty_before - order_qty_t{ 100 }, qty_after );
    ASSERT_EQ( qty_before, order_before.qty );
    ASSERT_EQ( qty_after, order_after.qty );
    ASSERT_EQ( order_before.id, order_after.id );
    ASSERT_EQ( order_before.price, order_after.price );

    ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
    ASSERT_EQ( order_qty_t{ 50UL }, plvl.orders_qty() );
    ASSERT_EQ( 3ULL, plvl.orders_count() );

    // Remove orders.
    plvl.delete_order( refs[ 1 ] );
    ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
    ASSERT_EQ( order_qty_t{ 39UL }, plvl.orders_qty() );
    ASSERT_EQ( 2ULL, plvl.orders_count() );

    plvl.delete_order( refs[ 0 ] );
    ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
    ASSERT_EQ( order_qty_t{ 7UL }, plvl.orders_qty() );
    ASSERT_EQ( 1ULL, plvl.orders_count() );

    plvl.delete_order( refs[ 2 ] );
    ASSERT_EQ( order_price_t{ 100LL }, plvl.price() );
    ASSERT_EQ( order_qty_t{ 0UL }, plvl.orders_qty() );
    ASSERT_EQ( 0ULL, plvl.orders_count() );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookPriceLevelTest, CheckCorrectOrder )
{
    auto plvl =
        this->price_levels_factory.make_price_level( order_price_t{ 100LL } );

    std::vector< order_t > orders{
        order_t{ order_id_t{ 1 }, order_qty_t{ 50 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 2 }, order_qty_t{ 100 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 3 }, order_qty_t{ 42 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 4 }, order_qty_t{ 10 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 22 }, order_qty_t{ 1111 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 123123 }, order_qty_t{ 1 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 3232 }, order_qty_t{ 21 }, order_price_t{ 100LL } },
        order_t{ order_id_t{ 123 }, order_qty_t{ 908 }, order_price_t{ 100LL } },
    };

    for( const auto & order : orders )
    {
        std::ignore = plvl.add_order( order );
    }

    for( const auto [ i, book_order ] :
         ranges::views::zip( ranges::views::iota( 0U ), plvl.orders_range() ) )
    {
        const auto order = orders.at( i );
        ASSERT_EQ( order.id, book_order.id ) << "i=" << i;
        ASSERT_EQ( order.qty, book_order.qty ) << "i=" << i;
        ASSERT_EQ( order.price, book_order.price ) << "i=" << i;
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookPriceLevelTest, AddManyRemoveMany )
{

    auto plvl =
        this->price_levels_factory.make_price_level( order_price_t{ 100LL } );

    using price_level_t    = decltype( plvl );
    using reference_type_t = price_level_t::reference_t;

    std::vector< reference_type_t > refs;
    refs.reserve( 10'000UL );

    for( auto i = 1UL; i <= 10'000UL; ++i )
    {
        const order_t order0{ .id    = order_id_t{ i },
                              .qty   = order_qty_t{ 42 },
                              .price = order_price_t{ 100LL } };

        refs.push_back( plvl.add_order( order0 ) );
    }

    ASSERT_EQ( plvl.orders_count(), 10'000UL );
    ASSERT_EQ( plvl.orders_qty(), order_qty_t{ 420'000UL } );

    for( auto i = 0UL; i < 1'000UL; ++i )
    {
        plvl.delete_order( refs[ i ] );
    }

    ASSERT_EQ( plvl.orders_count(), 9'000UL );
    ASSERT_EQ( plvl.orders_qty(), order_qty_t{ 42 * 9'000UL } );

    for( auto i = 9000UL; i < 10'000UL; ++i )
    {
        plvl.delete_order( refs[ i ] );
    }

    ASSERT_EQ( plvl.orders_count(), 8'000UL );
    ASSERT_EQ( plvl.orders_qty(), order_qty_t{ 42 * 8'000UL } );

    refs.resize( 9'000UL );
    refs.erase( refs.begin(), refs.begin() + 1'000UL );

    const auto indexes = [ & ]
    {
        std::vector< unsigned > indexes{};
        indexes.resize( refs.size() );
        std::iota( indexes.begin(), indexes.end(), 0U );

        std::random_device rd;
        std::mt19937 g( rd() );
        std::shuffle( indexes.begin(), indexes.end(), g );

        return indexes;
    }();

    for( const auto i : indexes )
    {
        plvl.delete_order( refs[ i ] );
    }

    ASSERT_EQ( plvl.orders_count(), 0UL );
    ASSERT_EQ( plvl.orders_qty(), order_qty_t{ 0UL } );
}

}  // anonymous namespace

}  // namespace jacobi::book
