// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Price level routines.
 */

#pragma once

#include <type_traits>
#include <vector>

#include <range/v3/view/generate.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <type_safe/strong_typedef.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level_fwd.hpp>

namespace jacobi::book
{

//
// chunked_soa_price_level_order_reference_t
//

/**
 * @brief Reference agregate to locate the order exactly
 *        .
 */
template < typename Iterator >
struct chunked_soa_price_level_order_reference_t
{
public:
    explicit chunked_soa_price_level_order_reference_t() = default;

    explicit chunked_soa_price_level_order_reference_t( order_price_t price,
                                                        Iterator chunk_it,
                                                        std::uint8_t pos )
        : m_price{ price }
        , m_chunk_it{ chunk_it }
        , m_pos{ pos }
    {
    }

    [[nodiscard]] order_price_t price() const noexcept { return m_price; }

    [[nodiscard]] order_t make_order() const noexcept
    {
        return order_t{ .id    = m_chunk_it->id[ m_pos ],
                        .qty   = m_chunk_it->qty[ m_pos ],
                        .price = m_price };
    }

    /**
     * @brief Copy reference from another instance.
     */
    void copy_from( const chunked_soa_price_level_order_reference_t & ref )
    {
        m_price    = ref.m_price;
        m_chunk_it = ref.m_chunk_it;
        m_pos      = ref.m_pos;
    }

    [[nodiscard]] Iterator chunk_it() const noexcept { return m_chunk_it; }

    [[nodiscard]] std::uint8_t pos() const noexcept { return m_pos; }

private:
    order_price_t m_price;
    Iterator m_chunk_it;
    std::uint8_t m_pos;
};

namespace details
{

//
// soa_chunk_node_t
//

struct soa_chunk_node_t
{
    inline static constexpr std::uint8_t chunk_size{ 16 - 2 };
    inline static constexpr std::uint8_t head_pos{ 14 };
    inline static constexpr std::uint8_t free_head_pos{ 15 };

    struct links_t
    {
        std::uint8_t prev : 4;
        std::uint8_t next : 4;
    };

    using links_array_t = std::array< links_t, chunk_size + 2 >;

    links_array_t links{
        links_t{ .prev = free_head_pos, .next = 1 },
        links_t{ .prev = 0, .next = 2 },
        links_t{ .prev = 1, .next = 3 },
        links_t{ .prev = 2, .next = 4 },
        links_t{ .prev = 3, .next = 5 },
        links_t{ .prev = 4, .next = 6 },
        links_t{ .prev = 5, .next = 7 },
        links_t{ .prev = 6, .next = 8 },
        links_t{ .prev = 7, .next = 9 },
        links_t{ .prev = 8, .next = 10 },
        links_t{ .prev = 9, .next = 11 },
        links_t{ .prev = 10, .next = 12 },
        links_t{ .prev = 11, .next = 13 },
        links_t{ .prev = 12, .next = free_head_pos },

        // Anchor nodes for head and free head.

        // HEAD
        links_t{ .prev = head_pos, .next = head_pos },

        // FREE HEAD
        links_t{ .prev = 13, .next = 0 },

    };

    std::array< order_qty_t, chunk_size > qty;
    std::array< order_id_t, chunk_size > id;

    void unlink_node( std::uint8_t pos ) noexcept
    {
        const auto prev = links[ pos ].prev;
        const auto next = links[ pos ].next;

        // Make neighbours of eleminated node linked:
        links[ prev ].next = next;
        links[ next ].prev = prev;
    }

    void insert_node( std::uint8_t pos, std::int8_t hp ) noexcept
    {
        // Here: we have i-th node disconnected from the list.
        //       What we need to do is to add i-th as new node
        //       before pos-node.

        const auto t = links[ hp ].prev;

        // Set new linkage for i-th node.
        links[ pos ].next = hp;
        links[ pos ].prev = t;

        links[ hp ].prev = pos;
        links[ t ].next  = pos;
    }

    [[nodiscard]] constexpr bool full() const noexcept
    {
        return links[ free_head_pos ].prev == free_head_pos;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return links[ head_pos ].prev == head_pos;
    }

