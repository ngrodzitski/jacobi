// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Price level routines using segregated SOA approach.
 */

#pragma once

#include <bit>
#include <cstring>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <optional>
#include <stdexcept>

#include <range/v3/view/generate.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <jacobi/book/soa_price_level.hpp>

namespace jacobi::book
{

namespace details
{

using unified_soa_position_t = soa_price_level_order_node_pos_t< std::uint32_t >;

//
// soa_buf_read_u32()
//

/**
 * @brief Read u32 from raw bytes.
 *
 * @pre Address `d` must be properly aligned (4).
 */
[[nodiscard]] inline std::uint32_t soa_buf_read_u32( const std::byte * d ) noexcept
{
    std::uint32_t value;
    std::memcpy( &value, d, sizeof( value ) );
    return value;
}

//
// soa_buf_write_u32()
//

/**
 * @brief Writes u32 to raw bytes.
 *
 * @pre Address `d` must be properly aligned (4).
 */
inline void soa_buf_write_u32( std::uint32_t value, std::byte * d ) noexcept
{
    std::memcpy( d, &value, sizeof( value ) );
}

//
// soa_buf_capacity_offset
//

/**
 * @brief The location of the capacity parameter in a price level soa data buffer.
 */
constexpr std::size_t soa_buf_capacity_offset = sizeof( std::uint32_t );

//
// soa_buf_alignment
//

constexpr std::size_t soa_buf_alignment = 8;

using raw_buffer_t = std::span< std::byte >;

//
// Soa_Index_Type
//

template < typename T >
concept Soa_Index_Type =
    std::same_as< T, std::uint8_t > || std::same_as< T, std::uint16_t >
    || std::same_as< T, std::uint32_t >;

//
// extra_links_for_anchors
//

constexpr std::uint8_t extra_links_for_anchors = 2;

//
// soa_price_level_data_t
//

/**
 * @brief Price level orders data buffer accessor object.
 *
 * Operates over a raw buffer. For performance it expects a proper alignments.
 * It is a part of an internal implementation and is not meant for using
 * otherwise.
 *
 * The attached buffer must have the following data layout:
 * @code
 * [3:0] Size
 * [7:4] Capacity (CAP)
 * [I-1:8] Order ids;                  I = 8 + CAP*8
 * [Q-1:I] Order qtys;                 Q = I + CAP*4
 * [L-1:Q] Node links;                 L = Q + (CAP+2) * sizeof(Index_Type)
 * @endcode
 *
 * Lifetime concerns are addressed by C++20's P0593 rules.
 * We have only trivial types in buffer.
 */
template < Soa_Index_Type Index_Type >
class soa_price_level_data_access_t
{
    using short_index_t = Index_Type;
    constexpr inline static short_index_t links_head_pos{ 0 };
    constexpr inline static short_index_t links_free_head_pos{ 1 };

    using raw_order_id_t  = order_id_t::value_type;
    using raw_order_qty_t = order_qty_t::value_type;

    //
    // raw_soa_links_t
    //

    /**
     * @brief Short SOA data referencing to adjacent nodes.
     *
     */
    struct raw_soa_links_t
    {
        short_index_t prev;
        short_index_t next;
    };

    /**
     * @name Offset calculations data in the buffer.
     */
    /// @{
    [[nodiscard]] constexpr static std::size_t ids_offset() noexcept { return 8; }

    [[nodiscard]] constexpr static std::size_t qtys_offset(
        std::size_t cap ) noexcept
    {
        return ids_offset() + cap * sizeof( raw_order_id_t );
    }

    [[nodiscard]] constexpr static std::size_t links_offset(
        std::size_t cap ) noexcept
    {
        return qtys_offset( cap ) + cap * sizeof( raw_order_qty_t );
    }

    // ==================================================================
    // We expect a certain alignment for the data:
    static_assert( ids_offset() % alignof( raw_order_id_t ) == 0 );

    static_assert( qtys_offset( 1 ) % alignof( raw_order_qty_t ) == 0 );
    static_assert( qtys_offset( 2 ) % alignof( raw_order_qty_t ) == 0 );
    static_assert( qtys_offset( 3 ) % alignof( raw_order_qty_t ) == 0 );
    static_assert( qtys_offset( 4 ) % alignof( raw_order_qty_t ) == 0 );

