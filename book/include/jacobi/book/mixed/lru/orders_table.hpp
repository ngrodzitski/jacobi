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
#include <range/v3/view/transform.hpp>

#include <boost/intrusive/set.hpp>

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
 * @note Implementation assumes we always have a double-linked list
 *       that contains all the items (indexes). Initially the list follows the
 *       natural order of 0...N which aligns with how indexes in items storage
 *       must be assigned with a initial fill of the cache.
 */
class lru_kick_list_t
{
public:
    using index_type_t = std::uint8_t;
    inline static constexpr std::size_t invalid_index =
        std::numeric_limits< index_type_t >::max();
    inline static constexpr std::size_t max_elements_count =
        std::numeric_limits< index_type_t >::max();

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
     *      which effectively store head and tail of the list).
     */
    explicit lru_kick_list_t( std::size_t size )
        : m_nodes_count{ static_cast< index_type_t >(
              std::clamp< std::size_t >( size, 4, max_elements_count ) ) }
        , m_nodes(
              std::make_unique_for_overwrite< node_t[] >( m_nodes_count + 1 ) )
    {
#if defined( __GNUC__ ) && !defined( __clang__ )
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
        // Make initial list
        // first must have node (n aka anchor) as its previous:
        m_nodes[ 0 ] = node_t{ .prev = m_nodes_count, .next = 1 };
        for( index_type_t i = 1; i < m_nodes_count; ++i )
        {
            m_nodes[ i ].prev = i - 1;
            m_nodes[ i ].next = i + 1;
        }
        // Node (n aka anchor) next (aka head) must be linked to first node.
        m_nodes[ m_nodes_count ] =
            node_t{ .prev = static_cast< index_type_t >( m_nodes_count - 1 ),
                    .next = 0 };
#if defined( __GNUC__ ) && !defined( __clang__ )
#    pragma GCC diagnostic pop
#endif
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
        const auto t = m_nodes[ pos ].prev;

        if( t == i ) return;

        // Let's eliminate i-th node and insert it to the end.
        {
            const auto node_i = m_nodes[ i ];

            // Make neighbors of eliminated node linked:
            m_nodes[ node_i.prev ].next = node_i.next;
            m_nodes[ node_i.next ].prev = node_i.prev;
        }

        // Here: we have i-th node disconnected from the list.
        //       What we need to do is to add i-th as new node
        //       before pos-node.

        // Set new linkage for i-th node.
        m_nodes[ i ] = node_t{ .prev = t, .next = pos };

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
     * and for normal case (node somewhere in the middle of the list).
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

    /**
     * @brief Constructor with explicit cache size.
     *
     * @note The maximum cache size is 255.
     *
     * @param  cache_size  Cache size, must be in `[4..255)`
     *
     * @return            [description]
     */
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
        explicit cached_levels_t( std::size_t size = 32 )
            : size{ std::clamp< std::size_t >( size, 4, 254 ) }
            , prices{ new order_price_t[ size ] }
            , levels{ new price_level_t *[ size ] }
            , iterators{ new typename levels_table_t::iterator[ size ] }
            , kick_list{ size }
        {
        }

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
#if defined( __clang__ )
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wbitwise-instead-of-logical"
#endif  // defined(__clang__)
                if( ( prices[ i ] == p ) & ( nullptr != levels[ i ] ) )
                {
                    lvl   = levels[ i ];
                    it    = iterators[ i ];
                    index = i;
                    break;
                }
            }
#if defined( __clang__ )
#    pragma clang diagnostic pop
#endif  // defined(__clang__)

            return std::make_tuple( lvl, it, index );
        }

        void add_entry( typename levels_table_t::iterator it )
        {
            const auto i   = kick_list.lru_index();
            prices[ i ]    = it->first;
            levels[ i ]    = &( it->second );
            iterators[ i ] = it;
            kick_list.use_index( i );
        }

        void remove_entry( std::size_t index )
        {
            kick_list.free_index( index );
            levels[ index ] = nullptr;
        }
    };

    cached_levels_t m_cache;
};

namespace v2
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
        return ( *this )( n1.plvl.price(), n2.plvl.price() );
    }
    [[nodiscard]] bool operator()( const Price_Level_Node & node,
                                   order_price_t p ) const noexcept
    {
        return ( *this )( node.plvl.price(), p );
    }
    [[nodiscard]] bool operator()( order_price_t p,
                                   const Price_Level_Node & node ) const noexcept
    {
        return ( *this )( p, node.plvl.price() );
    }
};

using hook_type_t = boost::intrusive::set_member_hook<>;

//
// price_level_node_t
//

/**
 * @brief Intrusive node that contains meaningful price level.
 */
template < Price_Level_Concept Price_Level >
struct price_level_node_t
{
    price_level_node_t( Price_Level && plvl_obj )
        : plvl{ std::move( plvl_obj ) }
    {
    }

    hook_type_t hook;
    Price_Level plvl;
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
 *
 * Implements the following strategy:
 *     * Use boost::intrusive::set as storage for levels.
 *     * Cached hot-nodes are stored in linear storage.
 *     * Intrusiveness makes it possible to embedding cached nodes
 *       into set for sorted access.
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

    explicit orders_table_t( book_private_data_t & data )
        : base_type_t{ data }
        , m_cache{ this->m_book_private_data.price_levels_factory }
    {
    }