    [[nodiscard]] std::uint8_t push_back( order_id_t id_value,
                                          order_qty_t qty_value )
    {
        assert( !full() );
        const auto pos = links[ free_head_pos ].next;

        unlink_node( pos );
        qty[ pos ] = qty_value;
        id[ pos ]  = id_value;

        insert_node( pos, head_pos );
        return pos;
    }

    void pop_at( std::uint8_t pos )
    {
        assert( !empty() );

        unlink_node( pos );
        insert_node( pos, free_head_pos );
    }

    /**
     * @brief Get a range of the orders in this chunk.
     */
    [[nodiscard]] auto orders_range() const noexcept
    {
        const auto start_pos = links[ head_pos ].next;

        return ranges::view::generate(
                   [ this, i = start_pos ]() mutable
                   {
                       const auto pos = i;
                       i              = links[ i ].next;
                       return pos;
                   } )
               | ranges::views::take_while( []( auto i )
                                            { return i != head_pos; } )
               | ranges::views::transform(
                   [ this ]( auto i ) -> std::pair< order_id_t, order_qty_t >
                   { return std::make_pair( id[ i ], qty[ i ] ); } );
    }

    /**
     * @brief Get a reverse range of the order in this chunk.
     */
    [[nodiscard]] auto orders_range_reverse() const noexcept
    {
        const auto start_pos = links[ head_pos ].prev;

        return ranges::view::generate(
                   [ this, i = start_pos ]() mutable
                   {
                       const auto pos = i;
                       i              = links[ i ].prev;
                       return pos;
                   } )
               | ranges::views::take_while( []( auto i )
                                            { return i != head_pos; } )
               | ranges::views::transform(
                   [ this ]( auto i ) -> std::pair< order_id_t, order_qty_t >
                   { return std::make_pair( id[ i ], qty[ i ] ); } );
    }
};

static_assert( 1 == sizeof( soa_chunk_node_t::links_t ) );

}  // namespace details

//
// chunked_soa_price_level_t
//

/**
 * @brief A storage of the orders on a given level using SOA to store data.
 */
template < List_Traits_Concept List_Traits >
class chunked_soa_price_level_t
{
public:
    using soa_chunks_container_t =
        typename List_Traits::list_t< details::soa_chunk_node_t >;

    explicit chunked_soa_price_level_t() = default;

    // We don't want copy this object.
    chunked_soa_price_level_t( const chunked_soa_price_level_t & ) = delete;
    chunked_soa_price_level_t & operator=( const chunked_soa_price_level_t & ) =
        delete;

    explicit chunked_soa_price_level_t( order_price_t p )
        : m_price{ p }
    {
    }

    friend inline void swap( chunked_soa_price_level_t & lvl1,
                             chunked_soa_price_level_t & lvl2 ) noexcept
    {
        using std::swap;
        swap( lvl1.m_price, lvl2.m_price );
        swap( lvl1.m_soa_chunks, lvl2.m_soa_chunks );
        swap( lvl1.m_orders_qty, lvl2.m_orders_qty );
        swap( lvl1.m_orders_count, lvl2.m_orders_count );
    }

    explicit chunked_soa_price_level_t( chunked_soa_price_level_t && lvl )
        : m_price{ lvl.m_price }
        , m_soa_chunks{ std::move( lvl.m_soa_chunks ) }
        , m_orders_qty{ lvl.m_orders_qty }
        , m_orders_count{ lvl.m_orders_count }
    {
        lvl.m_orders_qty   = order_qty_t{};
        lvl.m_orders_count = 0;
    }

    chunked_soa_price_level_t & operator=( chunked_soa_price_level_t && lvl )
    {
        chunked_soa_price_level_t tmp{ std::move( lvl ) };
        swap( *this, tmp );

        return *this;
    }

    using reference_t = chunked_soa_price_level_order_reference_t<
        typename soa_chunks_container_t::iterator >;

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

