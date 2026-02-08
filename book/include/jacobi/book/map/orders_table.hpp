// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table implementation based on
 *       ordered-map (price=>price_level) implementation.
 */

#pragma once

#include <type_traits>
#include <map>

#include <absl/container/btree_map.h>

#include <range/v3/view/map.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level.hpp>
#include <jacobi/book/orders_table_crtp_base.hpp>

namespace jacobi::book::map
{

//
// generic_orders_table_t
//

/**
 * @brief Represents a table of orders for a single trade side.
 *
 * @tparam Order_Reference_Table  A table to store orders ids
 *                                Implementation assumes external
 *                                ownership.
 *
 * @tparam Side_Indicator         Which side of the book this table represents.
 *
 * @pre The value type of Order_Reference_Table is expected to
 *      be compatible with price_level_t::reference_t.
 */
template < Book_Impl_Data_Concept Book_Impl_Data,
           trade_side Side_Indicator,
           typename Map_Container_Traits >
class generic_orders_table_t
    : public orders_table_crtp_base_t<
          generic_orders_table_t< Book_Impl_Data,
                                  Side_Indicator,
                                  Map_Container_Traits >,
          Book_Impl_Data,
          Side_Indicator >
{
public:
    // Base class must have access to private functions.
    friend class orders_table_crtp_base_t< generic_orders_table_t,
                                           Book_Impl_Data,
                                           Side_Indicator >;

    using base_type_t = orders_table_crtp_base_t< generic_orders_table_t,
                                                  Book_Impl_Data,
                                                  Side_Indicator >;

    using typename base_type_t::price_level_t;

    // Reuse base constructors.
    using base_type_t::base_type_t;

    [[nodiscard]] bool empty() const noexcept { return m_price_levels.empty(); }

    /**
     * @brief Get top price.
     */
    [[nodiscard]] std::optional< order_price_t > top_price() const noexcept
    {
        if( empty() )
        {
            return std::nullopt;
        }
        return m_price_levels.begin()->first;
    }

    /**
     * @brief Get top price quantity.
     */
    [[nodiscard]] std::optional< order_qty_t > top_price_qty() const noexcept
    {
        if( empty() )
        {
            return std::nullopt;
        }
        return m_price_levels.begin()->second.orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        return m_price_levels | ranges::views::values;
    }

    /**
     * @brief Get the first order (the one that should be matched for execution)
     *        on this table.
     *
     * @pre Table MUST NOT be empty.
     */
    [[nodiscard]] order_t first_order() const noexcept
    {
        assert( !empty() );
        return begin( m_price_levels )->second.first_order();
    }

private:
    /**
     * @brief A type alias for prices' levels' storage.
     */
    using levels_table_t = Map_Container_Traits::template map_t<
        order_price_t,
        price_level_t,
        typename order_price_operations_t< Side_Indicator >::cmp_t >;

    using levels_table_value_t = typename levels_table_t::value_type;

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, typename levels_table_t::iterator >
    level_at( order_price_t price )
    {
        auto it = m_price_levels.lower_bound( price );

        if( it == end( m_price_levels ) || it->first != price )
        {
            it = m_price_levels.insert(
                it,
                levels_table_value_t{
                    price,
                    this->m_book_private_data.price_levels_factory
                        .make_price_level( price ) } );
        }
        return std::make_pair( &( it->second ), it );
    }

    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        assert( !this->empty() );
        return m_price_levels.begin()->second;
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( levels_table_t::iterator lvl_it )
    {
        m_price_levels.erase( lvl_it );
    }

    /**
     * @brief A storage for price levels.
     *
     * Each levels contain a queue of orders at a given price.
     */
    levels_table_t m_price_levels;
};

//
// std_map_container_traits_t
//

/**
 * @brief Traits for std::map.
 */
struct std_map_container_traits_t
{
    template < typename K, typename Value, typename Comparator >
    using map_t = std::map< K, Value, Comparator >;
};

//
// std_map_orders_table_t
//

/**
 * @brief An alies for std::map based map-orders-table.
 */
template < Book_Impl_Data_Concept Book_Impl_Data, trade_side Side_Indicator >
using std_map_orders_table_t =
    generic_orders_table_t< Book_Impl_Data,
                            Side_Indicator,
                            std_map_container_traits_t >;

//
// absl_map_container_traits_t
//

/**
 * @brief Traits for std::map.
 */
struct absl_map_container_traits_t
{
    template < typename K, typename Value, typename Comparator >
    using map_t = absl::btree_map< K, Value, Comparator >;
};

//
// absl_map_orders_table_t
//

/**
 * @brief An alies for std::map based map-orders-table.
 */
template < Book_Impl_Data_Concept Book_Impl_Data, trade_side Side_Indicator >
using absl_map_orders_table_t =
    generic_orders_table_t< Book_Impl_Data,
                            Side_Indicator,
                            absl_map_container_traits_t >;

}  // namespace jacobi::book::map
