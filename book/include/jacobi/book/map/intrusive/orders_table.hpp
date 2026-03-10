// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table implementation based on
 *       intrusive ordered set (price=>price_level) implementation
 *       with pool of reusable nodes.
 */

#pragma once

#include <cassert>
#include <type_traits>

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/any_hook.hpp>

#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level.hpp>
#include <jacobi/book/orders_table_crtp_base.hpp>

namespace jacobi::book::map::intrusive
{

namespace details
{

//
// set_cmp_t
//

/**
 * @brief A Comparator integrator for using with boost::intrusiveL::set.
 */
template < typename Price_Level_Node, trade_side Side_Indicator >
struct set_cmp_t : public order_price_operations_t< Side_Indicator >::cmp_t
{
    using base_t = order_price_operations_t< Side_Indicator >::cmp_t;

    using base_t::operator();

    [[nodiscard]] bool operator()( const Price_Level_Node & n1,
                                   const Price_Level_Node & n2 ) const noexcept
    {
        return ( *this )( n1.plvl()->price(), n2.plvl()->price() );
    }
    [[nodiscard]] bool operator()( const Price_Level_Node & node,
                                   order_price_t p ) const noexcept
    {
        return ( *this )( node.plvl()->price(), p );
    }
    [[nodiscard]] bool operator()( order_price_t p,
                                   const Price_Level_Node & node ) const noexcept
    {
        return ( *this )( p, node.plvl.price() );
    }
};

using node_hook_type_t = boost::intrusive::any_member_hook<>;

//
// price_level_node_t
//

/**
 * @brief Intrusive node that contains meaningful price level.
 */
template < Price_Level_Concept Price_Level >
struct price_level_node_t
{
    node_hook_type_t node_hook;

    /**
     * @brief Initialize the embedded object.
     */
    void init( Price_Level && plvl )
    {
        // Use .data() to get the raw pointer for placement new
        new( payload.data() ) Price_Level( std::move( plvl ) );
#if !defined( NDEBUG )
        assert( !is_active );
        is_active = true;
#endif  // if !defined(NDEBUG)
    }

    /**
     * @brief Extract the object and destroy the embedded instance
     *
     * @return An object of price level to be retired.
     */
    void deinit()
    {
        Price_Level * ptr = plvl();
        ptr->~Price_Level();

#if !defined( NDEBUG )
        assert( is_active );
        is_active = false;
#endif  // if !defined(NDEBUG)
    }

    [[nodiscard]] Price_Level * plvl() noexcept
    {
#if !defined( NDEBUG )
        assert( is_active );
#endif  // if !defined(NDEBUG)
        return reinterpret_cast< Price_Level * >( payload.data() );
    }
    [[nodiscard]] const Price_Level * plvl() const noexcept
    {
#if !defined( NDEBUG )
        assert( is_active );
#endif  // if !defined(NDEBUG)
        return reinterpret_cast< const Price_Level * >( payload.data() );
    }

private:
    /**
     * @brief Raw memory storage for price level object.
     */
    alignas( Price_Level ) std::array< std::byte, sizeof( Price_Level ) > payload;

#if !defined( NDEBUG )
    bool is_active{ false };
#endif  // if !defined(NDEBUG)
};

//
// price_level_node_list_member_t
//

/**
 * @brief Member hook for intrusive list.
 */
template < Price_Level_Concept Price_Level >
using price_level_node_list_member_t =
    boost::intrusive::any_to_list_hook< boost::intrusive::member_hook<
        price_level_node_t< Price_Level >,
        node_hook_type_t,
        &price_level_node_t< Price_Level >::node_hook > >;

//
// price_level_node_set_member_t
//

/**
 * @brief Member hook for intrusive set.
 */
template < Price_Level_Concept Price_Level >
using price_level_node_set_member_t =
    boost::intrusive::any_to_set_hook< boost::intrusive::member_hook<
        price_level_node_t< Price_Level >,
        node_hook_type_t,
        &price_level_node_t< Price_Level >::node_hook > >;

}  // namespace details

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
 * @tparam Nodes_Pool             Nodes pool class.
 *
 * @pre The value type of Order_Reference_Table is expected to
 *      be compatible with price_level_t::reference_t.
 */
template < typename Book_Impl_Data,
           trade_side Side_Indicator,
           typename Nodes_Pool >
class generic_orders_table_t
    : public orders_table_crtp_base_t<
          generic_orders_table_t< Book_Impl_Data, Side_Indicator, Nodes_Pool >,
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

