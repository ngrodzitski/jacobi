// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Nodes pool.
 */

#pragma once

#include <array>
#include <list>

#include <boost/intrusive/list.hpp>

namespace jacobi::book::utils
{

//
// intrusive_nodes_page_t
//

/**
 * @brief A collection of preallocated nodes.
 */
template < typename Node, std::size_t Nodes_Count >
struct alignas( 64 ) intrusive_nodes_page_t
{
    std::array< Node, Nodes_Count > nodes;
};

//
// intrusive_nodes_pool_t
//

/**
 * @brief A pool of for intrusive_node_t objects.
 */
template < typename Node,
           typename Intrusive_List_Member,
           std::size_t Page_Size = 1024 >
class intrusive_nodes_pool_t
{
public:
    using intrusive_node_t        = Node;
    using intrusive_list_member_t = Intrusive_List_Member;

    explicit intrusive_nodes_pool_t() = default;

    explicit intrusive_nodes_pool_t( std::size_t initial_page_count )
    {
        while( initial_page_count-- > 0 )
        {
            add_page();
        }
    }
    ~intrusive_nodes_pool_t() { m_free_nodes.clear(); }

    // Copy is not allowed.
    intrusive_nodes_pool_t( const intrusive_nodes_pool_t & )             = delete;
    intrusive_nodes_pool_t & operator=( const intrusive_nodes_pool_t & ) = delete;

    intrusive_nodes_pool_t( intrusive_nodes_pool_t && )             = default;
    intrusive_nodes_pool_t & operator=( intrusive_nodes_pool_t && ) = default;

    [[nodiscard]] intrusive_node_t * allocate()
    {
        if( m_free_nodes.empty() ) [[unlikely]]
        {
            add_page();
        }

        assert( !m_free_nodes.empty() );

        intrusive_node_t * node = &( m_free_nodes.front() );
        m_free_nodes.pop_front();

        return node;
    }

    void deallocate( intrusive_node_t * node ) noexcept
    {
        m_free_nodes.push_front( *node );
    }

    [[nodiscard]] std::size_t free_nodes_count() const noexcept
    {
        return m_free_nodes.size();
    }

    [[nodiscard]] std::size_t allocated_pages_count() const noexcept
    {
        return m_pages.size();
    }

private:
    using nodes_page_t = intrusive_nodes_page_t< intrusive_node_t, Page_Size >;
    void add_page()
    {
        m_pages.emplace_back( nodes_page_t{} );
        for( auto & n : m_pages.back().nodes )
        {
            m_free_nodes.push_back( n );
        }
    }

    using intrusive_list_t =
        boost::intrusive::list< intrusive_node_t, intrusive_list_member_t >;

    intrusive_list_t m_free_nodes;
    std::list< nodes_page_t > m_pages;
};

}  // namespace jacobi::book::utils
