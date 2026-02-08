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

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level_fwd.hpp>

namespace jacobi::book
{

//
// std_price_level_t
//

/**
 * @brief A storage of the orders on a given level.
 *
 * List based implementation.
 */
template < List_Traits_Concept List_Traits >
class std_price_level_t
{
public:
    /**
     * @brief A storage type for the sequence of orders within a level.
     */
    using orders_container_t = typename List_Traits::template list_t< order_t >;

    explicit std_price_level_t() = default;

    // We don't want copy this object.
    std_price_level_t( const std_price_level_t & )             = delete;
    std_price_level_t & operator=( const std_price_level_t & ) = delete;

    explicit std_price_level_t( order_price_t p )
        : m_price{ p }
    {
    }

    friend inline void swap( std_price_level_t & lvl1,
                             std_price_level_t & lvl2 ) noexcept
    {
        using std::swap;
        swap( lvl1.m_price, lvl2.m_price );
        swap( lvl1.m_orders, lvl2.m_orders );
        swap( lvl1.m_orders_qty, lvl2.m_orders_qty );
    }

    explicit std_price_level_t( std_price_level_t && lvl )
        : m_price{ lvl.m_price }
        , m_orders{ std::move( lvl.m_orders ) }
        , m_orders_qty{ lvl.m_orders_qty }
    {
        // Let's leave the moved-from object in a valid state.
        // Here we will make it "empty".
        lvl.m_orders_qty = order_qty_t{};
    }