    static_assert( links_offset( 1 ) % alignof( raw_soa_links_t ) == 0 );
    static_assert( links_offset( 2 ) % alignof( raw_soa_links_t ) == 0 );
    static_assert( links_offset( 3 ) % alignof( raw_soa_links_t ) == 0 );
    static_assert( links_offset( 4 ) % alignof( raw_soa_links_t ) == 0 );
    // ==================================================================
    /// @}

public:
    constexpr inline static std::size_t max_capacity =
        std::numeric_limits< short_index_t >::max() - 1;

    [[nodiscard]] constexpr static std::size_t required_size(
        std::uint32_t cap ) noexcept
    {
        return links_offset( cap )
               + ( cap + extra_links_for_anchors ) * sizeof( raw_soa_links_t );
    }

    static std::uint32_t validate_buffer(
        std::span< const std::byte > buf,
        std::optional< short_index_t > expected_capacity = std::nullopt )
    {
        if( 0
            != reinterpret_cast< std::uintptr_t >( buf.data() )
                   % soa_buf_alignment )
        {
            throw std::runtime_error{ "buffer misaligned" };
        }

        if( buf.size() < 2 * sizeof( std::uint32_t ) )
        {
            throw std::runtime_error{ fmt::format( "buffer size ({}) is too small",
                                                   buf.size() ) };
        }

        const auto cap = soa_buf_read_u32( buf.data() + soa_buf_capacity_offset );

        if( expected_capacity && expected_capacity.value() != cap )
        {
            throw std::runtime_error{ fmt::format(
                "capacity defined in buffer ({}) "
                "doesn't match expected capacity ({})",
                cap,
                expected_capacity.value() ) };
        }

        if( cap < 1 )
        {
            throw std::runtime_error{ fmt::format(
                "capacity must be at least 1, given capacity is {}", cap ) };
        }

        if( const auto max_cap = std::numeric_limits< Index_Type >::max()
                                 - extra_links_for_anchors + 1;
            cap > max_cap )
        {
            throw std::runtime_error{ fmt::format(
                "max capacity allowed for a given link type is {}, "
                "while {} requested",
                max_cap,
                cap ) };
        }

        if( const auto min_buf_size = required_size( cap );
            buf.size() < min_buf_size )
        {
            throw std::runtime_error{ fmt::format(
                "buffer size ({}) is lesser than "
                "{} which is required for given capacity ({})",
                buf.size(),
                min_buf_size,
                cap ) };
        }

        const auto sz = soa_buf_read_u32( buf.data() );
        if( sz > cap )
        {
            throw std::runtime_error{ fmt::format(
                "invalid size ({}) for a given capacity {}", sz, cap ) };
        }

        return cap;
    }

    /**
     * @brief Initializes layout of the data in buffer.
     *
     * @pre Capacity must not exceed max_capacity value.
     * @pre The buffer must have a proper size for a given capacity.
     */
    static void initialize_layout( raw_buffer_t raw_buf,
                                   std::uint32_t cap ) noexcept
    {
        assert( cap <= max_capacity );
        assert( raw_buf.size() >= required_size( cap ) );

        // Initial parameters:
        soa_buf_write_u32( 0, raw_buf.data() );
        soa_buf_write_u32( cap, raw_buf.data() + soa_buf_capacity_offset );

        auto * links = reinterpret_cast< raw_soa_links_t * >(
            raw_buf.data() + links_offset( cap ) );

        links[ links_head_pos ] = { links_head_pos, links_head_pos };

        // Notes regarding reusable free nodes list.
        //
        // Free reusable nodes list is a forward list.
        // We only use .next field to build it.
        //
        // Initially the free-list is empty.
        // We do not init all links to establish free-list so that
        // it contains all nodes initially.
        // What we do instead is assume all nodes starting from
        // offset `extra_links_for_anchors` to be available for being used.
        // Once the node is used and later released (the order was removed)
        // it goes to free-list and could be reused from there.
        //
        // To keep track of virgin nodes we use
        // free-head-link .prev field (it is not used because free-list
        // is a forward list). And it is used to supply a usable node
        // if the free-head-link.next is not pointing to a reusable node
        // (meaning .next points back to free-head-link).
        links[ links_free_head_pos ] = { extra_links_for_anchors,
                                         links_free_head_pos };
    }

