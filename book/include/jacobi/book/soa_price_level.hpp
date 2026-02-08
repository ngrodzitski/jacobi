// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Price level routines.
 */

#pragma once

#include <type_traits>
#include <vector>

#include <range/v3/view/generate.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <type_safe/strong_typedef.hpp>

#include <boost/container/small_vector.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level_fwd.hpp>

namespace jacobi::book
{

//
// SOA_Price_Level_Traits_Concept
//

/**
 * @brief A concept that defines what book traits must guarantee.
 */
template < typename SOA_Price_Level_Traits >
concept SOA_Price_Level_Traits_Concept = requires {
    typename SOA_Price_Level_Traits::node_index_t;
    typename SOA_Price_Level_Traits::template linear_container_t< int >;
    typename SOA_Price_Level_Traits::template links_container_t< int >;
};

//
// std_vector_soa_price_level_traits_t
//

struct std_vector_soa_price_level_traits_t
{
    using node_index_t = unsigned;

    template < typename T >
    using linear_container_t = std::vector< T >;
    template < typename T >
    using links_container_t = std::vector< T >;
};

//
// boost_smallvec_soa_price_level_traits_t
//

template < std::size_t N >
struct boost_smallvec_soa_price_level_traits_t
{
    using node_index_t = unsigned;

    template < typename T >
    using linear_container_t = boost::container::small_vector< T, N >;
    template < typename T >
    using links_container_t = boost::container::small_vector< T, N + 2 >;
};

//
// soa_price_level_order_node_pos_t
//

/**
 * @brief Strong type for order position in SOA.
 */
template < typename Integer >
struct soa_price_level_order_node_pos_t
    : type_safe::strong_typedef< soa_price_level_order_node_pos_t< Integer >,
                                 Integer >
    , type_safe::strong_typedef_op::equality_comparison<
          soa_price_level_order_node_pos_t< Integer > >
    , type_safe::strong_typedef_op::relational_comparison<
          soa_price_level_order_node_pos_t< Integer > >
{
    using base_t =
        type_safe::strong_typedef< soa_price_level_order_node_pos_t< Integer >,
                                   Integer >;
    using base_t::base_t;

    inline static constexpr Integer usable_nodes_offset{ 2 };

    [[nodiscard]] constexpr std::size_t data_index() const noexcept
    {
        assert( type_safe::get( *this ) >= usable_nodes_offset );
        return type_safe::get( *this ) - usable_nodes_offset;
    }

    [[nodiscard]] constexpr std::size_t node_link_index() const noexcept
    {
        return static_cast< std::size_t >( type_safe::get( *this ) );
    }
};

//
// soa_price_level_order_reference_t
//

/**
 * @brief Reference agregate to locate the order exactly.
 */
template < typename Node_Position_Type >
struct soa_price_level_order_reference_t
{
public:
    explicit soa_price_level_order_reference_t() = default;

    explicit soa_price_level_order_reference_t( order_t order,
                                                Node_Position_Type i )
        : m_order{ order }
        , m_pos{ i }
    {
    }

    [[nodiscard]] order_price_t price() const noexcept { return m_order.price; }

    [[nodiscard]] order_t make_order() const noexcept { return m_order; }

    /**
     * @brief Copy reference from another instance.
     */
    void copy_from( const soa_price_level_order_reference_t & ref )
    {
        m_order = ref.m_order;
        m_pos   = ref.m_pos;
    }

    [[nodiscard]] Node_Position_Type pos() const noexcept { return m_pos; }

private:
    order_t m_order;
    Node_Position_Type m_pos;
};

//
// soa_price_level_t
//

/**
 * @brief A storage of the orders on a given level using SOA to store data.
 */
template < SOA_Price_Level_Traits_Concept Traits >
class soa_price_level_t
{
public:
    using index_t    = Traits::node_index_t;
    using position_t = soa_price_level_order_node_pos_t< index_t >;

    explicit soa_price_level_t() = default;

    // We don't want copy this object.
    soa_price_level_t( const soa_price_level_t & )             = delete;
    soa_price_level_t & operator=( const soa_price_level_t & ) = delete;

    explicit soa_price_level_t( order_price_t p )
        : m_price{ p }
    {
        // Orders list on that level initialy empty.
        m_links_storage.push_back(
            node_t{ .prev = position_t{ 0 }, .next = position_t{ 0 } } );

        // Free nodes list on that level initialy empty.
        m_links_storage.push_back(
            node_t{ .prev = position_t{ 1 }, .next = position_t{ 1 } } );
    }

