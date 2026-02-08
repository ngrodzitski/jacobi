// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Price level common routines.
 */

#pragma once

#include <type_traits>

#include <jacobi/book/vocabulary_types.hpp>

namespace jacobi::book
{

//
// List_Traits_Concept
//

/**
 * @brief A concept that defines what book traits must guarantee.
 */
template < typename List_Traits >
concept List_Traits_Concept =
    requires { typename List_Traits::template list_t< order_t >; };

//
// std_list_traits_t
//

struct std_list_traits_t
{
    template < typename T >
    using list_t = std::list< T >;
};

//
// plf_list_traits_t
//

struct plf_list_traits_t
{
    template < typename T >
    using list_t = plf::list< T >;
};

//
// Price_Level_Order_Reference_Concept
//

template < typename Order_Reference >
concept Price_Level_Order_Reference_Concept = requires( Order_Reference ref ) {
    { std::as_const( ref ).price() } -> std::same_as< order_price_t >;
    { std::as_const( ref ).make_order() } -> std::same_as< order_t >;
};

//
// Price_Level_Concept
//

/**
 * @brief A interface requirements on Price Level routine implementation.
 */
template < typename Price_Level >
concept Price_Level_Concept = requires( Price_Level & plvl ) {
    // Must define a reference,
    // through which the level is capable of locating the order.
    typename Price_Level::reference_t;

    /**
     * @name Price level API.
     */
    // @{
    { std::as_const( plvl ).price() } -> std::same_as< order_price_t >;

    { std::as_const( plvl ).orders_count() } -> std::convertible_to< std::size_t >;

    { std::as_const( plvl ).orders_qty() } -> std::same_as< order_qty_t >;

    { std::as_const( plvl ).empty() } -> std::convertible_to< bool >;

    {
        plvl.add_order( std::declval< order_t >() )
    } -> std::same_as< typename Price_Level::reference_t >;

    { plvl.delete_order( std::declval< typename Price_Level::reference_t >() ) };

    {
        plvl.reduce_qty( std::declval< typename Price_Level::reference_t >(),
                         std::declval< order_qty_t >() )
    } -> std::same_as< typename Price_Level::reference_t >;

    { std::as_const( plvl ).orders_range() } -> std::ranges::range;

    { std::as_const( plvl ).orders_range_reverse() } -> std::ranges::range;

    // @}
};

//
// Price_Levels_Factory_Concept
//

/**
 * @brief A concept that defines what BSN counter API is like .
 */
template < typename Price_Levels_Factory >
concept Price_Levels_Factory_Concept = requires( Price_Levels_Factory & factory ) {
    // Must define price level type.
    typename Price_Levels_Factory::price_level_t;
    requires Price_Level_Concept< typename Price_Levels_Factory::price_level_t >;

    // Must provide a factory function.
    // That is how we create price-level instances.
    {
        factory.make_price_level( std::declval< order_price_t >() )
    } -> std::same_as< typename Price_Levels_Factory::price_level_t >;

    // Must provide a retire function.
    // That is how we leave an option to enable reuse of price levels.
    {
        factory.retire_price_level(
            std::declval< typename Price_Levels_Factory::price_level_t && >() )
    };
};

//
// list_based_price_level_order_reference_t
//

/**
 * @brief Reference agregate to locate the order exactly.
 */
template < typename Iterator >
class list_based_price_level_order_reference_t
{
public:
    explicit list_based_price_level_order_reference_t() = default;

    explicit list_based_price_level_order_reference_t( order_price_t p,
                                                       Iterator it )
        : m_price{ p }
        , m_order_it{ it }
    {
    }

    [[nodiscard]] order_price_t price() const noexcept { return m_price; }

    [[nodiscard]] order_t make_order() const noexcept { return *m_order_it; }

    [[nodiscard]] order_t & mutable_order() const noexcept { return *m_order_it; }

    [[nodiscard]] Iterator iterator() const noexcept { return m_order_it; }

    /**

     * @brief Copy reference from another instance.
     */
    void copy_from( const list_based_price_level_order_reference_t & ref )
    {
        m_price    = ref.m_price;
        m_order_it = ref.m_order_it;
    }

private:
    order_price_t m_price;
    Iterator m_order_it;
};

//
// trivial_price_levels_factory_t
//

template < typename Price_Level >
class trivial_price_levels_factory_t
{
public:
    using price_level_t = Price_Level;

    [[nodiscard]] price_level_t make_price_level( order_price_t p ) const
    {
        return price_level_t{ p };
    }

    void retire_price_level( [[maybe_unused]] price_level_t && price_level )
    {
        // Do nothing
    }
};

}  // namespace jacobi::book