    /**
     * @brief Initializes layout of the data in buffer
     *        And populate it with data from another already existing data.
     *
     * The purpose is to clone data from a full buffer to a larger one
     * and preserving old positions of the data in new buffer.
     *
     * @note In normal case src_date is expected to be full.
     *
     * @pre Capacity must not exceed max_capacity value.
     * @pre The buffer must have a proper size for a given capacity.
     * @pre Capacity for this buffer must be greater then capacity in scr_data.
     */
    template < Soa_Index_Type Other_Index_Type >
    static void initialize_layout(
        raw_buffer_t raw_buf,
        std::uint32_t cap,
        soa_price_level_data_access_t< Other_Index_Type > src_data ) noexcept
    {
        assert( sizeof( Other_Index_Type ) <= sizeof( short_index_t ) );
        assert( cap <= max_capacity );
        assert( raw_buf.size() >= required_size( cap ) );

        const auto old_cap = src_data.capacity();
        assert( old_cap <= cap );

        // Initial parameters:
        soa_buf_write_u32( src_data.size(), raw_buf.data() );
        soa_buf_write_u32( cap, raw_buf.data() + soa_buf_capacity_offset );

        auto * new_links = reinterpret_cast< raw_soa_links_t * >(
            raw_buf.data() + links_offset( cap ) );

        const auto * old_links = src_data.links();

        if constexpr( std::is_same_v< short_index_t, Other_Index_Type > )
        {
            // Same size of links.
            std::memcpy( new_links,
                         old_links,
                         ( old_cap + extra_links_for_anchors )
                             * sizeof( raw_soa_links_t ) );
        }
        else
        {
            // Hard case: size of links are different.
            // We can treat raw_soa_links_t as two uintX values (no padding
            // involved), an array of N structs is physically identical to a flat
            // array of 2*N uintX.
            const std::size_t num_elements =
                ( old_cap + extra_links_for_anchors ) * 2;

            // Use __restrict to promise the compiler that source and destination
            // buffers do not overlap, to promote auto-vectorization.
            const auto * __restrict src_arr =
                reinterpret_cast< const Other_Index_Type * >( old_links );
            auto * __restrict dst_arr =
                reinterpret_cast< short_index_t * >( new_links );

#if defined( __clang__ )
#    pragma clang loop vectorize( enable ) interleave( enable )
#elif defined( __GNUC__ )
#    pragma GCC ivdep
#elif defined( _MSC_VER )
#    pragma loop( ivdep )
#endif
            for( std::size_t i = 0; i < num_elements; ++i )
            {
                dst_arr[ i ] = static_cast< short_index_t >( src_arr[ i ] );
            }

            // Note: After copying links we are perfectly fine with
            //       value in `new_links[ links_free_head_pos ].prev`
            //       which is a next virgin node cursor.
            //
            //       But there is an edge case when we upgrade the type of
            //       a short_index_t. That happens when we  expanding buffer
            //       to a capacity larger then src_data type can handle:
            //        e.g. u8-links to u16-links or u16 to u32.
            //       In that case virgin cursor might be 0
            //       because all virgin nodes were used and cursor value overflow
            //       taking a value zero.

            if( 0 == new_links[ links_free_head_pos ].prev ) [[likely]]
            {
                // Fix virgin cursor value:
                new_links[ links_free_head_pos ].prev =
                    old_cap + extra_links_for_anchors;
            }
        }

        // So we only need to copy orders data now:
        const auto * ids  = src_data.ids();
        const auto * qtys = src_data.qtys();

        auto * new_ids =
            reinterpret_cast< raw_order_id_t * >( raw_buf.data() + ids_offset() );
        auto * new_qtys = reinterpret_cast< raw_order_qty_t * >(
            raw_buf.data() + qtys_offset( cap ) );

        std::memcpy( new_ids, ids, old_cap * sizeof( raw_order_id_t ) );
        std::memcpy( new_qtys, qtys, old_cap * sizeof( raw_order_qty_t ) );
    }

