// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Price level routines.
 */

#pragma once

#include <type_traits>
#include <list>
#include <map>

#include <range/v3/view/subrange.hpp>
#include <range/v3/view/reverse.hpp>

#include <plf_list.h>

#include <jacobi/book/utils/chunk_list.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level_fwd.hpp>

namespace jacobi::book
{

//
// std_chunk_list_traits_t
//

struct std_chunk_list_traits_t
{
    template < typename T >
    using list_t = ::jacobi::book::utils::chunk_list_t< T, std_list_traits_t >;
};

//
// plf_chunk_list_traits_t
//

struct plf_chunk_list_traits_t
{
    template < typename T >
    using list_t = ::jacobi::book::utils::chunk_list_t< T, plf_list_traits_t >;
};

//
// chunked_price_level_t
//

/**
 * @brief A storage of the orders on a given level.
 *
 * List based implementation.
 */
template < List_Traits_Concept List_Traits >
class chunked_price_level_t
{
public:
    /**
     * @brief A storage type for the sequence of orders within a level.
     */
    using orders_container_t = typename List_Traits::list_t< order_t >;

    explicit chunked_price_level_t() = default;

    // We don't want copy this object.
    chunked_price_level_t( const chunked_price_level_t & )             = delete;
    chunked_price_level_t & operator=( const chunked_price_level_t & ) = delete;

    explicit chunked_price_level_t( order_price_t p )
        : m_price{ p }
    {
    }

    friend inline void swap( chunked_price_level_t & lvl1,
                             chunked_price_level_t & lvl2 ) noexcept
    {
        using std::swap;
        swap( lvl1.m_price, lvl2.m_price );
        swap( lvl1.m_orders, lvl2.m_orders );
        swap( lvl1.m_orders_qty, lvl2.m_orders_qty );
    }

    explicit chunked_price_level_t( chunked_price_level_t && lvl )
        : m_price{ lvl.m_price }
        , m_orders{ std::move( lvl.m_orders ) }
        , m_orders_qty{ lvl.m_orders_qty }
    {
        // Let's leave the moved-from object in a valid state.
        // Here we will make it "empty".
        lvl.m_orders_qty = order_qty_t{};
    }

    chunked_price_level_t & operator=( chunked_price_level_t && lvl )
    {
        chunked_price_level_t tmp{ std::move( lvl ) };
        swap( *this, tmp );

        return *this;
    }

    using reference_t = list_based_price_level_order_reference_t<
        typename orders_container_t::iterator >;

    static_assert( Price_Level_Order_Reference_Concept< reference_t > );

    /**
     * @name Manipulation API.
     */
    /// @{
    /**
     * @brief Add an order to this price level.
     */
    [[nodiscard]] reference_t add_order( order_t order )
    {
        assert( order.price == m_price );

        m_orders_qty += order.qty;
        return reference_t{ m_price, m_orders.push_back( order ) };
    }

    /**
     * @brief Remove an order identified by reference.
     */
    void delete_order( const reference_t & ref )
    {
        assert( ref.price() == m_price );

        {
            const auto & order = ref.make_order();
            assert( order.price == price() );
            assert( m_orders_qty >= order.qty );
            m_orders_qty -= order.qty;
        }
        m_orders.erase( ref.iterator() );
    }
    /// @}

    /**
     * @name General attributes of the level.
     */
    /// @{
    /**
     * @brief Get current level price.
     */
    [[nodiscard]] order_price_t price() const noexcept { return m_price; }

    /**
     * @brief Get orders count on a given level.
     */
    [[nodiscard]] std::size_t orders_count() const noexcept
    {
        return m_orders.size();
    }

    /**
     * @brief A total quantity in all orders.
     */
    [[nodiscard]] order_qty_t orders_qty() const noexcept { return m_orders_qty; }

    /**
     * @brief Get an order at a given position.
     */
    [[nodiscard]] order_t order_at( const reference_t & ref )
    {
        assert( ref.price() == m_price );
        return ref.make_order();
    }

    /**
     * @brief Reduce total qty on this level (execute,reduce or modify).
     */
    [[nodiscard]] reference_t reduce_qty( const reference_t & ref,
                                          order_qty_t qty ) noexcept
    {
        assert( ref.price() == m_price );
        assert( m_orders_qty > qty );
        ref.mutable_order().qty -= qty;
        m_orders_qty -= qty;
        return ref;
    }

    /**
     * @brief Is level empty (contains no orders).
     */
    [[nodiscard]] bool empty() const noexcept { return m_orders.empty(); }
    /// @}

    /**
     * @brief Get a range of the order on this level.
     */
    [[nodiscard]] auto orders_range() const noexcept
    {
        return ranges::subrange( m_orders.begin(), m_orders.end() );
    }

    /**
     * @brief Get a range of the order on this level in reverse.
     */
    [[nodiscard]] auto orders_range_reverse() const noexcept
    {
        return orders_range() | ranges::views::reverse;
    }

    /**
     * @brief Get the first order on this level.
     *
     * @pre Level MUST NOT be empty.
     */
    [[nodiscard]] order_t first_order() const noexcept
    {
        assert( !empty() );
        return m_orders.front();
    }

private:
    /**
     * @brief Current level price.
     */
    order_price_t m_price{};

    /**
     * @brief Orders container.
     */
    orders_container_t m_orders{};

    /**
     * @brief A total quantity in all orders.
     */
    order_qty_t m_orders_qty{};
};

//
// chunked_price_levels_factory_t
//

template < typename Price_Level >
class chunked_price_levels_factory_t
{
public:
    using price_level_t = Price_Level;

    [[nodiscard]] price_level_t make_price_level( order_price_t p ) const
    {
        return price_level_t{ p };
    }

    void retire_price_level( [[maybe_unused]] price_level_t && price_level )
    {
        // Do nothing
    }
};

static_assert(
    Price_Level_Concept< chunked_price_level_t< std_chunk_list_traits_t > > );
static_assert(
    Price_Level_Concept< chunked_price_level_t< plf_chunk_list_traits_t > > );

static_assert( Price_Levels_Factory_Concept< chunked_price_levels_factory_t<
                   chunked_price_level_t< std_chunk_list_traits_t > > > );
static_assert( Price_Levels_Factory_Concept< chunked_price_levels_factory_t<
                   chunked_price_level_t< plf_chunk_list_traits_t > > > );

}  // namespace jacobi::book
