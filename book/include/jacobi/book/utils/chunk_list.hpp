// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Contains a specifically designed list implementation which
 *       enables a better use of cache while still having a
 *       quick insert-to-back/remove.
 */
#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <list>

namespace jacobi::book::utils
{

namespace details
{

//
// make_1bit_mask()
//

/**
 * @brief Create 64-bit mask with a single bit set to 1 at a given position.
 */
constexpr inline std::uint64_t make_1bit_mask( int pos )
{
    return 1ull << pos;
}

//
// make_1bit_mask_inverted()
//

/**
 * @brief Create 64-bit mask with a single bit set to 0 at a given position.
 */
constexpr inline std::uint64_t make_1bit_mask_inverted( int pos )
{
    return ~make_1bit_mask( pos );
}

//
// std_list_traits_t
//

struct std_list_traits_t
{
    template < typename T >
    using list_t = std::list< T >;
};

}  // namespace details

//
// chunk_node_t
//

/**
 * @brief Chunk node that is capable of storing 64 items.
 */
template < typename T >
struct chunk_node_t
{
    chunk_node_t() = default;

    chunk_node_t( const chunk_node_t & )             = delete;
    chunk_node_t( chunk_node_t && )                  = delete;
    chunk_node_t & operator=( const chunk_node_t & ) = delete;
    chunk_node_t & operator=( chunk_node_t && )      = delete;

    using item_t = T;
    /**
     * @brief A bitmask that tells which items within this chunk are
     *        holding a value.
     */
    std::uint64_t stored_mask{};

    using storage_t = std::array< item_t, sizeof( stored_mask ) * 8 >;

    /**
     * @brief Storage for the elements.
     */
    storage_t data;

    /**
     * @name Access the element at a given position.
     *
     * @pre the element might be active (associated bit in stored_mask
     *      must be set to 1).
     */
    ///@{
    item_t & at( int pos ) noexcept
    {
        assert( 0 <= pos && pos < 64
                && ( 0 != ( stored_mask & details::make_1bit_mask( pos ) ) ) );

        return data[ pos ];
    }

    const item_t & at( int pos ) const noexcept { return data[ pos ]; }
    ///@}

    /**
     * @brief Tells if a new item can be appended to this chunk.
     */
    [[nodiscard]] bool can_append_to_chunk() const noexcept
    {
        constexpr auto highest_bit_mask = details::make_1bit_mask( 63 );
        return 0 == ( stored_mask & highest_bit_mask );
    }

    /**
     * @brief Tells if the chunk is empty.
     */
    [[nodiscard]] bool empty() const noexcept { return 0 == stored_mask; }

    /**
     * @brief A position of the first element in this chunk
     *        that can store the inserted item.
     *
     * If the value equals 64 then no more elements can be
     * inserter into this chunk. Note it doesn't mean
     * that the storage is fully utilized, because
     * some the item might be deleted by setting to zero an
     * associated bit in stored_mask.
     */
    [[nodiscard]] auto insert_position() const noexcept
    {
        return 64 - std::countl_zero( stored_mask );
    }

    /**
     * @brief how many elements does the chunk holds.
     */
    [[nodiscard]] int stored_count() const noexcept
    {
        return std::popcount( stored_mask );
    }

    [[nodiscard]] int find_next_allocated( int pos ) const noexcept
    {
        return pos + 1
               + std::countr_zero( ( stored_mask >> ( pos + 1 ) )
                                   | ( 0x8000000000000000ull >> pos ) );
    }

    [[nodiscard]] int find_prev_allocated( int pos ) const noexcept
    {
        return pos
               - ( 1
                   + std::countl_zero( ( stored_mask << ( 64 - pos ) )
                                       | 0x1ull ) );
    }

    [[nodiscard]] int find_first_allocated() const noexcept
    {
        return std::countr_zero( stored_mask );
    }

    [[nodiscard]] int find_last_allocated() const noexcept
    {
        return find_prev_allocated( 64 );
    }

    /**
     * @brief Inserted a new item.
     *
     * @pre The chunk must be capable of inserting a new element
     *      (value return by `insert_position()` is less than 64).
     */
    [[nodiscard]] int append( item_t item ) noexcept
    {
        assert( insert_position() != 64 );
        int pos = insert_position();
        stored_mask |= details::make_1bit_mask( pos );
        data[ pos ] = item;

        return pos;
    }