    /**
     * @brief Create accessor object for a given buffer.
     *
     * @pre Buffer pointed by raw_data must be aligned to 8.
     * @pre Buffer size must at least capable of storing size and capacity
     *      components which means minimum size is 8.
     */
    explicit soa_price_level_data_access_t( raw_buffer_t raw_data ) noexcept
        : m_raw_data{ raw_data }
        , m_cached_size{ soa_buf_read_u32( m_raw_data.data() ) }
        , m_cached_capacity{ soa_buf_read_u32( m_raw_data.data()
                                               + soa_buf_capacity_offset ) }
    {
    }

    [[nodiscard]] std::uint32_t size() const noexcept { return m_cached_size; }

    void size( std::uint32_t s ) noexcept
    {
        assert( s <= m_cached_capacity );
        m_cached_size = s;
        soa_buf_write_u32( s, m_raw_data.data() );
    }

    [[nodiscard]] std::uint32_t capacity() const noexcept
    {
        return m_cached_capacity;
    }

    [[nodiscard]] bool empty() const noexcept { return 0 == size(); }

    [[nodiscard]] bool full() const noexcept
    {
        return size() == m_cached_capacity;
    }

    [[nodiscard]] raw_soa_links_t * links() noexcept
    {
        std::byte * ptr = m_raw_data.data() + links_offset( m_cached_capacity );
        assert( 0
                == reinterpret_cast< std::uintptr_t >( ptr )
                       % alignof( raw_soa_links_t ) );
        return reinterpret_cast< raw_soa_links_t * >(
            std::assume_aligned< alignof( raw_soa_links_t ) >( ptr ) );
    }

    [[nodiscard]] const raw_soa_links_t * links() const noexcept
    {
        return const_cast< soa_price_level_data_access_t * >( this )->links();
    }

    [[nodiscard]] raw_order_id_t * ids() noexcept
    {
        std::byte * ptr = m_raw_data.data() + ids_offset();

        assert( 0
                == reinterpret_cast< std::uintptr_t >( ptr )
                       % alignof( raw_order_id_t ) );

        return reinterpret_cast< raw_order_id_t * >(
            std::assume_aligned< alignof( raw_order_id_t ) >( ptr ) );
    }

    [[nodiscard]] const raw_order_id_t * ids() const noexcept
    {
        return const_cast< soa_price_level_data_access_t * >( this )->ids();
    }

    [[nodiscard]] raw_order_qty_t * qtys() noexcept
    {
        std::byte * ptr = m_raw_data.data() + qtys_offset( m_cached_capacity );

        assert( 0
                == reinterpret_cast< std::uintptr_t >( ptr )
                       % alignof( raw_order_qty_t ) );

        return reinterpret_cast< raw_order_qty_t * >(
            std::assume_aligned< alignof( raw_order_qty_t ) >( ptr ) );
    }

    [[nodiscard]] const raw_order_qty_t * qtys() const noexcept
    {
        return const_cast< soa_price_level_data_access_t * >( this )->qtys();
    }

    /**
     * @brief Remove order at a given position.
     *
     * @pre pos must be pointing to a location within capacity
     */
    void pop_at( unified_soa_position_t pos )
    {
        assert( !empty() );
        assert( pos.node_link_index() >= extra_links_for_anchors );
        assert( pos.node_link_index() < static_cast< std::size_t >(
                    extra_links_for_anchors + m_cached_capacity ) );

        auto * const links_arr = links();

        const auto i = static_cast< short_index_t >( pos.node_link_index() );
        unlink_node( links_arr, i );

        links_arr[ i ].next = links_arr[ links_free_head_pos ].next;
        links_arr[ links_free_head_pos ].next = i;

        size( size() - 1 );
    }

