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
#include <range/v3/view/subrange.hpp>

#include <jacobi/book/soa_price_level_common.hpp>

namespace jacobi::book
{

namespace details
{

using unified_soa_index_t = std::uint32_t;
using unified_soa_position_t =
    soa_price_level_order_node_pos_t< unified_soa_index_t >;

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
// unified_soa_price_level_data_access_t
//

/**
 * @brie Allows a access to orders data in soa-buffer.
 */
class unified_soa_price_level_data_access_t
{
public:
    using raw_order_id_t  = order_id_t::value_type;
    using raw_order_qty_t = order_qty_t::value_type;

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
    /// @}

    /**
     * @brief Create accessor object for a given buffer.
     *
     * @pre Buffer pointed by raw_data must be aligned to 8.
     * @pre Buffer size must at least capable of storing size and capacity
     *      components which means minimum size is 8.
     */
    explicit unified_soa_price_level_data_access_t(
        raw_buffer_t raw_data ) noexcept
        : m_raw_data{ raw_data }
        , m_cached_size{ soa_buf_read_u32( m_raw_data.data() ) }
        , m_cached_capacity{ soa_buf_read_u32( m_raw_data.data()
                                               + soa_buf_capacity_offset ) }
        , m_ids{ make_ids_ptr( m_raw_data.data() ) }
        , m_qtys{ make_qtys_ptr( m_raw_data.data(), m_cached_capacity ) }
    {
    }

    unified_soa_price_level_data_access_t(
        const unified_soa_price_level_data_access_t & ) = delete;
    unified_soa_price_level_data_access_t & operator=(
        const unified_soa_price_level_data_access_t & ) = delete;

    unified_soa_price_level_data_access_t(
        unified_soa_price_level_data_access_t && ) = default;
    unified_soa_price_level_data_access_t & operator=(
        unified_soa_price_level_data_access_t && ) = default;

    [[nodiscard]] std::uint32_t size() const noexcept { return m_cached_size; }

    [[nodiscard]] std::uint32_t capacity() const noexcept
    {
        return m_cached_capacity;
    }

    [[nodiscard]] bool empty() const noexcept { return 0 == size(); }

    [[nodiscard]] bool full() const noexcept
    {
        return size() == m_cached_capacity;
    }

    [[nodiscard]] const raw_order_id_t * ids() const noexcept { return m_ids; }

    [[nodiscard]] const raw_order_qty_t * qtys() const noexcept { return m_qtys; }

    [[nodiscard]] raw_order_id_t * ids() noexcept { return m_ids; }

    [[nodiscard]] raw_order_qty_t * qtys() noexcept { return m_qtys; }

    [[nodiscard]] raw_buffer_t buffer() const noexcept { return m_raw_data; }

private:
    [[nodiscard]] static raw_order_id_t * make_ids_ptr( std::byte * buf ) noexcept
    {
        std::byte * ptr = buf + ids_offset();

        assert( 0
                == reinterpret_cast< std::uintptr_t >( ptr )
                       % alignof( raw_order_id_t ) );

        return reinterpret_cast< raw_order_id_t * >(
            std::assume_aligned< alignof( raw_order_id_t ) >( ptr ) );
    }

    [[nodiscard]] static raw_order_qty_t * make_qtys_ptr(
        std::byte * buf,
        std::uint32_t cap ) noexcept
    {
        std::byte * ptr = buf + qtys_offset( cap );

        assert( 0
                == reinterpret_cast< std::uintptr_t >( ptr )
                       % alignof( raw_order_qty_t ) );

        return reinterpret_cast< raw_order_qty_t * >(
            std::assume_aligned< alignof( raw_order_qty_t ) >( ptr ) );
    }

protected:
    raw_buffer_t m_raw_data;
    std::uint32_t m_cached_size;
    const std::uint32_t m_cached_capacity;
    raw_order_id_t * m_ids;
    raw_order_qty_t * m_qtys;
};

// ==================================================================
// We expect a certain alignment for the data:
static_assert(
    unified_soa_price_level_data_access_t::ids_offset()
        % alignof( unified_soa_price_level_data_access_t::raw_order_id_t )
    == 0 );

static_assert(
    unified_soa_price_level_data_access_t::qtys_offset( 1 )
        % alignof( unified_soa_price_level_data_access_t::raw_order_qty_t )
    == 0 );
static_assert(
    unified_soa_price_level_data_access_t::qtys_offset( 2 )
        % alignof( unified_soa_price_level_data_access_t::raw_order_qty_t )
    == 0 );
static_assert(
    unified_soa_price_level_data_access_t::qtys_offset( 3 )
        % alignof( unified_soa_price_level_data_access_t::raw_order_qty_t )
    == 0 );
static_assert(
    unified_soa_price_level_data_access_t::qtys_offset( 4 )
        % alignof( unified_soa_price_level_data_access_t::raw_order_qty_t )
    == 0 );
// ==================================================================

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
// data_buf_accessor_type
//

/**
 * @brief A hint what type of interpreter to apply to a buffer.
 */
enum class data_buf_accessor_type
{
    u8_links,
    u16_links,
    u32_links,
};

//
// make_accessor_type()
//

template < Soa_Index_Type Index_Type >
constexpr data_buf_accessor_type make_accessor_type() noexcept
{
    if constexpr( std::is_same_v< Index_Type, std::uint8_t > )
    {
        return data_buf_accessor_type::u8_links;
    }
    if constexpr( std::is_same_v< Index_Type, std::uint16_t > )
    {
        return data_buf_accessor_type::u16_links;
    }

    return data_buf_accessor_type::u32_links;
}

//
// JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
//

#if __cplusplus >= 202302L
#    define JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE std::unreachable();
#elif defined( __GNUC__ ) || defined( __clang__ )
#    define JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE __builtin_unreachable();
#elif defined( _MSC_VER )
#    define JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE __assume( false );
#endif

/**
 * @name Iterator for .
 */
/// @{
/**
 * @brief Universal soa iterator.
 */
template < bool Is_Reverse, bool Is_Const >
struct order_soa_data_iterator_t
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
                            const unified_soa_price_level_data_access_t,
                            unified_soa_price_level_data_access_t >;

