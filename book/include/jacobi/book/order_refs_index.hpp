// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table srtp base-class part
 *       for trade-side orderes table implementation.
 */

#pragma once

#include <type_traits>
#include <unordered_map>

#include <boost/unordered/unordered_flat_map.hpp>

#include <absl/container/flat_hash_map.h>

#include <tsl/robin_map.h>

#include <jacobi/book/vocabulary_types.hpp>

namespace jacobi::book
{

//
// Order_Refs_Index_Concept
//

/**
 * @brief A concept that defines the requirements on
 *        for order-references index.
 *
 * Order references store order location in the book
 * and also additional attributes necessary for handling
 * order updates. The key for lookup assumed to be an order id.
 *
 * For example it may store a reference to a symbol
 * so that the index can be used to identify which book instance must handle it
 * (think of the cases when symbol is not provided for order's events
 * except add: e.g. `delete id=555` or `execute id=123, q=12).
 *
 * TODO: add an example
 */
template < typename Order_Refs_Index >
concept Order_Refs_Index_Concept = requires( Order_Refs_Index & index ) {
    typename Order_Refs_Index::key_t;
    typename Order_Refs_Index::value_t;
    typename Order_Refs_Index::order_reference_t;
    typename Order_Refs_Index::iterator_t;

    {
        index.insert( std::declval< typename Order_Refs_Index::key_t >(),
                      std::declval< typename Order_Refs_Index::value_t >() )
    };

    { index.erase( std::declval< typename Order_Refs_Index::iterator_t >() ) };

    {
        index.find( std::declval< typename Order_Refs_Index::key_t >() )
    } -> std::same_as< typename Order_Refs_Index::iterator_t >;

    { index.end() } -> std::same_as< typename Order_Refs_Index::iterator_t >;

    {
        index.access_value(
            std::declval< typename Order_Refs_Index::iterator_t >() )
    } -> std::same_as< typename Order_Refs_Index::value_t * >;
};

//
// Order_Refs_Index_Value_Concept
//

/**
 * @brief A concept describing the interface
 *        of the value-type of the order-references index.
 */
template < typename Order_Refs_Index_Value, typename Order_Reference >
concept Order_Refs_Index_Value_Concept =
    requires( Order_Refs_Index_Value & value ) {
        { std::as_const( value ).access_order() } -> std::same_as< order_t >;

        { value.access_order_reference() } -> std::same_as< Order_Reference * >;

        { value.set_trade_side( std::declval< trade_side >() ) };
        { std::as_const( value ).get_trade_side() } -> std::same_as< trade_side >;
    };

//
// order_refs_index_std_unordered_map_t
//

/**
 * @brief Order refs index using `std::unordered_map`.
 *
 * Implements Order_Refs_Index_Concept.
 */
template < typename Value, typename Order_Reference >
    requires Order_Refs_Index_Value_Concept< Value, Order_Reference >
struct order_refs_index_std_unordered_map_t
{
    explicit order_refs_index_std_unordered_map_t() = default;

    order_refs_index_std_unordered_map_t(
        const order_refs_index_std_unordered_map_t & ) = delete;
    order_refs_index_std_unordered_map_t(
        order_refs_index_std_unordered_map_t && ) = delete;
    order_refs_index_std_unordered_map_t & operator=(
        const order_refs_index_std_unordered_map_t & ) = delete;
    order_refs_index_std_unordered_map_t & operator=(
        order_refs_index_std_unordered_map_t && ) = delete;

    using key_t             = order_id_t;
    using value_t           = Value;
    using order_reference_t = Order_Reference;

    using index_container_type_t =
        std::unordered_map< key_t, value_t, order_id_custom_hash_t >;

    using iterator_t = index_container_type_t::iterator;

    /**
     * @name Order refs' index interface.
     */
    /// @{
    template < typename M >
    [[nodiscard]] iterator_t insert( key_t key, M && value )
    {
        using vtype_t = typename index_container_type_t::value_type;
        return index.insert( vtype_t{ key, std::forward< M >( value ) } ).first;
    }

    void erase( iterator_t it ) { index.erase( it ); }

    [[nodiscard]] iterator_t find( key_t key ) { return index.find( key ); }

    [[nodiscard]] iterator_t end() { return index.end(); }

    [[nodiscard]] value_t * access_value( iterator_t it )
    {
        return &( it->second );
    }
    /// @}

    index_container_type_t index;
};

//
// order_refs_index_tsl_robin_map_t
//

/**
 * @brief Order refs index using `tsl::robin_map`.
 *
 * Implements Order_Refs_Index_Concept.
 */
template < typename Value, typename Order_Reference >
struct order_refs_index_tsl_robin_map_t
{
    explicit order_refs_index_tsl_robin_map_t() = default;