    /**
     * @brief Add an order.
     *
     * @pre Level must not be full.
     *
     * @return Link position of the allocated node.
     */
    [[nodiscard]] unified_soa_position_t push_back( order_id_t id_value,
                                                    order_qty_t qty_value )
    {
        assert( !full() );
        auto * const links_arr = links();
        auto free_link         = links_arr[ links_free_head_pos ];

        short_index_t i;
        if( free_link.next != links_free_head_pos )
        {
            // First supply from free list.
            i              = free_link.next;
            free_link.next = links_arr[ i ].next;
        }
        else
        {
            // Actually we have `assert( !full() )` at the entrance of the function
            // but we still put this assert here on case
            // `full()` logic is somehow compromised.
            assert( free_link.prev < m_cached_capacity + extra_links_for_anchors );

            // Here: means this level first time reaches that size
            // and we supply yet another virgin node from a pos that we hold in
            // free_link.prev.
            i = free_link.prev++;

            // When we use the last yet not used node
            // `free_link.prev` will be equal to
            // `m_cached_capacity + extra_links_for_anchors`
            // and we no longer must ever execute this branch.
        }

        links_arr[ links_free_head_pos ] = free_link;

        ids()[ i - extra_links_for_anchors ]  = type_safe::get( id_value );
        qtys()[ i - extra_links_for_anchors ] = type_safe::get( qty_value );

        insert_node( links_arr, i, links_head_pos );

        size( size() + 1 );

        return unified_soa_position_t{ i };
    }

    /**
     * @name Range support.
     */
    /// @{
    /**
     * @brief Universal soa iterator.
     */
    template < bool Is_Reverse, bool Is_Const >
    struct order_data_iterator_t
    {
        using iterator_concept  = std::bidirectional_iterator_tag;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = std::pair< order_id_t, order_qty_t >;
        using difference_type   = std::ptrdiff_t;
        using reference         = value_type;
        using pointer           = void;

        // Ensure the parent pointer respects the Is_Const flag
        using parent_type_t =
            std::conditional_t< Is_Const,
                                const soa_price_level_data_access_t,
                                soa_price_level_data_access_t >;

        parent_type_t * parent{};
        short_index_t current_link{};

        // Grant access to all other instantiations of this template.
        template < bool, bool >
        friend struct order_data_iterator_t;

        order_data_iterator_t() = default;
        order_data_iterator_t( parent_type_t * p, short_index_t link ) noexcept
            : parent{ p }
            , current_link{ link }
        {
        }

        /**
         * @brief Converting Constructor
         *
         * Privides: `Non-Const => Const` and `Forward <=> Reverse`.
         */
        template < bool Other_Reverse, bool Other_Const >
            requires( Is_Const
                      || !Other_Const )  // Only one way Non-Const => Const
        constexpr explicit(
            Is_Reverse != Other_Reverse )  // Must be explicit if change direction.
            order_data_iterator_t(
                const order_data_iterator_t< Other_Reverse, Other_Const > &
                    other ) noexcept
            : parent{ other.parent }
            , current_link{ other.current_link }
        {
            if constexpr( Is_Reverse != Other_Reverse )
            {
                // When converting between direct and reverse,
                // we must physically shift the pointer by one
                // to maintain [begin, end) interval symmetry.
                if constexpr( Is_Reverse )
                {
                    current_link = parent->links()[ current_link ].prev;
                }
                else
                {
                    current_link = parent->links()[ current_link ].next;
                }
            }
        }

        /**
         * @brief Extract the underlying forward iterator.
         */
        [[nodiscard]] constexpr order_data_iterator_t< false, Is_Const > base()
            const noexcept
            requires( Is_Reverse )  // Only available on reverse iterators.
        {
            return order_data_iterator_t< false, Is_Const >( *this );
        }

        [[nodiscard]] value_type operator*() const noexcept
        {
            const auto i = current_link - extra_links_for_anchors;
            return { order_id_t{ parent->ids()[ i ] },
                     order_qty_t{ parent->qtys()[ i ] } };
        }

        order_data_iterator_t & operator++() noexcept
        {
            if constexpr( Is_Reverse )
            {
                current_link = parent->links()[ current_link ].prev;
            }
            else
            {
                current_link = parent->links()[ current_link ].next;
            }
            return *this;
        }