    parent_type_t * parent{};
    unified_soa_index_t current_link{};
    data_buf_accessor_type acc_type{};

    // Grant access to all other instantiations of this template.
    template < bool, bool >
    friend struct order_soa_data_iterator_t;

    order_soa_data_iterator_t() = default;

    template < typename Parent_Exact_Type >
    order_soa_data_iterator_t( Parent_Exact_Type * p,
                               unified_soa_index_t link ) noexcept
        : parent{ p }
        , current_link{ link }
        , acc_type{ Parent_Exact_Type::accessor_type }
    {
    }

    // NOTE: Implementations of the next two functions use
    //       soa_price_level_data_access_u8_t
    //       soa_price_level_data_access_u16_t
    //       and soa_price_level_data_access_u32_t
    //       definitions and as such cannot be defined here
    //       See implementation after
    //       soa_price_level_data_access_t<I> is defined.
    /**
     * @brief Unified way to ask for the `.prev` position
     *        of a currently observed item.
     */
    [[nodiscard]] unified_soa_index_t link_prev_at(
        unified_soa_index_t ix ) const noexcept;

    /**
     * @brief Unified way to ask for the `.next` position
     *        of a currently observed item.
     */
    [[nodiscard]] unified_soa_index_t link_next_at(
        unified_soa_index_t ix ) const noexcept;

    /**
     * @brief Converting Constructor
     *
     * Privides: `Non-Const => Const` and `Forward <=> Reverse`.
     */
    template < bool Other_Reverse, bool Other_Const >
        requires( Is_Const || !Other_Const )  // Only one way Non-Const => Const
    constexpr explicit(
        Is_Reverse != Other_Reverse )  // Must be explicit if change direction.
        order_soa_data_iterator_t(
            const order_soa_data_iterator_t< Other_Reverse, Other_Const > &
                other ) noexcept
        : parent{ other.parent }
        , current_link{ other.current_link }
        , acc_type{ other.acc_type }
    {
        if constexpr( Is_Reverse != Other_Reverse )
        {
            // When converting between direct and reverse,
            // we must physically shift the pointer by one
            // to maintain [begin, end) interval symmetry.
            if constexpr( Is_Reverse )
            {
                current_link = link_prev_at( current_link );
            }
            else
            {
                current_link = link_next_at( current_link );
            }
        }
    }

    /**
     * @brief Extract the underlying forward iterator.
     */
    [[nodiscard]] constexpr order_soa_data_iterator_t< false, Is_Const > base()
        const noexcept
        requires( Is_Reverse )  // Only available on reverse iterators.
    {
        return order_soa_data_iterator_t< false, Is_Const >( *this );
    }

    [[nodiscard]] value_type operator*() const noexcept
    {
        const auto i = current_link - extra_links_for_anchors;
        return { order_id_t{ parent->ids()[ i ] },
                 order_qty_t{ parent->qtys()[ i ] } };
    }

    order_soa_data_iterator_t & operator++() noexcept
    {
        if constexpr( Is_Reverse )
        {
            current_link = link_prev_at( current_link );
        }
        else
        {
            current_link = link_next_at( current_link );
        }
        return *this;
    }

    order_soa_data_iterator_t operator++( int ) noexcept
    {
        order_soa_data_iterator_t tmp = *this;
        ++( *this );
        return tmp;
    }

    order_soa_data_iterator_t & operator--() noexcept
    {
        if constexpr( Is_Reverse )
        {
            current_link = link_next_at( current_link );
            ;
        }
        else
        {
            current_link = link_prev_at( current_link );
            ;
        }
        return *this;
    }

    order_soa_data_iterator_t operator--( int ) noexcept
    {
        order_soa_data_iterator_t tmp = *this;
        --( *this );
        return tmp;
    }