    /**
     * @brief marks an element as deleted.
     *
     * @return Return a position of next allocated element.
     */
    int erase( int pos ) noexcept
    {
        int res = find_next_allocated( pos );
        stored_mask &= details::make_1bit_mask_inverted( pos );
        return res;
    }
};

//
// chunk_list_t
//

/**
 * @brief A specific implementation of a list that can store a bunch of
 *        items in a single node with quick iterations over them.
 */
template < typename T, typename List_Traits = details::std_list_traits_t >
class chunk_list_t
{
public:
    using item_t  = T;
    using chunk_t = chunk_node_t< item_t >;
    /**
     * @brief The type that eventually stores all the orders on this level.
     */
    using list_impl_t      = typename List_Traits::template list_t< chunk_t >;
    using chunk_iterator_t = list_impl_t::iterator;

    chunk_list_t() = default;

    chunk_list_t( const chunk_list_t & )             = delete;
    chunk_list_t & operator=( const chunk_list_t & ) = delete;

    chunk_list_t( chunk_list_t && cl )
    {
        using std::swap;
        swap( m_chunks, cl.m_chunks );
        swap( m_items_count, cl.m_items_count );
    }

    friend void swap( chunk_list_t & cl1, chunk_list_t & cl2 )
    {
        using std::swap;
        swap( cl1.m_chunks, cl2.m_chunks );
        swap( cl1.m_items_count, cl2.m_items_count );
    }

    chunk_list_t & operator=( chunk_list_t && cl )
    {
        swap( *this, cl );
        cl.m_chunks->clear();
        cl.m_items_count = 0;

        return *this;
    }

    //
    // iterator_t
    //

    /**
     * @ An iterator over the elements stored in this chunk.
     */
    template < bool Is_Const >
    class iterator_t
    {
        friend class chunk_list_t;

    public:
        using reference = std::conditional_t< Is_Const, const item_t &, item_t & >;
        using pointer   = std::conditional_t< Is_Const, const item_t *, item_t * >;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::conditional_t< Is_Const, const item_t, item_t >;
        using difference_type = std::ptrdiff_t;

        using hosting_list_t =
            std::conditional_t< Is_Const, const list_impl_t, list_impl_t >;
        using chunk_iterator_t = std::remove_reference_t<
            decltype( std::declval< hosting_list_t >().begin() ) >;

        iterator_t() = default;

        iterator_t( hosting_list_t * host )
            : m_host{ host }
            , m_chunk_it( host->begin() )
            , m_inside_chunk_pos(
                  m_host->empty() ? 64 : m_chunk_it->find_first_allocated() )
        {
        }

        iterator_t( hosting_list_t * host, chunk_iterator_t chunk_it, int pos )
            : m_host{ host }
            , m_chunk_it( chunk_it )
            , m_inside_chunk_pos( pos )
        {
        }

        iterator_t( const iterator_t & ) = default;
        iterator_t( iterator_t && )      = default;

        iterator_t & operator=( const iterator_t & ) = default;
        iterator_t & operator=( iterator_t && it )   = default;

        iterator_t & operator++() noexcept
        {
            assert( m_chunk_it != m_host->end() );

            m_inside_chunk_pos =
                m_chunk_it->find_next_allocated( m_inside_chunk_pos );

            if( m_inside_chunk_pos < 64 )
            {
                return *this;
            }

            ++m_chunk_it;
            if( m_host->end() != m_chunk_it )
            {
                m_inside_chunk_pos = m_chunk_it->find_first_allocated();
            }

            return *this;
        }

        iterator_t operator++( int ) noexcept
        {
            iterator_t before{ *this };
            ++*this;
            return before;
        }

        iterator_t & operator--()
        {
            if( m_chunk_it != m_host->end() )
            {
                m_inside_chunk_pos =
                    m_chunk_it->find_prev_allocated( m_inside_chunk_pos );

                if( 0 <= m_inside_chunk_pos )
                {
                    return *this;
                }
            }

            assert( m_chunk_it != m_host->begin() );
            --m_chunk_it;
            m_inside_chunk_pos = m_chunk_it->find_last_allocated();

            return *this;
        }

