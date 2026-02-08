// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Hot/Cold storage book init params adaptations.
 */

#pragma once

#include <jacobi/book/book.hpp>
#include <jacobi/book/mixed/hot_cold/orders_table.hpp>

namespace jacobi::book::mixed::hot_cold
{

//
// custom_params_t
//

template < Book_Traits_Concept Book_Traits >
struct std_book_init_params_t
    : public ::jacobi::book::std_book_init_params_t< Book_Traits >
{
    explicit std_book_init_params_t(
        std::size_t init_hot_storage_size = details::default_hot_levels_count )
        : m_hot_storage_size{ init_hot_storage_size }
    {
    }

    [[nodiscard]] std::size_t hot_storage_size() const noexcept
    {
        return m_hot_storage_size;
    }

    using base_type_t = ::jacobi::book::std_book_init_params_t< Book_Traits >;

    using typename base_type_t::sell_orders_table_t;
    using typename base_type_t::buy_orders_table_t;
    using typename base_type_t::impl_data_t;

    // Overload factories for trade sides to
    // use a hot storage of size 8.
    [[nodiscard]] sell_orders_table_t sell_orders_table(
        impl_data_t & impl_data ) const
    {
        return sell_orders_table_t{ m_hot_storage_size, impl_data };
    }

    [[nodiscard]] buy_orders_table_t buy_orders_table(
        impl_data_t & impl_data ) const
    {
        return buy_orders_table_t{ m_hot_storage_size, impl_data };
    }

private:
    std::size_t m_hot_storage_size;
};

}  // namespace jacobi::book::mixed::hot_cold