    [[nodiscard]] friend bool operator==(
        const order_soa_data_iterator_t & a,
        const order_soa_data_iterator_t & b ) noexcept
    {
        const bool res =
            ( a.parent == b.parent ) & ( a.current_link == b.current_link );

        // if parent+link_index are the same,
        // so must be accessor types.
        assert( !res || a.acc_type == b.acc_type );
        return res;
    }
};

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
class soa_price_level_data_access_t : public unified_soa_price_level_data_access_t
{
public:
    constexpr inline static data_buf_accessor_type accessor_type =
        make_accessor_type< Index_Type >();

    using short_index_t = Index_Type;
    constexpr inline static short_index_t links_head_pos{ 0 };
    constexpr inline static short_index_t links_free_head_pos{ 1 };

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
    [[nodiscard]] constexpr static std::size_t links_offset(
        std::size_t cap ) noexcept
    {
        return qtys_offset( cap ) + cap * sizeof( raw_order_qty_t );
    }

    // ==================================================================
    // We expect a certain alignment for the data:
    static_assert( links_offset( 1 ) % alignof( raw_soa_links_t ) == 0 );
    static_assert( links_offset( 2 ) % alignof( raw_soa_links_t ) == 0 );
    static_assert( links_offset( 3 ) % alignof( raw_soa_links_t ) == 0 );
    static_assert( links_offset( 4 ) % alignof( raw_soa_links_t ) == 0 );
    // ==================================================================
    /// @}

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
        const soa_price_level_data_access_t< Other_Index_Type > &
            src_data ) noexcept
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
        : unified_soa_price_level_data_access_t{ raw_data }
        , m_links{ make_links_ptr( raw_data.data(), m_cached_capacity ) }
    {
    }

    soa_price_level_data_access_t( const soa_price_level_data_access_t & ) =
        delete;
    soa_price_level_data_access_t & operator=(
        const soa_price_level_data_access_t & ) = delete;

    soa_price_level_data_access_t( soa_price_level_data_access_t && ) = default;
    soa_price_level_data_access_t & operator=( soa_price_level_data_access_t && ) =
        default;

    [[nodiscard]] raw_soa_links_t * links() noexcept { return m_links; }

    [[nodiscard]] const raw_soa_links_t * links() const noexcept
    {
        return m_links;
    }

    /**
     * @brief Remove order at a given position.
     *
     * @pre pos must be pointing to a location within capacity
     */
    void pop_at( unified_soa_position_t pos ) noexcept
    {
        assert( !empty() );
        assert( pos.node_link_index() >= extra_links_for_anchors );
        assert( pos.node_link_index() < static_cast< std::size_t >(
                    extra_links_for_anchors + m_cached_capacity ) );

        const auto i = static_cast< short_index_t >( pos.node_link_index() );
        unlink_node( i );

        m_links[ i ].next                   = m_links[ links_free_head_pos ].next;
        m_links[ links_free_head_pos ].next = i;

        --m_cached_size;
        flush_cached_size();
    }

