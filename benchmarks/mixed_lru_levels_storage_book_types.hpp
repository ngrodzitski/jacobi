#pragma once

#include <stdexcept>

#include <jacobi/book/mixed/lru/orders_table.hpp>

#include "benchmark_helpers.hpp"

namespace jacobi::bench
{

//
// mixed_lru_levels_storage_book_impl_data_t
//

template < typename Price_Level_Factory,
           template < typename V, typename R >
           class Order_Refs_Index >
struct mixed_lru_levels_storage_book_impl_data_t
{
    using price_levels_factory_t = Price_Level_Factory;
    using price_level_t          = typename Price_Level_Factory::price_level_t;

    struct test_order_ref_value_t
    {
        explicit test_order_ref_value_t( price_level_t::reference_t r )
            : ref{ r }
        {
        }

        price_level_t::reference_t ref;
        book::trade_side ts{};

        [[nodiscard]] book::order_t access_order() const noexcept
        {
            return ref.make_order();
        }

        [[nodiscard]] price_level_t::reference_t *
        access_order_reference() noexcept
        {
            return &ref;
        }

        void set_trade_side( book::trade_side t ) noexcept { ts = t; };
        [[nodiscard]] book::trade_side get_trade_side() const noexcept
        {
            return ts;
        }
    };

    using order_refs_index_t =
        Order_Refs_Index< test_order_ref_value_t,
                          typename price_level_t::reference_t >;

    order_refs_index_t order_refs_index{};
    price_levels_factory_t price_levels_factory{};
};

//
// mixed_lru_based_book_traits_t
//

template < typename Impl_Data, book::BSN_Counter_Concept Book_Counter >
struct mixed_lru_based_book_traits_t
{
    // using bsn_counter_t = std_bsn_counter_t;
    using bsn_counter_t = Book_Counter;
    using impl_data_t   = Impl_Data;
    using sell_orders_table_t =
        book::mixed::lru::orders_table_t< impl_data_t, book::trade_side::sell >;
    using buy_orders_table_t =
        book::mixed::lru::orders_table_t< impl_data_t, book::trade_side::buy >;
};

JACOBI_GENERATE_BENCHMARKS_XXX_TYPES( mixed_lru_based_book_traits_t,
                                      mixed_lru_levels_storage_book_impl_data_t );

}  // namespace jacobi::bench