        iterator_t operator--( int ) noexcept
        {
            iterator_t before{ *this };
            --*this;
            return before;
        }

        [[nodiscard]] reference operator*() const
        {
            return m_chunk_it->at( m_inside_chunk_pos );
        }

        [[nodiscard]] reference operator*()
        {
            return m_chunk_it->at( m_inside_chunk_pos );
        }

        [[nodiscard]] pointer operator->() const
        {
            return &( m_chunk_it->at( m_inside_chunk_pos ) );
        }

        [[nodiscard]] pointer operator->()
        {
            return &( m_chunk_it->at( m_inside_chunk_pos ) );
        }

        [[nodiscard]] bool operator==( const iterator_t & other ) const
        {
            return m_host == other.m_host && m_chunk_it == other.m_chunk_it
                   && m_inside_chunk_pos == other.m_inside_chunk_pos;
        }

        [[nodiscard]] bool operator!=( const iterator_t & other ) const
        {
            return !( *this == other );
        }

        bool is_end() const noexcept
        {
            return nullptr == m_host || m_host->end() == m_chunk_it;
        }

    private:
        hosting_list_t * m_host{};
        chunk_iterator_t m_chunk_it;
        int m_inside_chunk_pos;
    };

    using iterator       = iterator_t< false >;
    using const_iterator = iterator_t< true >;

    /**
     * @brief Are there any orders stored in this list?
     */
    [[nodiscard]] bool empty() const noexcept { return m_chunks->empty(); }

    /**
     * @brief Store an order.
     *
     * The new order is appended to the last chunk if the one is appendable
     * or adds a new chunk-node and store the item to it.
     *
     * @return The reference to the inserted order storage.
     */
    [[nodiscard]] iterator push_back( item_t item )
    {
        ++m_items_count;

        if( empty() || !m_chunks->back().can_append_to_chunk() )
        {
            m_chunks->emplace_back();
        }

        chunk_iterator_t chunk_it = m_chunks->end();
        --chunk_it;
        auto & chunk = *chunk_it;

        return iterator{ m_chunks.get(), chunk_it, chunk.append( item ) };
    }

    /**
     * @brief Remove a chunk identified by iterator.
     */
    iterator erase( iterator it )
    {
        assert( m_chunks.get() == it.m_host );

        --m_items_count;

        it.m_chunk_it->erase( it.m_inside_chunk_pos );

        if( it.m_chunk_it->empty() )
        {
            it.m_chunk_it = m_chunks->erase( it.m_chunk_it );
            if( it.m_chunk_it == m_chunks->end() )
            {
                it.m_inside_chunk_pos = 64;
            }
            else
            {
                it.m_inside_chunk_pos = it.m_chunk_it->find_first_allocated();
            }
        }
        else
        {
            ++it;
        }

        return it;
    }

    auto begin() noexcept { return iterator{ m_chunks.get() }; }
    auto end() noexcept { return iterator{ m_chunks.get(), m_chunks->end(), 64 }; }

    auto begin() const noexcept { return const_iterator{ m_chunks.get() }; }
    auto end() const noexcept
    {
        return const_iterator{ m_chunks.get(), m_chunks->end(), 64 };
    }

    std::size_t size() const noexcept { return m_items_count; }

private:
    /**
     * @brief A private list container to store chunks.
     */
    std::unique_ptr< list_impl_t > m_chunks = std::make_unique< list_impl_t >();
    /**
     * @brief Count of elements of type `T` stored in the chunked list.
     *
     * @note `m_chunks.size()` gives us the number of chunks not the elements.
     *       while a single chunk can hold up to 64 elements.
     */
    std::size_t m_items_count{};
};

template < typename T >
chunk_list_t< T >::iterator begin( chunk_list_t< T > & v ) noexcept
{
    return v.begin();
}
template < typename T >
chunk_list_t< T >::iterator end( chunk_list_t< T > & v ) noexcept
{
    return v.end();
}

template < typename T >
chunk_list_t< T >::const_iterator begin( const chunk_list_t< T > & v ) noexcept
{
    return v.begin();
}
template < typename T >
chunk_list_t< T >::const_iterator end( const chunk_list_t< T > & v ) noexcept
{
    return v.end();
}

}  // namespace jacobi::book::utils