    /**
     * @brief Add an order.
     *
     * @pre Level must not be full.
     *
     * @return Link position of the allocated node.
     */
    [[nodiscard]] unified_soa_position_t push_back(
        order_id_t id_value,
        order_qty_t qty_value ) noexcept
    {
        assert( !full() );
        auto free_link = m_links[ links_free_head_pos ];

        unified_soa_index_t i;
        if( free_link.next != links_free_head_pos )
        {
            // First supply from free list.
            i              = free_link.next;
            free_link.next = m_links[ i ].next;
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

        m_links[ links_free_head_pos ] = free_link;

        ids()[ i - extra_links_for_anchors ]  = type_safe::get( id_value );
        qtys()[ i - extra_links_for_anchors ] = type_safe::get( qty_value );

        insert_node( i, links_head_pos );

        ++m_cached_size;
        flush_cached_size();

        assert( i >= extra_links_for_anchors );
        return unified_soa_position_t{ i };
    }

    /**
     * @name Range support.
     */
    /// @{
    //                                              Is_Reverse  Is_Const
    using iterator               = order_soa_data_iterator_t< false, false >;
    using const_iterator         = order_soa_data_iterator_t< false, true >;
    using reverse_iterator       = order_soa_data_iterator_t< true, false >;
    using const_reverse_iterator = order_soa_data_iterator_t< true, true >;

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
    /**
     * @brief Write current cached size to buffer.
     */
    void flush_cached_size() noexcept
    {
        assert( m_cached_size <= m_cached_capacity );
        soa_buf_write_u32( m_cached_size, m_raw_data.data() );
    }

    void unlink_node( short_index_t i ) noexcept
    {
        const auto before_link = m_links[ i ];

        // Make neighbours of eliminated node linked:
        m_links[ before_link.prev ].next = before_link.next;
        m_links[ before_link.next ].prev = before_link.prev;
    }

    void insert_node( short_index_t i, short_index_t head ) noexcept
    {
        // Here: we have i-th node disconnected from the list.
        //       What we need to do is to add i-th as new node
        //       before head.

        const auto t = m_links[ head ].prev;

        // Set new linkage for i-th node.
        m_links[ i ] = { t, head };

        m_links[ head ].prev = i;
        m_links[ t ].next    = i;
    }

    [[nodiscard]] static raw_soa_links_t * make_links_ptr(
        std::byte * buf,
        std::uint32_t cap ) noexcept
    {
        std::byte * ptr = buf + links_offset( cap );
        assert( 0
                == reinterpret_cast< std::uintptr_t >( ptr )
                       % alignof( raw_soa_links_t ) );
        return reinterpret_cast< raw_soa_links_t * >(
            std::assume_aligned< alignof( raw_soa_links_t ) >( ptr ) );
    }

    raw_soa_links_t * m_links;
};

using soa_price_level_data_access_u8_t =
    soa_price_level_data_access_t< std::uint8_t >;
using soa_price_level_data_access_u16_t =
    soa_price_level_data_access_t< std::uint16_t >;
using soa_price_level_data_access_u32_t =
    soa_price_level_data_access_t< std::uint32_t >;

static_assert(
    std::is_trivially_destructible_v< soa_price_level_data_access_u8_t > );
static_assert(
    std::is_trivially_destructible_v< soa_price_level_data_access_u16_t > );
static_assert(
    std::is_trivially_destructible_v< soa_price_level_data_access_u32_t > );

static_assert( alignof( soa_price_level_data_access_u8_t )
               == alignof( soa_price_level_data_access_u16_t ) );
static_assert( alignof( soa_price_level_data_access_u16_t )
               == alignof( soa_price_level_data_access_u32_t ) );

static_assert( sizeof( soa_price_level_data_access_u8_t )
               == sizeof( soa_price_level_data_access_u16_t ) );
static_assert( sizeof( soa_price_level_data_access_u16_t )
               == sizeof( soa_price_level_data_access_u32_t ) );

//
// Legit_SOA_Data_Accessor
//

/**
 * @brief A concept of what is recognized as an accessor.
 */
template < typename T >
concept Legit_SOA_Data_Accessor =
    std::same_as< T, soa_price_level_data_access_u8_t >
    || std::same_as< T, soa_price_level_data_access_u16_t >
    || std::same_as< T, soa_price_level_data_access_u32_t >;

//
// order_soa_data_iterator_t
//

// Implementations of get-functions for prev/next component of a link
// which accessable throug a Legit_SOA_Data_Accessor type.
template < bool Is_Reverse, bool Is_Const >
unified_soa_index_t
order_soa_data_iterator_t< Is_Reverse, Is_Const >::link_prev_at(
    unified_soa_index_t ix ) const noexcept
{
    switch( acc_type )
    {
        case data_buf_accessor_type::u8_links:
        {
            return reinterpret_cast< const soa_price_level_data_access_u8_t * >(
                       parent )
                ->links()[ ix ]
                .prev;
        }
        case data_buf_accessor_type::u16_links:
        {
            return reinterpret_cast< const soa_price_level_data_access_u16_t * >(
                       parent )
                ->links()[ ix ]
                .prev;
        }
        case data_buf_accessor_type::u32_links:
        {
            return reinterpret_cast< const soa_price_level_data_access_u32_t * >(
                       parent )
                ->links()[ ix ]
                .prev;
        }
        default:
            JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
    }
}

template < bool Is_Reverse, bool Is_Const >
unified_soa_index_t
order_soa_data_iterator_t< Is_Reverse, Is_Const >::link_next_at(
    unified_soa_index_t ix ) const noexcept
{
    switch( acc_type )
    {
        case data_buf_accessor_type::u8_links:
        {
            return reinterpret_cast< const soa_price_level_data_access_u8_t * >(
                       parent )
                ->links()[ ix ]
                .next;
        }
        case data_buf_accessor_type::u16_links:
        {
            return reinterpret_cast< const soa_price_level_data_access_u16_t * >(
                       parent )
                ->links()[ ix ]
                .next;
        }
        case data_buf_accessor_type::u32_links:
        {
            return reinterpret_cast< const soa_price_level_data_access_u32_t * >(
                       parent )
                ->links()[ ix ]
                .next;
        }
        default:
            JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
    }
}

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
    constexpr inline static std::size_t dummy_zero_capacity_buffer_size =
        8
        + extra_links_for_anchors
              * sizeof( soa_price_level_data_access_u8_t::raw_soa_links_t );