        order_data_iterator_t operator++( int ) noexcept
        {
            order_data_iterator_t tmp = *this;
            ++( *this );
            return tmp;
        }

        order_data_iterator_t & operator--() noexcept
        {
            if constexpr( Is_Reverse )
            {
                current_link = parent->links()[ current_link ].next;
            }
            else
            {
                current_link = parent->links()[ current_link ].prev;
            }
            return *this;
        }

        order_data_iterator_t operator--( int ) noexcept
        {
            order_data_iterator_t tmp = *this;
            --( *this );
            return tmp;
        }

        [[nodiscard]] friend bool operator==(
            const order_data_iterator_t & a,
            const order_data_iterator_t & b ) noexcept
        {
            return ( a.parent == b.parent ) & ( a.current_link == b.current_link );
        }
    };

    //                                              Is_Reverse  Is_Const
    using iterator               = order_data_iterator_t< false, false >;
    using const_iterator         = order_data_iterator_t< false, true >;
    using reverse_iterator       = order_data_iterator_t< true, false >;
    using const_reverse_iterator = order_data_iterator_t< true, true >;

    static_assert( std::bidirectional_iterator< iterator > );
    static_assert( std::bidirectional_iterator< const_iterator > );
    static_assert( std::bidirectional_iterator< reverse_iterator > );
    static_assert( std::bidirectional_iterator< const_reverse_iterator > );

    [[nodiscard]] iterator begin() noexcept
    {
        return iterator{ this, links()[ links_head_pos ].next };
    }
    [[nodiscard]] iterator end() noexcept
    {
        return iterator{ this, links_head_pos };
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return const_iterator{ this, links()[ links_head_pos ].next };
    }
    [[nodiscard]] const_iterator end() const noexcept
    {
        return const_iterator{ this, links_head_pos };
    }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }

    [[nodiscard]] reverse_iterator rbegin() noexcept
    {
        return reverse_iterator{ this, links()[ links_head_pos ].prev };
    }
    [[nodiscard]] reverse_iterator rend() noexcept
    {
        return reverse_iterator{ this, links_head_pos };
    }

    [[nodiscard]] const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator{ this, links()[ links_head_pos ].prev };
    }
    [[nodiscard]] const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator{ this, links_head_pos };
    }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return rend(); }
    /// @}

private:
    static void unlink_node( raw_soa_links_t * links_arr,
                             short_index_t i ) noexcept
    {
        const auto before_link = links_arr[ i ];

        // Make neighbours of eliminated node linked:
        links_arr[ before_link.prev ].next = before_link.next;
        links_arr[ before_link.next ].prev = before_link.prev;
    }

    static void insert_node( raw_soa_links_t * links_arr,
                             short_index_t i,
                             short_index_t head ) noexcept
    {
        // Here: we have i-th node disconnected from the list.
        //       What we need to do is to add i-th as new node
        //       before head.

        const auto t = links_arr[ head ].prev;

        // Set new linkage for i-th node.
        links_arr[ i ] = { t, head };

        links_arr[ head ].prev = i;
        links_arr[ t ].next    = i;
    }

    raw_buffer_t m_raw_data;
    std::uint32_t m_cached_size;
    const std::uint32_t m_cached_capacity;
};

using soa_price_level_data_access_u8_t =
    soa_price_level_data_access_t< std::uint8_t >;
using soa_price_level_data_access_u16_t =
    soa_price_level_data_access_t< std::uint16_t >;
using soa_price_level_data_access_u32_t =
    soa_price_level_data_access_t< std::uint32_t >;

//
// soa_buffers_pool_t
//

/**
 * @brief Standard pool implementation for allocating chunks of memory
 *        for SOA price level 2 implementation.
 *
 * Provides soa-buffers for storing price level orders data.
 *
 * Asking to allocate a new buffer must only come when previous
 * buffer is exhausted and asked capacity must be +1 the previous capacity.
 */
