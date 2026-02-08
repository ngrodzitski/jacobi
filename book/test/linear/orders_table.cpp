#include <jacobi/book/linear/orders_table.hpp>

#include <jacobi/book/price_level.hpp>
#include <jacobi/book/order_refs_index.hpp>

#include <gtest/gtest.h>

namespace jacobi::book::linear
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

namespace v1
{

using test_orders_table_sell_t =
    orders_table_t< book_impl_data_t, trade_side::sell >;

#define JACOBI_BOOK_TYPE_ORDERS_TABLE JacobiBookLinearV1OrdersTableSell
#define JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE \
    JacobiBookLinearV1OrdersTableSellFixture
#define JACOBI_BOOK_ORDERS_TABLE_TYPE test_orders_table_sell_t
#include "../map/orders_table_reusable_set_of_tests.ipp"
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
#undef JACOBI_BOOK_ORDERS_TABLE_TYPE

}  // namespace v1

namespace v2
{

using test_orders_table_sell_t =
    orders_table_t< book_impl_data_t, trade_side::sell >;

#define JACOBI_BOOK_TYPE_ORDERS_TABLE JacobiBookLinearV2OrdersTableSell
#define JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE \
    JacobiBookLinearV2OrdersTableSellFixture
#define JACOBI_BOOK_ORDERS_TABLE_TYPE test_orders_table_sell_t
#include "../map/orders_table_reusable_set_of_tests.ipp"
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
#undef JACOBI_BOOK_ORDERS_TABLE_TYPE

}  // namespace v2

namespace v3
{

using test_orders_table_sell_t =
    orders_table_t< book_impl_data_t, trade_side::sell >;

#define JACOBI_BOOK_TYPE_ORDERS_TABLE JacobiBookLinearV3OrdersTableSell
#define JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE \
    JacobiBookLinearV3OrdersTableSellFixture
#define JACOBI_BOOK_ORDERS_TABLE_TYPE test_orders_table_sell_t
#include "../map/orders_table_reusable_set_of_tests.ipp"
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE
#undef JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
#undef JACOBI_BOOK_ORDERS_TABLE_TYPE

}  // namespace v3

}  // anonymous namespace

}  // namespace jacobi::book::linear
