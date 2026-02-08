// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table implementation based on
 *       ordered-map (price=>price_level) implementation
 *       using lru cache.
 */

#pragma once

#include <type_traits>
#include <map>

#include <range/v3/view/map.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level.hpp>
#include <jacobi/book/orders_table_crtp_base.hpp>

namespace jacobi::book::mixed::lru
{

namespace details
{

//
//  lru_kick_list_t
//

/**
 * @brief A list to track the LRU item index.
 *
 * The purpose is to keep track of "usages" of all indexes in a storage.
 * Maintains a linked-list of used indexes (head - least recently used,
 * tail - most recently used).
 *
 * Usage suggests that a USER tells what index was used VERY RECENTLY
 * and the routine tells which index was used LEAST RECENTLY.
 * @code
 * +--------------+                    +--------------+
 * |              |  #i was used ===>  |    LRU       |
 * |   USER       |                    |    kick      |
 * |              |  <===  #i is lru   |    list      |
 * +--------------+                    +--------------+
 * @endcode
 *
 * Main API function work in constant time `O(1)` and use no conditional
 * statements. The later makes implementation a bit sophisticated.
 *
 * @note Implementation uses indexes to refer to an item subjected to caching.
 *
 * @note The routine is a part of private implementation of
 *       @c jacobi::book::mixed::lru::orders_table_t
 *       and should not be used independently.
 *       The functionality is extracted into independent class
 *       to make it available for unit-tests and keep things SOLID.
 *
 * @note Implementation assumes we alway have a double-linked list
 *       that contains all the items (indexes). Initialy the list follows the
 *       natural order of 0...N which alignes with how indexes in items storage
 *       must be assigned with a initial fill of the cache.
 */
class lru_kick_list_t
{
public:
    using index_type_t = std::uint8_t;
    inline static constexpr std::size_t invalid_index =
        std::numeric_limits< index_type_t >::max();
    inline static constexpr std::size_t max_elements_count =
        std::numeric_limits< index_type_t >::max() - 1;

    /**
     * @brief Construct LRU list.
     *
     * Initial list is initialized so that it
     * connects indexes : `0 ... n`
     * and the first LRU element would be `0` followed by `1`,
     * followed by `2`, etc.
     *
     * @pre The size must be less then a max number representable with index_type_t
     *      (note: one elements (which is last) is reserved as a anchor node
     *      which effectively store head and tal of the list).
     */
    explicit lru_kick_list_t( std::size_t size )
        : m_nodes_count{ []( auto s )
                         {
                             assert( s <= max_elements_count );
                             return static_cast< index_type_t >( s );
                         }( size ) }
        , m_nodes( new node_t[ m_nodes_count + 1 ] )
    {
        // Make initial list
        // first must have node (n aka anchor) as its previous:
        m_nodes[ 0 ] = node_t{ .prev = m_nodes_count, .next = 1 };
        for( index_type_t i = 1; i < m_nodes_count; ++i )
        {
            m_nodes[ i ] = node_t{ .prev = static_cast< index_type_t >( i - 1 ),
                                   .next = static_cast< index_type_t >( i + 1 ) };
        }
        // Node (n aka anchor) next (aka head) must be linked to first node.
        m_nodes[ m_nodes_count ] =
            node_t{ .prev = static_cast< index_type_t >( m_nodes_count - 1 ),
                    .next = 0 };

        //clang-format off
        // The initial list looks like this:
        //                                                           **ANCHOR**
        //       |   0  |    |   1  |    |   2  |   ...   |  N-1 |    |  N   |
        //       +------+    +------+    +------+   ...   +------+    +------+
        // +-----| prev |<---| prev |<---| prev |   ...<--| prev |<---| tail |<---+
        // | +-->| next |--->| next |--->| next |-->...   | next |--->| head |--+ |
        // | |   +------+    +------+    +------+   ...   +------+    +------+  | |
        // | |                                                                  | |
        // | +------------------------------------------------------------------+ |
        // |                                                                      |
        // +--  ------------------------------------------------------------------+
        // clang-format on
    }