    explicit generic_orders_table_t( Book_Impl_Data & data,
                                     Nodes_Pool * nodes_pool )
        : base_type_t{ data }
        , m_nodes_pool{ nodes_pool }
    {
        assert( m_nodes_pool );
    }

    ~generic_orders_table_t()
    {
        m_price_levels.clear_and_dispose(
            [ this ]( auto * node ) noexcept
            {
                this->m_book_private_data.price_levels_factory.retire_price_level(
                    std::move( *( node->plvl() ) ) );
                node->deinit();
                m_nodes_pool->deallocate( node );
            } );
    }

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
        return m_price_levels.begin()->plvl()->price();
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
        return m_price_levels.begin()->plvl()->orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        return m_price_levels
               | ranges::views::transform(
                   []( const auto & node ) -> const price_level_t &
                   { return *node.plvl(); } );
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
        return m_price_levels.begin()->plvl()->first_order();
    }

private:
    using price_level_node_t = details::price_level_node_t< price_level_t >;
    using cmp_t = details::set_cmp_t< price_level_node_t, Side_Indicator >;
    using price_level_node_set_member_t =
        details::price_level_node_set_member_t< price_level_t >;

    using levels_table_t =
        boost::intrusive::set< price_level_node_t,
                               price_level_node_set_member_t,
                               boost::intrusive::compare< cmp_t > >;

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, typename levels_table_t::iterator >
    level_at( order_price_t price )
    {
        auto it = m_price_levels.lower_bound( price, cmp_t{} );

        if( it == m_price_levels.end() || it->plvl()->price() != price )
        {
            // We must pop the meaningful node from cache
            // to occupy a new position within cached array of nodes.
            auto * new_node = m_nodes_pool->allocate();
            assert( new_node );

            new_node->init(
                this->m_book_private_data.price_levels_factory.make_price_level(
                    price ) );

            it = m_price_levels.insert( it, *new_node );
        }

        return std::make_pair( it->plvl(), it );
    }

    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        assert( !this->empty() );
        return *( m_price_levels.begin()->plvl() );
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( levels_table_t::iterator lvl_it )
    {
        auto * node = &( *lvl_it );
        this->m_book_private_data.price_levels_factory.retire_price_level(
            std::move( *( node->plvl() ) ) );
        m_price_levels.erase( lvl_it );
        node->deinit();
        m_nodes_pool->deallocate( node );
    }

    /**
     * @brief A storage for price levels.
     *
     * Each levels contain a queue of orders at a given price.
     */
    levels_table_t m_price_levels;

    /**
     * @brief A pool for nodes.
     */
    Nodes_Pool * m_nodes_pool;
};

//
// std_nodes_pool_t
//

template < Price_Level_Concept Price_Level, std::size_t Page_Size = 128 >
using std_nodes_pool_t = utils::intrusive_nodes_pool_t<
    details::price_level_node_t< Price_Level >,
    details::price_level_node_list_member_t< Price_Level >,
    Page_Size >;

//
// std_orders_table_nodes_pool_mixin_t
//

template < typename Book_Impl_Data, std::size_t Page_Size = 128 >
class std_orders_table_nodes_pool_mixin_t
{
public:
    explicit std_orders_table_nodes_pool_mixin_t() = default;

    using price_levels_factory_t = typename Book_Impl_Data::price_levels_factory_t;
    using price_level_t          = typename price_levels_factory_t::price_level_t;
    using nodes_pool_t           = std_nodes_pool_t< price_level_t, Page_Size >;

protected:
    nodes_pool_t m_nodes_pool_obj{};
};

//
// std_orders_table_t
//

template < typename Book_Impl_Data, trade_side Side_Indicator >
class std_orders_table_t
    : protected std_orders_table_nodes_pool_mixin_t< Book_Impl_Data >
    , public generic_orders_table_t< Book_Impl_Data,
                                     Side_Indicator,
                                     typename std_orders_table_nodes_pool_mixin_t<
                                         Book_Impl_Data >::nodes_pool_t >
{
public:
    using nodes_pool_mixin_t =
        std_orders_table_nodes_pool_mixin_t< Book_Impl_Data >;
    using nodes_pool_t = typename nodes_pool_mixin_t::nodes_pool_t;
    using base_type_t =
        generic_orders_table_t< Book_Impl_Data, Side_Indicator, nodes_pool_t >;

    explicit std_orders_table_t( Book_Impl_Data & data )
        : base_type_t{ data, &( this->m_nodes_pool_obj ) }
    {
    }
};

}  // namespace jacobi::book::map::intrusive
