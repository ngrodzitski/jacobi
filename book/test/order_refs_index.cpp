#include <jacobi/book/order_refs_index.hpp>

#include <string>

#include <gtest/gtest.h>

namespace jacobi::book
{

namespace /* anonymous */
{

//
// test_order_ref_value_base_t
//

struct test_order_ref_value_base_t
{
    order_t order;

    [[nodiscard]] order_t access_order() const noexcept { return order; }
    [[nodiscard]] int * access_order_reference() noexcept { return nullptr; }

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

//
// test_order_ref_value_t
//

struct test_order_ref_value_t : public test_order_ref_value_base_t
{
    // Some more data that is not handled by book operatioens
    std::string tag;
};

using fake_order_reference_t = int;

using index_std_unordered_map_t =
    order_refs_index_std_unordered_map_t< test_order_ref_value_t,
                                          fake_order_reference_t >;
using index_tsl_robin_map_t =
    order_refs_index_tsl_robin_map_t< test_order_ref_value_t,
                                      fake_order_reference_t >;
using index_boost_unordered_flat_map_t =
    order_refs_index_boost_unordered_flat_map_t< test_order_ref_value_t,
                                                 fake_order_reference_t >;

using index_absl_flat_hash_map_t =
    order_refs_index_absl_flat_hash_map_t< test_order_ref_value_t,
                                           fake_order_reference_t >;

static_assert( Order_Refs_Index_Concept< index_std_unordered_map_t > );
static_assert( Order_Refs_Index_Concept< index_tsl_robin_map_t > );
static_assert( Order_Refs_Index_Concept< index_boost_unordered_flat_map_t > );
static_assert( Order_Refs_Index_Concept< index_absl_flat_hash_map_t > );

template < typename Index_Type >
class JacobiBookOrderIndexConceptTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    Index_Type refs_index{};
};

using OrderRefIndexTypes = ::testing::Types< index_std_unordered_map_t,
                                             index_tsl_robin_map_t,
                                             index_boost_unordered_flat_map_t,
                                             index_absl_flat_hash_map_t >;

TYPED_TEST_SUITE( JacobiBookOrderIndexConceptTest, OrderRefIndexTypes );

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookOrderIndexConceptTest, PriceLevel )
{
    auto & index = this->refs_index;

    std::vector< order_t > orders{
        order_t{ order_id_t{ 0 }, order_qty_t{ 10 }, order_price_t{ 333 } },
        order_t{ order_id_t{ 1 }, order_qty_t{ 222 }, order_price_t{ 223 } },
        order_t{ order_id_t{ 2 }, order_qty_t{ 42 }, order_price_t{ 112 } }
    };
    // ===============================================================
    // Insert some:
    {
        const auto & order = orders[ 0 ];
        // Note: here and further we use test_order_ref_value_base_t
        //       as the type of the value. It is not
        //      the eventual type of the KV container (test_order_ref_value_t).
        auto it = index.insert( order.id,
                                test_order_ref_value_base_t{ .order = order } );

        ASSERT_NE( it, index.end() );

        const order_t order2 = index.access_value( it )->access_order();
        // Must be the same:
        const order_t order3 = index.access_value( it )->access_order();

        EXPECT_EQ( order2.id, order.id );
        EXPECT_EQ( order2.qty, order.qty );
        EXPECT_EQ( order2.price, order.price );

        EXPECT_EQ( order3.id, order.id );
        EXPECT_EQ( order3.qty, order.qty );
        EXPECT_EQ( order3.price, order.price );

        test_order_ref_value_t * val = index.access_value( it );
        ASSERT_NE( val, nullptr );
        ASSERT_EQ( val, index.access_value( it ) );

        val->tag = "tag0";
    }
    {
        const auto & order = orders[ 1 ];
        auto it            = index.insert( order.id,
                                test_order_ref_value_base_t{ .order = order } );

        ASSERT_NE( it, index.end() );

        const order_t order2 = index.access_value( it )->access_order();
        EXPECT_EQ( order2.id, order.id );
        EXPECT_EQ( order2.qty, order.qty );
        EXPECT_EQ( order2.price, order.price );

        test_order_ref_value_t * val = index.access_value( it );
        val->tag                     = "tag1";
    }
    {
        const auto & order = orders[ 2 ];
        auto it            = index.insert( order.id,
                                test_order_ref_value_base_t{ .order = order } );

        ASSERT_NE( it, index.end() );

        const order_t order2 = index.access_value( it )->access_order();
        EXPECT_EQ( order2.id, order.id );
        EXPECT_EQ( order2.qty, order.qty );
        EXPECT_EQ( order2.price, order.price );

        test_order_ref_value_t * val = index.access_value( it );
        val->tag                     = "tag2";
    }

    for( unsigned i = 3U; i < 1000U; ++i )
    {
        orders.emplace_back( order_t{
            order_id_t{ i }, order_qty_t{ i % 20 }, order_price_t{ i / 100 } } );

        const order_t & order = orders.back();
        auto it               = index.insert( order.id,
                                test_order_ref_value_base_t{ .order = order } );

        ASSERT_NE( it, index.end() ) << "i=" << i;

        const order_t order2 = index.access_value( it )->access_order();
        EXPECT_EQ( order2.id, order.id ) << "i=" << i;
        EXPECT_EQ( order2.qty, order.qty ) << "i=" << i;
        EXPECT_EQ( order2.price, order.price ) << "i=" << i;

        test_order_ref_value_t * val = index.access_value( it );
        val->tag                     = "tag" + std::to_string( i );
    }
    // ===============================================================

    // ===============================================================
    // Find some:
    {
        // Id doesn't exist.
        auto it = index.find( order_id_t{ 99999 } );
        ASSERT_EQ( it, index.end() );
    }

    for( unsigned i = 0U; i < 1000U; ++i )
    {
        const auto & order = orders[ i ];
        auto it            = index.find( order.id );
        ASSERT_NE( it, index.end() );

        const order_t order2 = index.access_value( it )->access_order();
        EXPECT_EQ( order2.id, order.id );
        EXPECT_EQ( order2.qty, order.qty );
        EXPECT_EQ( order2.price, order.price );

        test_order_ref_value_t * val = index.access_value( it );
        ASSERT_NE( val, nullptr );
        ASSERT_EQ( val->tag,
                   "tag" + std::to_string( type_safe::get( order.id ) ) );
    }
    // ===============================================================

    // ===============================================================
    // Erase some:

    ASSERT_NE( index.end(), index.find( orders[ 0 ].id ) );
    index.erase( index.find( orders[ 0 ].id ) );
    ASSERT_EQ( index.end(), index.find( orders[ 0 ].id ) );

    ASSERT_NE( index.end(), index.find( orders[ 1 ].id ) );
    index.erase( index.find( orders[ 1 ].id ) );
    ASSERT_EQ( index.end(), index.find( orders[ 1 ].id ) );

    ASSERT_NE( index.end(), index.find( orders[ 2 ].id ) );
    index.erase( index.find( orders[ 2 ].id ) );
    ASSERT_EQ( index.end(), index.find( orders[ 2 ].id ) );

    ASSERT_NE( index.end(), index.find( orders[ 100 ].id ) );
    index.erase( index.find( orders[ 100 ].id ) );
    ASSERT_EQ( index.end(), index.find( orders[ 100 ].id ) );

    ASSERT_NE( index.end(), index.find( orders[ 777 ].id ) );
    index.erase( index.find( orders[ 777 ].id ) );
    ASSERT_EQ( index.end(), index.find( orders[ 777 ].id ) );
}

}  // anonymous namespace

}  // namespace jacobi::book