    /**
     * @brief Dummy soa-buffer With zero capacity.
     *
     * Represents a normal soa-buffer to be initially used for price levels.
     */
    alignas( 64 ) static inline std::array<
        std::byte,
        dummy_zero_capacity_buffer_size > dummy_zero_capacity_buffer{
        // clang-format off
        // SIZE:
            std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0x0 },
        // CAPACITY:
            std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0x0 },
        // LINKS:
                   //  Prev              Next
    /*[0]*/ std::byte{ 0x0 }, std::byte{ 0x0 },           // HEAD ANCHOR
    /*[1]*/ std::byte{ 0x2 }, std::byte{ 0x1 },           // FREE-NODES HEAD ANCHOR
        // clang-format on
    };

public:
    [[nodiscard]] static raw_buffer_t make_zero_capacity_buffer() noexcept
    {
        return raw_buffer_t{ dummy_zero_capacity_buffer.data(),
                             dummy_zero_capacity_buffer.size() };
    }

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

        const auto index =
            static_cast< uint32_t >( std::bit_width( cap + 7 ) - 4 );

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
     *      `1, 18, 36, 73, 146, 255, 512, 1024`
     *      or a value greater than 1024.
     */
    [[nodiscard]] std::pair< raw_buffer_t, data_buf_accessor_type >
    allocate_buffer( std::uint32_t capacity )
    {
        assert( capacity >= 1 );
        raw_buffer_t res_buf{};
        const auto hardcoded_capacity_index = soa_buf_cap_index( capacity );

        data_buf_accessor_type accessor_type = data_buf_accessor_type::u8_links;

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
                accessor_type = data_buf_accessor_type::u16_links;
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
                    accessor_type = data_buf_accessor_type::u16_links;
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u16_t >(
                            overprovision_capacity );
                }
                else
                {
                    accessor_type = data_buf_accessor_type::u32_links;
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u32_t >(
                            overprovision_capacity );
                }
            }
        }

        return std::make_pair( res_buf, accessor_type );
    }

    /**
     * @brief Allocate a Price level buffer with at least a given capacity.
     *
     * @pre The requested capacity must be one of:
     *      `1, 18, 36, 73, 146, 255, 512, 1024`
     *      or a value greater than 1024.
     */
    template < Soa_Index_Type Other_Index_Type >
    [[nodiscard]] std::pair< raw_buffer_t, data_buf_accessor_type >
    allocate_buffer(
        std::uint32_t capacity,
        const soa_price_level_data_access_t< Other_Index_Type > & src_data )
    {
        assert( capacity >= 1 );
        raw_buffer_t res_buf{};
        const auto hardcoded_capacity_index = soa_buf_cap_index( capacity );

        data_buf_accessor_type accessor_type = data_buf_accessor_type::u8_links;
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
                accessor_type = data_buf_accessor_type::u16_links;
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
                    accessor_type = data_buf_accessor_type::u16_links;
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u16_t >(
                            overprovision_capacity, src_data );
                }
                else
                {
                    accessor_type = data_buf_accessor_type::u32_links;
                    res_buf =
                        allocate_and_init< soa_price_level_data_access_u32_t >(
                            overprovision_capacity, src_data );
                }
            }
        }

        return std::make_pair( res_buf, accessor_type );
    }

    void deallocate_buffer( raw_buffer_t buf )
    {
        if( dummy_zero_capacity_buffer_size > buf.size() ) [[likely]]
        {
            m_mem.deallocate( buf.data(), buf.size(), soa_buf_alignment );
        }
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
    template < Legit_SOA_Data_Accessor Data_Accessor >
    [[nodiscard]] [[gnu::always_inline]] raw_buffer_t allocate_and_init(
        std::uint32_t capacity )
    {
        auto res_buf =
            allocate_raw_buffer( Data_Accessor::required_size( capacity ) );
        Data_Accessor::initialize_layout( res_buf, capacity );
        return res_buf;
    }

    template < Legit_SOA_Data_Accessor Data_Accessor,
               Soa_Index_Type Other_Index_Type >
    [[nodiscard]] [[gnu::always_inline]] raw_buffer_t allocate_and_init(
        std::uint32_t capacity,
        const soa_price_level_data_access_t< Other_Index_Type > & src_data )
    {
        auto res_buf =
            allocate_raw_buffer( Data_Accessor::required_size( capacity ) );
        Data_Accessor::initialize_layout( res_buf, capacity, src_data );
        return res_buf;
    }

    std::pmr::unsynchronized_pool_resource m_mem;
};

//
// soa_data_handle_t
//

/**
 * @brief An object responsible for storing data.
 */
struct soa_data_handle_t
{
public:
    soa_data_handle_t() noexcept
    {
        std::ignore = reinit< soa_price_level_data_access_u8_t >(
            std_soa_buffers_pool_t::make_zero_capacity_buffer() );
    }

    soa_data_handle_t( const soa_data_handle_t & )             = delete;
    soa_data_handle_t & operator=( const soa_data_handle_t & ) = delete;

    soa_data_handle_t( soa_data_handle_t && other ) noexcept
    {
        std::ignore =
            reinit( other.m_accessor_type,
                    other.reinit< soa_price_level_data_access_u8_t >(
                        std_soa_buffers_pool_t::make_zero_capacity_buffer() ) );
    }