class std_soa_buffers_pool_t
{
public:
    /**
     * @brief An array of reusable buffers' sizes.
     */
    inline static constexpr std::array< std::uint32_t, 7 > soa_bufs_capacities{
        // CAP     Block size (bytes)
        17,    //   250
        35,    //   502
        72,    //  1020
        145,   //  2042
        254,   //  3568
        511,   //  8192
        1023,  //  16386
    };
    /**
     * @brief Determine to which reusable bucket
     *        a given capacity request belongs to.
     *
     * @pre The requested capacity must be exactly equal
     *      to an existing capacity +1. And as such when a given price level
     *      is filled with orders the request come in this order:
     *      `1, 18, 36, 73, 146, 255, 512, 1024, ...`.
     *
     * @note It is fine when function returns 7 or even a bigger value.
     *       That is because on indexes in `[0, 6]` interval
     *       are handled specifically according to a value.
     *       Greater values of index are handled uniformly regardless
     *       of how much bigger it is (the key condition
     *       is `ix >= soa_bufs_capacities.size()` ).
     */
    [[nodiscard]] static std::uint32_t soa_buf_cap_index(
        std::uint32_t cap ) noexcept
    {
        assert( cap >= 1 );

        // Implementation is a hack,
        // which relies on the following:
        // the request for a new buffer comes when a current capacity
        // is exhausted and thus it request a buffer for `capacity + 1`.
        // E.g. first cap-17 buffer is used,
        // then it will ask capacity 18 which would result in cap-35 buffer.
        // Then the capacity 36 will be requested which would result in
        // cap-72 buffer and so on.
        //
        // So to have a simple operation to define what is the approapriate index
        // we do the following
        //
        // https://godbolt.org/z/re9vzEqvf
        //
        // We add 7 to handle case requested capacity is 1.

        const auto index = std::bit_width( cap + 7 ) - 4;

        assert( index >= soa_bufs_capacities.size() || index == 0
                || ( cap == ( soa_bufs_capacities[ index - 1 ] + 1 ) ) );

        return index;
    }

    inline static constexpr std::size_t max_pooled_buffer_size =
        soa_price_level_data_access_u16_t::required_size(
            soa_bufs_capacities[ 6 ] );

public:
    std_soa_buffers_pool_t( const std_soa_buffers_pool_t & )             = delete;
    std_soa_buffers_pool_t( std_soa_buffers_pool_t && )                  = delete;
    std_soa_buffers_pool_t & operator=( const std_soa_buffers_pool_t & ) = delete;
    std_soa_buffers_pool_t & operator=( std_soa_buffers_pool_t && )      = delete;

    explicit std_soa_buffers_pool_t( std::pmr::memory_resource * upstream )
        : m_mem{ std::pmr::pool_options{ .max_blocks_per_chunk = 1024,
                                         .largest_required_pool_block =
                                             max_pooled_buffer_size },
                 upstream }
    {
    }

    explicit std_soa_buffers_pool_t()
        : std_soa_buffers_pool_t{ std::pmr::get_default_resource() }
    {
    }