    // We have not intention to copy or move this class.
    lru_kick_list_t( const lru_kick_list_t & )             = delete;
    lru_kick_list_t( lru_kick_list_t && )                  = delete;
    lru_kick_list_t & operator=( const lru_kick_list_t & ) = delete;
    lru_kick_list_t & operator=( lru_kick_list_t && )      = delete;

    /**
     * @name Main API for handling LRU list.
     */
    /// @{
    /**
     * @brief Mark an index as recently used one.
     */
    void use_index( std::size_t index )
    {
        assert( index < std::numeric_limits< index_type_t >::max() );
        assert( m_nodes_count > index );

        const auto i = static_cast< index_type_t >( index );

        insert( i, anchor() );
    }

    /**
     * @brief Mark an index as free (move it to the head of the kick list).
     */
    void free_index( std::size_t index )
    {
        assert( index < std::numeric_limits< index_type_t >::max() );
        assert( m_nodes_count > index );

        const auto i = static_cast< index_type_t >( index );

        assert( i != head() );

        insert( i, head() );
    }

    /**
     * @brief Get the value of the least recently used index.
     */
    [[nodiscard]] int lru_index() const noexcept
    {
        // The head of the list is LRU element.
        return head();
    }
    /// @}

    /**
     * @brief Debug function to use in unit-test.
     *
     * @note It is a private class the purpose of which
     *       is to make lru routine available for testing.
     */
    [[nodiscard]] std::vector< int > make_lru_dump() const
    {
        std::vector< int > res;

        for( int i = head(); i != anchor() && res.size() < max_elements_count;
             i     = m_nodes[ i ].next )
        {
            res.push_back( i );
        }

        return res;
    }

private:
    void insert( index_type_t i, index_type_t pos )
    {
        // Let's eliminate i-th node and insert it to the end.
        {
            const auto prev = m_nodes[ i ].prev;
            const auto next = m_nodes[ i ].next;

            // Make neighbours of eleminated node linked:
            m_nodes[ prev ].next = next;
            m_nodes[ next ].prev = prev;
        }

        // Here: we have i-th node disconnected from the list.
        //       What we need to do is to add i-th as new node
        //       before pos-node.

        const auto t = m_nodes[ pos ].prev;

        // Set new linkage for i-th node.
        m_nodes[ i ].next = pos;
        m_nodes[ i ].prev = t;

        m_nodes[ pos ].prev = i;
        m_nodes[ t ].next   = i;
    }

    [[nodiscard]] index_type_t head() const noexcept
    {
        return m_nodes[ anchor() ].next;
    }
    [[nodiscard]] index_type_t tail() const noexcept
    {
        return m_nodes[ anchor() ].prev;
    }

    [[nodiscard]] index_type_t anchor() const noexcept
    {
        return static_cast< index_type_t >( m_nodes_count );
    }

    //
    // node_t
    //

    /**
     * A custom node that refers to naighbours by index.
     */
    struct node_t
    {
        index_type_t prev{};
        index_type_t next{};
    };