    std_price_level_t & operator=( std_price_level_t && lvl )
    {
        std_price_level_t tmp{ std::move( lvl ) };
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
        return reference_t{ m_price, m_orders.emplace( m_orders.end(), order ) };
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
// std_price_levels_factory_t
//

template < typename Price_Level >
using std_price_levels_factory_t = trivial_price_levels_factory_t< Price_Level >;

static_assert( Price_Level_Concept< std_price_level_t< std_list_traits_t > > );
static_assert( Price_Level_Concept< std_price_level_t< plf_list_traits_t > > );

static_assert(
    Price_Levels_Factory_Concept<
        std_price_levels_factory_t< std_price_level_t< std_list_traits_t > > > );
static_assert(
    Price_Levels_Factory_Concept<
        std_price_levels_factory_t< std_price_level_t< plf_list_traits_t > > > );

//
// shared_list_container_price_level_t
//

/**
 * @brief A storage of the orders on a given level
 *        which leverages a single list container.
 *
 * Each level instance allocates an extra note in the list (which is not used)
 * to mark the end of the list. It serves as a pivot which tells
 * in which position to insert new nodes.
 *
 */
template < List_Traits_Concept List_Traits >
class shared_list_container_price_level_t
{
public:
    /**
     * @brief A storage type for the sequence of orders within a level.
     */
    using orders_container_t = typename List_Traits::template list_t< order_t >;

    /**
     * @brief An iterator type.
     */
    using iterator_t = typename orders_container_t::iterator;

    // We don't want copy this object.
    shared_list_container_price_level_t(
        const shared_list_container_price_level_t & ) = delete;
    shared_list_container_price_level_t & operator=(
        const shared_list_container_price_level_t & ) = delete;

    /**
     * @brief Init constructor.
     *
     * @pre orders must not be `nullptr`.
     * @post One extra node is added to a shared list container.
     */
    explicit shared_list_container_price_level_t( order_price_t p,
                                                  orders_container_t * orders )
        : m_price{ p }
        , m_orders{ orders }
        , m_begin{ m_orders->insert( m_orders->end(), order_t{} ) }
        , m_end{ m_begin }
    {
        assert( this->is_valid() );
    }

    friend inline void swap( shared_list_container_price_level_t & lvl1,
                             shared_list_container_price_level_t & lvl2 ) noexcept
    {
        using std::swap;
        swap( lvl1.m_price, lvl2.m_price );
        swap( lvl1.m_orders, lvl2.m_orders );
        swap( lvl1.m_begin, lvl2.m_begin );
        swap( lvl1.m_end, lvl2.m_end );
        swap( lvl1.m_orders_count, lvl2.m_orders_count );
        swap( lvl1.m_orders_qty, lvl2.m_orders_qty );
    }

    explicit shared_list_container_price_level_t(
        shared_list_container_price_level_t && lvl )
        : m_price{ lvl.m_price }
        , m_orders{ lvl.m_orders }
        , m_begin{ std::move( lvl.m_begin ) }
        , m_end{ std::move( lvl.m_end ) }
        , m_orders_count{ lvl.m_orders_count }
        , m_orders_qty{ lvl.m_orders_qty }
    {
        assert( this->is_valid() );
        // The former instance becomes invalid.
        lvl.m_orders = nullptr;
    }

    ~shared_list_container_price_level_t()
    {
        if( this->is_valid() )
        {
            ++m_end;
            m_orders->erase( m_begin, m_end );
        }
    }

    shared_list_container_price_level_t & operator=(
        shared_list_container_price_level_t && lvl )
    {
        // The former instance becomes invalid.
        shared_list_container_price_level_t tmp{ std::move( lvl ) };
        assert( !lvl.is_valid() );

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
        assert( this->is_valid() );
        assert( order.price == m_price );

        m_orders_qty += order.qty;
        ++m_orders_count;

        auto it = m_orders->emplace( m_end, order );

        if( m_begin == m_end )
        {
            m_begin = it;
        }

        return reference_t{ m_price, it };
    }

    /**
     * @brief Remove an order identified by reference.
     */
    void delete_order( const reference_t & ref )
    {
        assert( this->is_valid() );
        assert( ref.price() == m_price );

        {
            const auto & order = ref.mutable_order();

            assert( order.price == price() );
            assert( m_orders_qty >= order.qty );

            m_orders_qty -= order.qty;
            --m_orders_count;
        }

        if( m_begin == ref.iterator() )
        {
            ++m_begin;
        }

        m_orders->erase( ref.iterator() );
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
        assert( m_orders_count
                == static_cast< std::size_t >( std::distance( m_begin, m_end ) ) );
        return m_orders_count;
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
     * @brief Is level empty (contains no orders).
     */
    [[nodiscard]] bool empty() const noexcept { return 0ULL == m_orders_count; }
    /// @}

    /**
     * @brief Get a range of the order on this level.
     */
    [[nodiscard]] auto orders_range() const noexcept
    {
        return ranges::subrange( m_begin, m_end );
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
        return *m_begin;
    }

    [[nodiscard]] bool is_valid() const noexcept { return nullptr != m_orders; }

private:
    /**
     * @brief Current level price.
     */
    order_price_t m_price{};

    /**
     * @brief Orders container.
     */
    orders_container_t * m_orders{};

    /**
     * @name This level range within a shared list.
     */
    /// @{
    iterator_t m_begin;
    iterator_t m_end;
    /// @}

    /**
     * @brief A total quantity in all orders.
     */
    std::size_t m_orders_count{};

    /**
     * @brief A total quantity in all orders.
     */
    order_qty_t m_orders_qty{};
};

//
// shared_list_container_price_levels_factory_t
//

template < typename Price_Level >
class shared_list_container_price_levels_factory_t
{
public:
    using price_level_t      = Price_Level;
    using orders_container_t = price_level_t::orders_container_t;

    [[nodiscard]] Price_Level make_price_level( order_price_t p )
    {
        return Price_Level{ p, &m_shared_list };
    }

    void retire_price_level( [[maybe_unused]] price_level_t && price_level )
    {
        // Do nothing
    }

private:
    /**
     * @brief Shared list container.
     */
    orders_container_t m_shared_list;
};

static_assert( Price_Level_Concept<
               shared_list_container_price_level_t< std_list_traits_t > > );
static_assert( Price_Level_Concept<
               shared_list_container_price_level_t< plf_list_traits_t > > );

static_assert(
    Price_Levels_Factory_Concept< shared_list_container_price_levels_factory_t<
        shared_list_container_price_level_t< std_list_traits_t > > > );
static_assert(
    Price_Levels_Factory_Concept< shared_list_container_price_levels_factory_t<
        shared_list_container_price_level_t< plf_list_traits_t > > > );

}  // namespace jacobi::book