    soa_data_handle_t & operator=( soa_data_handle_t && other ) noexcept
    {
        soa_data_handle_t tmp{ std::move( other ) };
        swap( *this, tmp );
        return *this;
    }

    /**
     * @name Current buffer accessor management.
     *
     * @return former buffer.
     */
    /// @{
    template < Legit_SOA_Data_Accessor Data_Accessor >
    [[nodiscard]] raw_buffer_t reinit( raw_buffer_t buffer ) noexcept
    {
        auto former_buf = unified_accessor()->buffer();

        // Use .data() to get the raw pointer for placement new
        new( m_accessor_storage.data() ) Data_Accessor( buffer );

        m_accessor_type = Data_Accessor::accessor_type;

        return former_buf;
    }

    [[nodiscard]] raw_buffer_t reinit( data_buf_accessor_type acc_type,
                                       raw_buffer_t buffer ) noexcept
    {
        switch( acc_type )
        {
            case data_buf_accessor_type::u8_links:
            {
                return reinit< soa_price_level_data_access_u8_t >( buffer );
            }
            case data_buf_accessor_type::u16_links:
            {
                return reinit< soa_price_level_data_access_u16_t >( buffer );
            }
            case data_buf_accessor_type::u32_links:
            {
                return reinit< soa_price_level_data_access_u32_t >( buffer );
            }
            default:
                JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
        }
    }

    [[nodiscard]] data_buf_accessor_type accessor_type() const noexcept
    {
        assert( data_buf_accessor_type::u8_links == m_accessor_type
                || data_buf_accessor_type::u16_links == m_accessor_type
                || data_buf_accessor_type::u32_links == m_accessor_type );

        return m_accessor_type;
    }
    /// @}

    [[nodiscard]] unified_soa_price_level_data_access_t *
    unified_accessor() noexcept
    {
        return reinterpret_cast< unified_soa_price_level_data_access_t * >(
            m_accessor_storage.data() );
    }

    [[nodiscard]] const unified_soa_price_level_data_access_t * unified_accessor()
        const noexcept
    {
        return reinterpret_cast< const unified_soa_price_level_data_access_t * >(
            m_accessor_storage.data() );
    }

    template < Legit_SOA_Data_Accessor Data_Accessor >
    [[nodiscard]] Data_Accessor * typed_accessor() noexcept
    {
        return reinterpret_cast< Data_Accessor * >( m_accessor_storage.data() );
    }

    template < Legit_SOA_Data_Accessor Data_Accessor >
    [[nodiscard]] const Data_Accessor * typed_accessor() const noexcept
    {
        return reinterpret_cast< const Data_Accessor * >(
            m_accessor_storage.data() );
    }

    [[nodiscard]] raw_buffer_t buffer() const noexcept
    {
        return unified_accessor()->buffer();
    }

    friend void swap( soa_data_handle_t & a, soa_data_handle_t & b )
    {
        auto buf_a       = a.unified_accessor()->buffer();
        auto buf_b       = b.unified_accessor()->buffer();
        const auto acc_a = a.m_accessor_type;
        const auto acc_b = b.m_accessor_type;

        std::ignore = a.reinit( acc_b, buf_b );
        std::ignore = b.reinit( acc_a, buf_a );
    }

private:
    using sample_acc_type_t = soa_price_level_data_access_u8_t;
    /**
     * @brief storage for current accessor.
     */
    alignas( sample_acc_type_t )
        std::array< std::byte, sizeof( sample_acc_type_t ) > m_accessor_storage;

    /**
     * @brief Indicator what kind of buffer does the price level
     *        store its data in.
     */
    data_buf_accessor_type m_accessor_type{
        details::data_buf_accessor_type::u8_links
    };
};

}  // namespace details

//
// soa_price_level2_t
//

/**
 * @brief A storage of the orders on a given level using SoA to store data
 *        and using reusable raw buffers to store levels data.
 */
class soa_price_level2_t
{
public:
    using unufied_data_access_t = details::unified_soa_price_level_data_access_t;
    using data_access_u8_t      = details::soa_price_level_data_access_u8_t;
    using data_access_u16_t     = details::soa_price_level_data_access_u16_t;
    using data_access_u32_t     = details::soa_price_level_data_access_u32_t;

    using position_t = details::unified_soa_position_t;

    // We don't want copy this object.
    soa_price_level2_t( const soa_price_level2_t & )             = delete;
    soa_price_level2_t & operator=( const soa_price_level2_t & ) = delete;

    explicit soa_price_level2_t( order_price_t p,
                                 details::std_soa_buffers_pool_t * buffers_pool )
        : m_price{ p }
        , m_buffers_pool{ buffers_pool }
    {
    }

    ~soa_price_level2_t() { m_buffers_pool->deallocate_buffer( m_data.buffer() ); }