    friend inline void swap( soa_price_level_t & lvl1,
                             soa_price_level_t & lvl2 ) noexcept
    {
        using std::swap;
        swap( lvl1.m_price, lvl2.m_price );
        swap( lvl1.m_orders_qty, lvl2.m_orders_qty );
        swap( lvl1.m_orders_count, lvl2.m_orders_count );
        swap( lvl1.m_ids, lvl2.m_ids );
        swap( lvl1.m_qtys, lvl2.m_qtys );
        swap( lvl1.m_links_storage, lvl2.m_links_storage );
    }

    explicit soa_price_level_t( soa_price_level_t && lvl )
        : m_price{ lvl.m_price }
        , m_orders_qty{ lvl.m_orders_qty }
        , m_orders_count{ lvl.m_orders_count }
        , m_ids{ std::move( lvl.m_ids ) }
        , m_qtys{ std::move( lvl.m_qtys ) }
        , m_links_storage{ std::move( lvl.m_links_storage ) }
    {
        lvl.m_orders_qty   = order_qty_t{};
        lvl.m_orders_count = 0;
    }

    soa_price_level_t & operator=( soa_price_level_t && lvl )
    {
        soa_price_level_t tmp{ std::move( lvl ) };
        swap( *this, tmp );

        return *this;
    }

    using reference_t = soa_price_level_order_reference_t< position_t >;

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

        const auto ix = allocate_node();

        m_ids[ ix.data_index() ]  = order.id;
        m_qtys[ ix.data_index() ] = order.qty;

        m_orders_qty += order.qty;
        ++m_orders_count;

        insert_node( ix, nodes_anchor_pos );

        return reference_t{ order, ix };
    }

    /**
     * @brief Remove an order identified by reference.
     */
    void delete_order( const reference_t & ref )
    {
        assert( ref.price() == m_price );

        assert( ref.pos().data_index() < m_ids.size() );
        assert( ref.pos().data_index() < m_qtys.size() );
        assert( m_orders_qty >= m_qtys[ ref.pos().data_index() ] );

        m_orders_qty -= m_qtys[ ref.pos().data_index() ];
        assert( m_orders_count > 0 );
        --m_orders_count;

        unlink_node( ref.pos() );
        insert_node( ref.pos(), free_nodes_anchor_pos );
    }

    /**
     * @brief Reduce total qty on this level (execute,reduce or modify).
     */
    [[nodiscard]] reference_t reduce_qty( const reference_t & ref,
                                          order_qty_t qty ) noexcept
    {
        assert( ref.price() == m_price );
        assert( m_orders_qty > qty );

        const auto i = ref.pos().data_index();
        assert( i < m_ids.size() );
        assert( i < m_qtys.size() );
        assert( qty < m_qtys[ i ] );

        m_qtys[ i ] -= qty;
        m_orders_qty -= qty;

        return reference_t{ this->order_at( ref ), ref.pos() };
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
        return m_orders_count;
    }

    /**
     * @brief A total quantity in all orders.
     */
    [[nodiscard]] order_qty_t orders_qty() const noexcept { return m_orders_qty; }

    [[nodiscard]] order_t order_at( const reference_t & ref )
    {
        assert( ref.price() == m_price );
        assert( orders_count() > 0 );

        const auto i = ref.pos().data_index();
        assert( i < m_ids.size() );
        assert( i < m_qtys.size() );

        return order_t{ m_ids[ i ], m_qtys[ i ], m_price };
    }

    /**
     * @brief Is level empty (contains no orders).
     */
    [[nodiscard]] bool empty() const noexcept { return 0 == orders_count(); }
    /// @}

    /**
     * @brief Get a range of the order on this level.
     */
    [[nodiscard]] auto orders_range() const noexcept
    {
        const auto head =
            m_links_storage[ nodes_anchor_pos.node_link_index() ].next;

        return ranges::views::generate(
                   [ i = head, this ]() mutable
                   {
                       const auto current = i;
                       i = m_links_storage[ i.node_link_index() ].next;
                       return current;
                   } )
               | ranges::views::take_while( []( auto i )
                                            { return i != nodes_anchor_pos; } )
               | ranges::views::transform(
                   [ this ]( auto i ) -> order_t
                   {
                       return order_t{ m_ids[ i.data_index() ],
                                       m_qtys[ i.data_index() ],
                                       m_price };
                   } );
    }