    order_refs_index_tsl_robin_map_t( const order_refs_index_tsl_robin_map_t & ) =
        delete;
    order_refs_index_tsl_robin_map_t( order_refs_index_tsl_robin_map_t && ) =
        delete;
    order_refs_index_tsl_robin_map_t & operator=(
        const order_refs_index_tsl_robin_map_t & ) = delete;
    order_refs_index_tsl_robin_map_t & operator=(
        order_refs_index_tsl_robin_map_t && ) = delete;

    using key_t             = order_id_t;
    using value_t           = Value;
    using order_reference_t = Order_Reference;

    using index_container_type_t =
        tsl::robin_map< key_t, value_t, order_id_custom_hash_t >;

    using iterator_t = index_container_type_t::iterator;

    /**
     * @name Order refs' index interface.
     */
    /// @{
    template < typename M >
    [[nodiscard]] iterator_t insert( key_t key, M && value )
    {
        using vtype_t = typename index_container_type_t::value_type;
        return index.insert( vtype_t{ key, std::forward< M >( value ) } ).first;
    }

    void erase( iterator_t it ) { index.erase( it ); }

    [[nodiscard]] iterator_t find( key_t key ) { return index.find( key ); }

    [[nodiscard]] iterator_t end() { return index.end(); }

    [[nodiscard]] value_t * access_value( iterator_t it )
    {
        return &( it.value() );
    }
    /// @}

    index_container_type_t index;
};

//
// order_refs_index_boost_unordered_flat_map_t
//

/**
 * @brief Order refs index using `boost::unordered::unordered_flat_map`.
 *
 * Implements Order_Refs_Index_Concept.
 */
template < typename Value, typename Order_Reference >
struct order_refs_index_boost_unordered_flat_map_t
{
    explicit order_refs_index_boost_unordered_flat_map_t() = default;

    order_refs_index_boost_unordered_flat_map_t(
        const order_refs_index_boost_unordered_flat_map_t & ) = delete;
    order_refs_index_boost_unordered_flat_map_t(
        order_refs_index_boost_unordered_flat_map_t && ) = delete;
    order_refs_index_boost_unordered_flat_map_t & operator=(
        const order_refs_index_boost_unordered_flat_map_t & ) = delete;
    order_refs_index_boost_unordered_flat_map_t & operator=(
        order_refs_index_boost_unordered_flat_map_t && ) = delete;

    using key_t             = order_id_t;
    using value_t           = Value;
    using order_reference_t = Order_Reference;

    using index_container_type_t = boost::unordered::
        unordered_flat_map< key_t, value_t, order_id_custom_hash_t >;

    using iterator_t = index_container_type_t::iterator;

    /**
     * @name Order refs' index interface.
     */
    /// @{
    template < typename M >
    [[nodiscard]] iterator_t insert( key_t key, M && value )
    {
        using vtype_t = typename index_container_type_t::value_type;
        return index.insert( vtype_t{ key, std::forward< M >( value ) } ).first;
    }

    void erase( iterator_t it ) { index.erase( it ); }

    [[nodiscard]] iterator_t find( key_t key ) { return index.find( key ); }

    [[nodiscard]] iterator_t end() { return index.end(); }

    [[nodiscard]] value_t * access_value( iterator_t it )
    {
        return &( it->second );
    }
    /// @}

    index_container_type_t index;
};

//
// order_refs_index_absl_flat_hash_map_t
//

/**
 * @brief Order refs index using `boost::unordered::unordered_flat_map`.
 *
 * Implements Order_Refs_Index_Concept.
 */
template < typename Value, typename Order_Reference >
struct order_refs_index_absl_flat_hash_map_t
{
    explicit order_refs_index_absl_flat_hash_map_t() = default;

    order_refs_index_absl_flat_hash_map_t(
        const order_refs_index_absl_flat_hash_map_t & ) = delete;
    order_refs_index_absl_flat_hash_map_t(
        order_refs_index_absl_flat_hash_map_t && ) = delete;
    order_refs_index_absl_flat_hash_map_t & operator=(
        const order_refs_index_absl_flat_hash_map_t & ) = delete;
    order_refs_index_absl_flat_hash_map_t & operator=(
        order_refs_index_absl_flat_hash_map_t && ) = delete;

    using key_t             = order_id_t;
    using value_t           = Value;
    using order_reference_t = Order_Reference;

    using index_container_type_t =
        absl::flat_hash_map< key_t, value_t, order_id_custom_hash_t >;

    using iterator_t = index_container_type_t::iterator;

    /**
     * @name Order refs' index interface.
     */
    /// @{
    template < typename M >
    [[nodiscard]] iterator_t insert( key_t key, M && value )
    {
        using vtype_t = typename index_container_type_t::value_type;
        return index.insert( vtype_t{ key, std::forward< M >( value ) } ).first;
    }

    void erase( iterator_t it ) { index.erase( it ); }

    [[nodiscard]] iterator_t find( key_t key ) { return index.find( key ); }

    [[nodiscard]] iterator_t end() { return index.end(); }

    [[nodiscard]] value_t * access_value( iterator_t it )
    {
        return &( it->second );
    }
    /// @}

    index_container_type_t index;
};

}  // namespace jacobi::book