        auto it = [ this ]
        {
            if( m_soa_chunks.empty() || m_soa_chunks.back().full() )
            {
                m_soa_chunks.emplace_back( details::soa_chunk_node_t{} );
            }

            auto it = m_soa_chunks.end();
            return --it;
        }();

        m_orders_qty += order.qty;
        ++m_orders_count;
        return reference_t{ m_price, it, it->push_back( order.id, order.qty ) };
    }

    /**
     * @brief Remove an order identified by reference.
     */
    void delete_order( const reference_t & ref )
    {
        assert( ref.price() == m_price );

        m_orders_qty -= ref.chunk_it()->qty[ ref.pos() ];
        --m_orders_count;

        ref.chunk_it()->pop_at( ref.pos() );

        if( ref.chunk_it()->empty() )
        {
            m_soa_chunks.erase( ref.chunk_it() );
        }
    }

    /**
     * @brief Reduce total qty on this level (execute,reduce or modify).
     */
    [[nodiscard]] reference_t reduce_qty( const reference_t & ref,
                                          order_qty_t qty ) noexcept
    {
        assert( ref.price() == m_price );
        assert( m_orders_qty > qty );
        assert( ref.chunk_it()->qty[ ref.pos() ] > qty );

        ref.chunk_it()->qty[ ref.pos() ] -= qty;
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
        return m_orders_count;
    }

    /**
     * @brief A total quantity in all orders.
     */
    [[nodiscard]] order_qty_t orders_qty() const noexcept { return m_orders_qty; }

    [[nodiscard]] order_t order_at( const reference_t & ref ) const noexcept
    {
        return ref.make_order();
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
        return m_soa_chunks
               | ranges::views::transform( []( const auto & chunk )
                                           { return chunk.orders_range(); } )
               | ranges::view::join
               | ranges::views::transform(
                   [ p = m_price ]( std::pair< order_id_t, order_qty_t > id_qty )
                   { return order_t{ id_qty.first, id_qty.second, p }; } );
    }

    /**
     * @brief Get a range of the order on this level in reverse.
     */
    [[nodiscard]] auto orders_range_reverse() const noexcept
    {
        return m_soa_chunks
               | ranges::views::transform(
                   []( const auto & chunk )
                   { return chunk.orders_range_reverse(); } )
               | ranges::view::join
               | ranges::views::transform(
                   [ p = m_price ]( std::pair< order_id_t, order_qty_t > id_qty )
                   { return order_t{ id_qty.first, id_qty.second, p }; } );
    }

    /**
     * @brief Get the first order on this level.
     *
     * @pre Level MUST NOT be empty.
     */
    [[nodiscard]] order_t first_order() const noexcept
    {
        assert( !empty() );
        const auto & chunk = m_soa_chunks.front();
        const auto pos = chunk.links[ details::soa_chunk_node_t::head_pos ].next;

        return order_t{ chunk.id[ pos ], chunk.qty[ pos ], m_price };
    }

private:
    /**
     * @brief Current level price.
     */
    order_price_t m_price{};

    /**
     * @brief Data container.
     */
    soa_chunks_container_t m_soa_chunks{};

    /**
     * @brief A total quantity in all orders.
     */
    order_qty_t m_orders_qty{};

    /**
     * @brief Number of orders at this level.
     */
    std::size_t m_orders_count{};
};

static_assert(
    Price_Level_Concept< chunked_soa_price_level_t< std_list_traits_t > > );
static_assert(
    Price_Level_Concept< chunked_soa_price_level_t< plf_list_traits_t > > );

//
// chunked_soa_price_levels_factory_t
//

template < typename Price_Level >
using chunked_soa_price_levels_factory_t =
    trivial_price_levels_factory_t< Price_Level >;

static_assert( Price_Levels_Factory_Concept< chunked_soa_price_levels_factory_t<
                   chunked_soa_price_level_t< std_list_traits_t > > > );
static_assert( Price_Levels_Factory_Concept< chunked_soa_price_levels_factory_t<
                   chunked_soa_price_level_t< plf_list_traits_t > > > );

}  // namespace jacobi::book