    friend inline void swap( soa_price_level2_t & lvl1,
                             soa_price_level2_t & lvl2 ) noexcept
    {
        using std::swap;
        swap( lvl1.m_price, lvl2.m_price );
        swap( lvl1.m_orders_qty, lvl2.m_orders_qty );
        swap( lvl1.m_buffers_pool, lvl2.m_buffers_pool );
        swap( lvl1.m_data, lvl2.m_data );
    }

    explicit soa_price_level2_t( soa_price_level2_t && lvl )
        : m_price{ lvl.m_price }
        , m_orders_qty{ lvl.m_orders_qty }
        , m_buffers_pool{ lvl.m_buffers_pool }
        , m_data{ std::move( lvl.m_data ) }
    {
        lvl.m_orders_qty = order_qty_t{};
    }

    soa_price_level2_t & operator=( soa_price_level2_t && lvl )
    {
        soa_price_level2_t tmp{ std::move( lvl ) };
        swap( *this, tmp );

        return *this;
    }

    using reference_t =
        soa_price_level_order_reference_t< details::unified_soa_position_t >;

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

        switch( m_data.accessor_type() )
        {
            case details::data_buf_accessor_type::u8_links:
            {
                return add_order_impl< data_access_u8_t >( order );
            }
            case details::data_buf_accessor_type::u16_links:
            {
                return add_order_impl< data_access_u16_t >( order );
            }
            case details::data_buf_accessor_type::u32_links:
            {
                return add_order_impl< data_access_u32_t >( order );
            }
            default:
                JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
        }
    }

    /**
     * @brief Remove an order identified by reference.
     */
    void delete_order( const reference_t & ref ) noexcept
    {
        assert( ref.price() == m_price );
        assert( orders_count() > 0 );
        assert( ref.pos().data_index() < m_data.unified_accessor()->capacity() );

        switch( m_data.accessor_type() )
        {
            case details::data_buf_accessor_type::u8_links:
            {
                return delete_order_impl< data_access_u8_t >( ref );
            }
            case details::data_buf_accessor_type::u16_links:
            {
                return delete_order_impl< data_access_u16_t >( ref );
            }
            case details::data_buf_accessor_type::u32_links:
            {
                return delete_order_impl< data_access_u32_t >( ref );
            }
            default:
                JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
        }
    }

    /**
     * @brief Reduce total qty on this level (execute,reduce or modify).
     */
    [[nodiscard]] reference_t reduce_qty( const reference_t & ref,
                                          order_qty_t qty ) noexcept
    {
        assert( ref.price() == m_price );
        assert( orders_count() > 0 );
        assert( ref.pos().data_index() < m_data.unified_accessor()->capacity() );
        assert( type_safe::get( ref.make_order().id )
                == m_data.unified_accessor()->ids()[ ref.pos().data_index() ] );
        assert( type_safe::get( ref.make_order().qty )
                == m_data.unified_accessor()->qtys()[ ref.pos().data_index() ] );

        auto order = ref.make_order();
        assert( order.qty > qty );

        order.qty -= qty;
        m_orders_qty -= qty;

        m_data.unified_accessor()->qtys()[ ref.pos().data_index() ] =
            type_safe::get( order.qty );

        return reference_t{ order, ref.pos() };
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
        return m_data.unified_accessor()->size();
    }

    /**
     * @brief A total quantity in all orders.
     */
    [[nodiscard]] order_qty_t orders_qty() const noexcept { return m_orders_qty; }

    [[nodiscard]] order_t order_at( const reference_t & ref )
    {
        assert( ref.price() == m_price );
        assert( orders_count() > 0 );
        assert( ref.pos().data_index() < m_data.unified_accessor()->capacity() );
        assert( type_safe::get( ref.make_order().id )
                == m_data.unified_accessor()->ids()[ ref.pos().data_index() ] );
        assert( type_safe::get( ref.make_order().qty )
                == m_data.unified_accessor()->qtys()[ ref.pos().data_index() ] );

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

        return order_data_range()
               | ranges::views::transform(
                   [ price = m_price ]( auto id_qty ) -> order_t
                   { return order_t{ id_qty.first, id_qty.second, price }; } );
    }

    /**
     * @brief Get a range of the order on this level in reverse.
     */
    [[nodiscard]] auto orders_range_reverse() const noexcept
    {
        return order_data_range_reverse()
               | ranges::views::transform(
                   [ price = m_price ]( auto id_qty ) -> order_t
                   { return order_t{ id_qty.first, id_qty.second, price }; } );
    }

    /**
     * @brief Get the first order on this level.
     *
     * @pre Level MUST NOT be empty.
     */
    [[nodiscard]] order_t first_order() const noexcept
    {
        assert( !empty() );

        const auto [ id, qty ] = *( order_data_range().begin() );

        return order_t{ id, qty, m_price };
    }