    /**
     * @brief Get a range of the order on this level in reverse.
     */
    [[nodiscard]] auto orders_range_reverse() const noexcept
    {
        const auto head =
            m_links_storage[ nodes_anchor_pos.node_link_index() ].prev;

        return ranges::views::generate(
                   [ i = head, this ]() mutable
                   {
                       const auto current = i;
                       i = m_links_storage[ i.node_link_index() ].prev;
                       return current;
                   } )
               | ranges::views::take_while( []( auto i )
                                            { return i != nodes_anchor_pos; } )
               | ranges::views::transform(
                   [ this ]( auto i ) -> order_t
                   {
                       return order_t{ m_ids[ i.data_index() ],
                                       m_qtys[ i.data_index() ],
                                       m_price };
                   } );
    }

    /**
     * @brief Get the first order on this level.
     *
     * @pre Level MUST NOT be empty.
     */
    [[nodiscard]] order_t first_order() const noexcept
    {
        assert( !empty() );
        const auto i = m_links_storage[ nodes_anchor_pos.node_link_index() ].next;
        assert( i >= position_t::usable_nodes_offset.node_link_index() );
        return order_t{ m_ids[ i ], m_qtys[ i ], m_price };
    }

private:
    inline static constexpr position_t nodes_anchor_pos{ 0 };
    inline static constexpr position_t free_nodes_anchor_pos{ 1 };

    void unlink_node( position_t pos ) noexcept
    {
        const auto prev = m_links_storage[ pos.node_link_index() ].prev;
        const auto next = m_links_storage[ pos.node_link_index() ].next;

        // Make neighbours of eleminated node linked:
        m_links_storage[ prev.node_link_index() ].next = next;
        m_links_storage[ next.node_link_index() ].prev = prev;
    }

    void insert_node( position_t i, position_t pos ) noexcept
    {
        // Here: we have i-th node disconnected from the list.
        //       What we need to do is to add i-th as new node
        //       before pos-node.

        const auto t = m_links_storage[ pos.node_link_index() ].prev;

        // Set new linkage for i-th node.
        m_links_storage[ i.node_link_index() ].next = pos;
        m_links_storage[ i.node_link_index() ].prev = t;

        m_links_storage[ pos.node_link_index() ].prev = i;
        m_links_storage[ t.node_link_index() ].next   = i;
    }

    /**
     * @brief Allocate new node to assigned for storing an order.
     */
    [[nodiscard]] position_t allocate_node()
    {
        constexpr auto anchor = free_nodes_anchor_pos.node_link_index();
        position_t pos{ m_links_storage[ anchor ].prev };
        if( pos != free_nodes_anchor_pos ) [[likely]]
        {
            // We have a reusable free node.
            unlink_node( pos );
        }
        else
        {
            // Need to add a new node.
            assert( m_links_storage.size()
                    < std::numeric_limits< index_t >::max() );
            pos = position_t{ static_cast< index_t >( m_links_storage.size() ) };
            m_links_storage.push_back( {} );
            m_ids.resize( m_ids.size() + 1 );
            m_qtys.resize( m_qtys.size() + 1 );
        }

        return pos;
    }

    /**
     * @brief Current level price.
     */
    order_price_t m_price{};

    /**
     * @brief A total quantity in all orders.
     */
    order_qty_t m_orders_qty{};

    /**
     * @brief A total number of orders at this price level.
     */
    std::size_t m_orders_count{};

    using id_container_t  = Traits::template linear_container_t< order_id_t >;
    using qty_container_t = Traits::template linear_container_t< order_qty_t >;

    id_container_t m_ids{};
    qty_container_t m_qtys{};

    //
    // node_t
    //

    /**
     * @brief An internal list referencing routine.
     *
     * The purpose is to keep track of orders within this price level.
     * It enables us to keep correct orders in order as they were added.
     *
     * @note The first item is reserved to be an anchor for the list
     *       that is a sequence of orders on this level.
     *
     * @note The second item is reserved to be an anchor for the list
     *       of free nodes.
     */
    struct node_t
    {
        position_t prev;
        position_t next;
    };

    using links_container_t = Traits::template links_container_t< node_t >;

    /**
     * @brief This is where nodes with links are located.
     */
    links_container_t m_links_storage{};
};

static_assert( Price_Level_Concept<
               soa_price_level_t< std_vector_soa_price_level_traits_t > > );

//
// soa_price_levels_factory_t
//

template < typename Price_Level >
using soa_price_levels_factory_t = trivial_price_levels_factory_t< Price_Level >;

static_assert( Price_Levels_Factory_Concept< soa_price_levels_factory_t<
                   soa_price_level_t< std_vector_soa_price_level_traits_t > > > );

}  // namespace jacobi::book