    /**
     * @brief Allocate a Price level buffer with at least a given capacity.
     *
     * @pre The requested capacity must be one of:
     *      `1, 18, 36, 73, 146, 254, 512, 1024`
     *      or a value greater than 1024.
     */
    [[nodiscard]] raw_buffer_t allocate_buffer( std::uint32_t capacity )
    {
        assert( capacity >= 1 );
        raw_buffer_t res_buf{};
        const auto hardcoded_capacity_index = soa_buf_cap_index( capacity );

        // Nothing to reuse so we need to ask m_mem for a new chunk of memory.
        if( hardcoded_capacity_index < 5 ) [[likely]]
        {
            res_buf = allocate_and_init< soa_price_level_data_access_u8_t >(
                soa_bufs_capacities[ hardcoded_capacity_index ] );
        }
        else
        {
            if( hardcoded_capacity_index < 7 ) [[likely]]
            {
                res_buf = allocate_and_init< soa_price_level_data_access_u16_t >(
                    soa_bufs_capacities[ hardcoded_capacity_index ] );
            }
            else
            {
                // Capacity is too big.
                // We need to allocate a custom block.

                if( capacity >= 0xC000'0000U ) [[unlikely]]
                {
                    // That is terrible in current implementation.
                    // we cant even add its quarter to this number.
                    std::abort();
                }

                // Reserve 1/4 more then requested.
                const auto overprovision_capacity = capacity + capacity / 4;

                if( overprovision_capacity
                    <= soa_price_level_data_access_u16_t::max_capacity )
                {
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u16_t >(
                            overprovision_capacity );
                }
                else
                {
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u32_t >(
                            overprovision_capacity );
                }
            }
        }

        return res_buf;
    }

    /**
     * @brief Allocate a Price level buffer with at least a given capacity.
     *
     * @pre The requested capacity must be one of:
     *      `1, 18, 36, 73, 146, 254, 512, 1024`
     *      or a value greater than 1024.
     */
    template < Soa_Index_Type Other_Index_Type >
    [[nodiscard]] raw_buffer_t allocate_buffer(
        std::uint32_t capacity,
        soa_price_level_data_access_t< Other_Index_Type > src_data )
    {
        assert( capacity >= 1 );
        raw_buffer_t res_buf{};
        const auto hardcoded_capacity_index = soa_buf_cap_index( capacity );

        // Nothing to reuse so we need to ask m_mem for a new chunk of memory.
        if( hardcoded_capacity_index < 5 ) [[likely]]
        {
            res_buf = allocate_and_init< soa_price_level_data_access_u8_t >(
                soa_bufs_capacities[ hardcoded_capacity_index ], src_data );
        }
        else
        {
            if( hardcoded_capacity_index < 7 ) [[likely]]
            {
                res_buf = allocate_and_init< soa_price_level_data_access_u16_t >(
                    soa_bufs_capacities[ hardcoded_capacity_index ], src_data );
            }
            else
            {
                // Capacity is too big.
                // We need to allocate a custom block.

                if( capacity >= 0xC000'0000U ) [[unlikely]]
                {
                    // That is terrible in current implementation.
                    // we cant even add its quarter to this number.
                    std::abort();
                }

                // Reserve 1/4 more then requested.
                const auto overprovision_capacity = capacity + capacity / 4;

                if( overprovision_capacity
                    <= soa_price_level_data_access_u16_t::max_capacity )
                {
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u16_t >(
                            overprovision_capacity, src_data );
                }
                else
                {
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u32_t >(
                            overprovision_capacity, src_data );
                }
            }
        }

        return res_buf;
    }

    void deallocate_buffer( raw_buffer_t buf )
    {
        assert( buf.size() > 8 );
        m_mem.deallocate( buf.data(), buf.size(), soa_buf_alignment );
    }

private:
    /**
     * @brief A helper function to allocate a buffer and return a span.
     */
    [[nodiscard]] [[gnu::always_inline]] raw_buffer_t allocate_raw_buffer(
        std::size_t sz )
    {
        std::byte * const ptr =
            static_cast< std::byte * >( m_mem.allocate( sz, soa_buf_alignment ) );

        return raw_buffer_t{ ptr, sz };
    }

    /**
     * @brief Helper to allocate and immediately initialize a typed SOA layout.
     */
    template < typename Data_Accessor >
    [[nodiscard]] [[gnu::always_inline]] raw_buffer_t allocate_and_init(
        std::uint32_t capacity )
    {
        auto res_buf =
            allocate_raw_buffer( Data_Accessor::required_size( capacity ) );
        Data_Accessor::initialize_layout( res_buf, capacity );
        return res_buf;
    }

    template < typename Data_Accessor, Soa_Index_Type Other_Index_Type >
    [[nodiscard]] [[gnu::always_inline]] raw_buffer_t allocate_and_init(
        std::uint32_t capacity,
        soa_price_level_data_access_t< Other_Index_Type > src_data )
    {
        auto res_buf =
            allocate_raw_buffer( Data_Accessor::required_size( capacity ) );
        Data_Accessor::initialize_layout( res_buf, capacity, src_data );
        return res_buf;
    }

    std::pmr::unsynchronized_pool_resource m_mem;
};

}  // namespace details

}  // namespace jacobi::book