private:
    /**
     * @name Order manipulations implementation
     */
    /// @{
    template < details::Legit_SOA_Data_Accessor Data_Accessor >
    [[nodiscard]] static reference_t add_order_impl_not_full(
        Data_Accessor * acc,
        order_t order ) noexcept
    {
        assert( !acc->full() );
        return reference_t{ order, acc->push_back( order.id, order.qty ) };
    }

    template < details::Legit_SOA_Data_Accessor Data_Accessor >
    [[nodiscard]] reference_t add_order_impl( order_t order )
    {
        auto * acc = m_data.typed_accessor< Data_Accessor >();

        if( !acc->full() ) [[likely]]
        {
            m_orders_qty += order.qty;
            return add_order_impl_not_full( acc, order );
        }

        auto [ new_data, acc_type ] =
            m_buffers_pool->allocate_buffer( acc->capacity() + 1, *acc );

        m_buffers_pool->deallocate_buffer( m_data.reinit( acc_type, new_data ) );

        m_orders_qty += order.qty;
        switch( m_data.accessor_type() )
        {
            case details::data_buf_accessor_type::u8_links:
            {
                return add_order_impl_not_full(
                    m_data.typed_accessor< data_access_u8_t >(), order );
            }
            case details::data_buf_accessor_type::u16_links:
            {
                return add_order_impl_not_full(
                    m_data.typed_accessor< data_access_u16_t >(), order );
            }
            case details::data_buf_accessor_type::u32_links:
            {
                return add_order_impl_not_full(
                    m_data.typed_accessor< data_access_u32_t >(), order );
            }
            default:
                JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
        }
    }

    template < details::Legit_SOA_Data_Accessor Data_Accessor >
    void delete_order_impl( const reference_t & ref ) noexcept
    {
        m_orders_qty -= ref.make_order().qty;
        m_data.typed_accessor< Data_Accessor >()->pop_at( ref.pos() );
    }
    /// @}

    /**
     * @name Range support helpers
     */
    /// @{
    template < typename Accessor >
    [[nodiscard]] static ranges::subrange< data_access_u8_t::const_iterator >
    order_data_range( const Accessor & acc ) noexcept
    {
        return ranges::subrange( acc.cbegin(), acc.cend() );
    }

    [[nodiscard]] ranges::subrange< data_access_u8_t::const_iterator >
    order_data_range() const noexcept
    {
        switch( m_data.accessor_type() )
        {
            case details::data_buf_accessor_type::u8_links:
            {
                return order_data_range(
                    *m_data.typed_accessor< data_access_u8_t >() );
            }
            case details::data_buf_accessor_type::u16_links:
            {
                return order_data_range(
                    *m_data.typed_accessor< data_access_u16_t >() );
            }
            case details::data_buf_accessor_type::u32_links:
            {
                return order_data_range(
                    *m_data.typed_accessor< data_access_u32_t >() );
            }
            default:
                JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
        }
    }

    template < typename Accessor >
    [[nodiscard]] static ranges::subrange<
        data_access_u8_t::const_reverse_iterator >
    order_data_range_reverse( const Accessor & acc ) noexcept
    {
        return ranges::subrange( acc.crbegin(), acc.crend() );
    }

    [[nodiscard]] ranges::subrange< data_access_u8_t::const_reverse_iterator >
    order_data_range_reverse() const noexcept
    {
        switch( m_data.accessor_type() )
        {
            case details::data_buf_accessor_type::u8_links:
            {
                return order_data_range_reverse(
                    *m_data.typed_accessor< data_access_u8_t >() );
            }
            case details::data_buf_accessor_type::u16_links:
            {
                return order_data_range_reverse(
                    *m_data.typed_accessor< data_access_u16_t >() );
            }
            case details::data_buf_accessor_type::u32_links:
            {
                return order_data_range_reverse(
                    *m_data.typed_accessor< data_access_u32_t >() );
            }
            default:
                JACOBI_BOOK_SELECT_DATA_ACCESOR_UNREACHABLE
        }
    }
    /// @}

    /**
     * @brief Current level price.
     */
    order_price_t m_price{};

    /**
     * @brief A total quantity in all orders.
     */
    order_qty_t m_orders_qty{};

    /**
     * @brief Buffers pool that supplies buffers to store soa data.
     */
    details::std_soa_buffers_pool_t * m_buffers_pool;

    // /**
    //  * @brief Level data handle.
    //  */
    details::soa_data_handle_t m_data;
};

static_assert( Price_Level_Concept< soa_price_level2_t > );

//
// soa_price_levels2_factory_t
//

class soa_price_levels2_factory_t
{
public:
    using price_level_t = soa_price_level2_t;

    [[nodiscard]] price_level_t make_price_level( order_price_t p )
    {
        return price_level_t{ p, &m_soa_buffers_pool };
    }

    void retire_price_level( [[maybe_unused]] price_level_t && price_level )
    {
        // Do nothing
    }

private:
    details::std_soa_buffers_pool_t m_soa_buffers_pool;
};

static_assert( Price_Levels_Factory_Concept< soa_price_levels2_factory_t > );

}  // namespace jacobi::book