    /**
     * @brief A storage of nodes.
     *
     * Contains one extra node, the purpose for it is to be an anchor
     * that sores head and tail of the list.
     *
     * @code
     * |   0  |   1  |   2  |     |  N   |
     * +------+------+------+-----+------+--
     * | prev | prev | prev | ... | tail |
     * | next | next | next | ... | head |
     * +------+------+------+-----+------+--
     * @endcode
     *
     * This enables us to write a unified algo in use_index() implementation
     * for edge cases (the node assigned to that index is a tail or a head)
     * and for noram case (node somewhere in the middle of the list).
     */
    const index_type_t m_nodes_count;
    const std::unique_ptr< node_t[] > m_nodes;
};

}  // namespace details

//
// orders_table_t
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
template < Book_Impl_Data_Concept Book_Impl_Data, trade_side Side_Indicator >
class orders_table_t
    : public orders_table_crtp_base_t<
          orders_table_t< Book_Impl_Data, Side_Indicator >,
          Book_Impl_Data,
          Side_Indicator >
{
public:
    // Base class must have access to private functions.
    friend class orders_table_crtp_base_t< orders_table_t,
                                           Book_Impl_Data,
                                           Side_Indicator >;

    using base_type_t =
        orders_table_crtp_base_t< orders_table_t, Book_Impl_Data, Side_Indicator >;

    using typename base_type_t::price_level_t;
    using typename base_type_t::book_private_data_t;

    explicit orders_table_t( std::size_t cache_size, book_private_data_t & data )
        : base_type_t{ data }
        , m_cache{ cache_size }
    {
    }

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
    using levels_table_t =
        std::map< order_price_t,
                  price_level_t,
                  typename order_price_operations_t< Side_Indicator >::cmp_t >;

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, typename levels_table_t::iterator >
    level_at( order_price_t price )
    {
        if( auto [ lvl, it, index ] = m_cache.find_entry( price ); lvl ) [[likely]]
        {
            m_cache.kick_list.use_index( index );
            return std::make_pair( lvl, it );
        }
        else
        {
            it = m_price_levels.lower_bound( price );

            if( it == end( m_price_levels ) || it->first != price )
            {
                it = m_price_levels.insert(
                    it,
                    std::make_pair( price,
                                    this->m_book_private_data.price_levels_factory
                                        .make_price_level( price ) ) );
            }

            m_cache.add_entry( it );

            return std::make_pair( &( it->second ), it );
        }
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
        const auto price = lvl_it->first;
        if( auto [ lvl, it, index ] = m_cache.find_entry( price ); lvl ) [[likely]]
        {
            m_cache.remove_entry( index );
            assert( it == lvl_it );
        }
        m_price_levels.erase( lvl_it );
    }

    /**
     * @brief A storage for price levels.
     *
     * Each levels contain a queue of orders at a given price.
     */
    levels_table_t m_price_levels;

    //
    // cached_levels_t
    //
    struct cached_levels_t
    {
        explicit cached_levels_t( std::size_t cap = 32 )
            : capacity{ cap }
            , prices{ new order_price_t[ capacity ] }
            , levels{ new price_level_t *[ capacity ] }
            , iterators{ new typename levels_table_t::iterator[ capacity ] }
            , kick_list{ cap }
        {
        }

        const std::size_t capacity{};
        std::size_t size{};
        std::unique_ptr< order_price_t[] > prices;
        std::unique_ptr< price_level_t *[] > levels;
        std::unique_ptr< typename levels_table_t::iterator[] > iterators;

        details::lru_kick_list_t kick_list;

        [[nodiscard]] std::tuple< price_level_t *,
                                  typename levels_table_t::iterator,
                                  std::size_t >
        find_entry( order_price_t p ) noexcept
        {
            price_level_t * lvl = nullptr;
            typename levels_table_t::iterator it;
            auto index = details::lru_kick_list_t::invalid_index;

            for( details::lru_kick_list_t::index_type_t i = 0; i < size; ++i )
            {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbitwise-instead-of-logical"
                if( ( prices[ i ] == p ) & ( nullptr != levels[ i ] ) )
                {
                    lvl   = levels[ i ];
                    it    = iterators[ i ];
                    index = i;
                }
            }
#pragma clang diagnostic pop

            return std::make_tuple( lvl, it, index );
        }

        void add_entry( typename levels_table_t::iterator it )
        {
            const auto i   = kick_list.lru_index();
            prices[ i ]    = it->first;
            levels[ i ]    = &( it->second );
            iterators[ i ] = it;
        }

        void remove_entry( std::size_t index )
        {
            kick_list.free_index( index );
            levels[ index ] = nullptr;
        }
    };

    cached_levels_t m_cache;
};

}  // namespace jacobi::book::mixed::lru
