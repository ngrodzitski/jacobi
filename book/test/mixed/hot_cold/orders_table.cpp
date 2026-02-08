#include <jacobi/book/mixed/hot_cold/orders_table.hpp>

#include <numeric>

#include <jacobi/book/price_level.hpp>
#include <jacobi/book/order_refs_index.hpp>

#include <gtest/gtest.h>

namespace jacobi::book::mixed::hot_cold
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

#define JACOBI_BOOK_TYPE_ORDERS_TABLE JacobiBookMixedHotColdOrdersTableSell
#define JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE \
    JacobiBookMixedHotColdOrdersTableSellFixture
#define JACOBI_BOOK_ORDERS_TABLE_TYPE test_orders_table_sell_t
#include "../../map/orders_table_reusable_set_of_tests.ipp"
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
#undef JACOBI_BOOK_ORDERS_TABLE_TYPE

}  // anonymous namespace

}  // namespace jacobi::book::mixed::hot_cold