    explicit orders_table_t( std::size_t cache_size, book_private_data_t & data )
        : base_type_t{ data }
        , m_cache{ this->m_book_private_data.price_levels_factory, cache_size }
    {
    }

    ~orders_table_t()
    {
        m_price_levels.clear_and_dispose(
            [ this ]( auto * node ) noexcept
            {
                if( !m_cache.is_cached_node( node ) ) delete node;
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
        return m_price_levels.begin()->plvl.price();
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
        return m_price_levels.begin()->plvl.orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        return m_price_levels
               | ranges::views::transform(
                   []( const auto & node ) -> const price_level_t &
                   { return node.plvl; } );
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
        return m_price_levels.begin()->plvl.first_order();
    }

private:
    using price_level_node_t = details::price_level_node_t< price_level_t >;
    using cmp_t = details::set_cmp_t< price_level_node_t, Side_Indicator >;

    using levels_table_t = boost::intrusive::set<
        price_level_node_t,
        boost::intrusive::member_hook< price_level_node_t,
                                       details::hook_type_t,
                                       &price_level_node_t::hook >,
        boost::intrusive::compare< cmp_t > >;

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, std::size_t > level_at(
        order_price_t price )
    {
        auto [ lvl, index ] = m_cache.find_entry( price );

        if( !lvl ) [[unlikely]]
        {
            // Position to where relocate (or create a new) a node
            // with requested price level:
            index = m_cache.kick_list.lru_index();

            if( m_cache.levels[ index ].hook.is_linked() )
            {
                // We must pop the meaningful node from cache
                // to occupy a new position within cached array of nodes.
                auto * new_node = m_cache.allocate_node(
                    std::move( m_cache.levels[ index ].plvl ) );

                auto it = m_price_levels.iterator_to( m_cache.levels[ index ] );
                m_price_levels.replace_node( it, *new_node );

                assert( new_node->hook.is_linked() );
            }

            assert( !m_cache.levels[ index ].hook.is_linked() );

            auto it = m_price_levels.find( price, cmp_t{} );
            if( m_price_levels.end() == it )
            {
                m_cache.levels[ index ].plvl =
                    this->m_book_private_data.price_levels_factory
                        .make_price_level( price );

                m_price_levels.insert( m_cache.levels[ index ] );
            }
            else
            {
                m_cache.levels[ index ].plvl  = std::move( it->plvl );
                price_level_node_t * old_node = &( *it );
                assert( !m_cache.is_cached_node( old_node ) );
                m_price_levels.replace_node( it, m_cache.levels[ index ] );

                assert( !old_node->hook.is_linked() );
                m_cache.deallocate_node( old_node );
            }
            m_cache.prices[ index ] = price;

            assert( m_cache.levels[ index ].hook.is_linked() );

            lvl = &( m_cache.levels[ index ].plvl );
        }

        assert( lvl );

        m_cache.kick_list.use_index( index );

        return std::make_pair( lvl, index );
    }

    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        assert( !this->empty() );
        return m_price_levels.begin()->plvl;
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( std::size_t index )
    {
        assert( m_cache.levels[ index ].hook.is_linked() );

        auto it = m_price_levels.iterator_to( m_cache.levels[ index ] );
        m_price_levels.erase( it );

        assert( !m_cache.levels[ index ].hook.is_linked() );

        m_cache.kick_list.free_index( index );
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
        template < typename Price_Level_Factory >
        explicit cached_levels_t( Price_Level_Factory & price_levels_factory,
                                  std::size_t cap = 32 )
            : kick_list{ cap }
        {
            prices.resize( cap );
            levels.reserve( cap );

            while( levels.size() != cap )
            {
                levels.emplace_back(
                    price_levels_factory.make_price_level( order_price_t{ 0 } ) );
            }
        }

        std::vector< order_price_t > prices;
        std::vector< price_level_node_t > levels;

        [[nodiscard]] price_level_node_t * allocate_node( price_level_t && plvl )
        {
            if( nodes_pool.empty() )
            {
                return new price_level_node_t{ std::move( plvl ) };
            }
            price_level_node_t * node = nodes_pool.back().release();
            nodes_pool.pop_back();
            node->plvl = std::move( plvl );
            return node;
        }

        void deallocate_node( price_level_node_t * node )
        {
            nodes_pool.emplace_back( node );
        }

        std::vector< std::unique_ptr< price_level_node_t > > nodes_pool;

        using lru_kick_list_t =
            ::jacobi::book::mixed::lru::details::lru_kick_list_t;
        lru_kick_list_t kick_list;

        [[nodiscard]] std::tuple< price_level_t *, std::size_t > find_entry(
            order_price_t p ) noexcept
        {
            price_level_t * lvl = nullptr;
            auto index          = lru_kick_list_t::invalid_index;

            for( lru_kick_list_t::index_type_t i = 0; i < levels.size(); ++i )
            {
                if( ( prices[ i ] == p ) && ( levels[ i ].hook.is_linked() ) )
                {
                    lvl   = &levels[ i ].plvl;
                    index = i;
                    break;
                }
            }

            return std::make_tuple( lvl, index );
        }

        [[nodiscard]] bool is_cached_node(
            const price_level_node_t * p ) const noexcept
        {
            return levels.data() <= p && p < ( levels.data() + levels.size() );
        }
    };

    cached_levels_t m_cache;
};

}  // namespace v2

}  // namespace jacobi::book::mixed::lru
