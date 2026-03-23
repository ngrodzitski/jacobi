// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Price level routines.
 */

#pragma once

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
 * @brief Reference aggregate to locate the order exactly.
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
    void copy_from( const soa_price_level_order_reference_t & ref ) noexcept
    {
        m_order = ref.m_order;
        m_pos   = ref.m_pos;
    }

    [[nodiscard]] Node_Position_Type pos() const noexcept { return m_pos; }

private:
    order_t m_order;
    Node_Position_Type m_pos;
};

}  // namespace jacobi::book
