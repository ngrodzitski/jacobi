// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table CRTP base-class part
 *       for trade-side orders table implementation.
 */

#pragma once

#include <type_traits>
#include <list>
#include <map>

#include <plf_list.h>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level_fwd.hpp>
#include <jacobi/book/order_refs_index.hpp>

namespace jacobi::book
{

//
// book_impl_data_t
//

template < typename Book_Impl_Data >
concept Book_Impl_Data_Concept = requires( Book_Impl_Data & data ) {
    typename Book_Impl_Data::order_refs_index_t;
    typename Book_Impl_Data::price_levels_factory_t;

    data.order_refs_index;
    data.price_levels_factory;

    requires std::same_as<
        std::remove_cvref_t< decltype( data.order_refs_index ) >,
        typename Book_Impl_Data::order_refs_index_t >;
    requires std::same_as<
        std::remove_cvref_t< decltype( data.price_levels_factory ) >,
        typename Book_Impl_Data::price_levels_factory_t >;
};

//
// book_impl_data_t
//

/**
 * @brief Book internals data that is shared by trade-side's tables
 */
template < Order_Refs_Index_Concept Order_Refs_Index,
           Price_Levels_Factory_Concept Price_Levels_Factory >
struct book_impl_data_t
{
    using order_refs_index_t     = Order_Refs_Index;
    using price_levels_factory_t = Price_Levels_Factory;

    order_refs_index_t order_refs_index;
    price_levels_factory_t price_levels_factory;
};

//
// orders_table_t
//

/**
 * @brief Represents A table of orders for a single trade side.
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
template < typename Derived,
           Book_Impl_Data_Concept Book_Impl_Data,
           trade_side Side_Indicator >
class orders_table_crtp_base_t
{
    /**
     * @name CRTP derived type accessors.
     */
    /// @{
    [[nodiscard]] Derived & derived() noexcept
    {
        return static_cast< Derived & >( *this );
    }
    [[nodiscard]] const Derived & derived() const noexcept
    {
        return static_cast< const Derived & >( *this );
    }
    /// @}

public:
    using book_private_data_t = Book_Impl_Data;
    using order_refs_index_t  = typename book_private_data_t::order_refs_index_t;
    using price_levels_factory_t =
        typename book_private_data_t::price_levels_factory_t;
    using price_level_t = price_levels_factory_t::price_level_t;

    explicit orders_table_crtp_base_t( book_private_data_t & data )
        : m_book_private_data{ data }
    {
    }

    // We not intend any copy/move for this class.
    orders_table_crtp_base_t( const orders_table_crtp_base_t & ) = delete;
    orders_table_crtp_base_t( orders_table_crtp_base_t && )      = delete;
    orders_table_crtp_base_t & operator=( const orders_table_crtp_base_t & ) =
        delete;
    orders_table_crtp_base_t & operator=( orders_table_crtp_base_t && ) = delete;

    /**
     * @name Order manipulation API.
     */
    /// @{

    /**
     * @brief Add new order to this table.
     *
     * @pre Order MUST NOT exist in the table.
     *
     * @note Depending on the hash-table implementation
     *       Iterator returnd by this function
     *       to the entry in Order References Table created for a given order
     *       might be invalidated upon the next rehash
     *       (that might be caused by the next insertion).
     */
    auto add_order( order_t order )
    {
        assert( order.qty > order_qty_t{ 0 } );
        // No such order should exist.
        assert( m_book_private_data.order_refs_index.end()
                == m_book_private_data.order_refs_index.find( order.id ) );

        auto res = m_book_private_data.order_refs_index.insert(
            order.id,
            derived().level_at( order.price ).first->add_order( order ) );

        return res;
    }

    /**
     * @brief Delete an order with a given id.
     *
     * @pre Order MUST exist in the table.
     */
    void delete_order( order_id_t id )
    {
        auto it = m_book_private_data.order_refs_index.find( id );
        assert( it != m_book_private_data.order_refs_index.end() );
        this->delete_order( it );
    }

    /**
     * @brief Perform execute action.
     *
     * @pre @c id MUST exist and MUST refer the first order in the table
     *      (first order at the top price).
     */
    void execute_order( order_id_t id, order_qty_t exec_qty )
    {
        assert( exec_qty > order_qty_t{ 0 } );
        auto it = m_book_private_data.order_refs_index.find( id );
        if( it != m_book_private_data.order_refs_index.end() ) [[likely]]
        {
            this->execute_order( it, exec_qty );
        }
    }

    /**
     * @brief Reduce quantity of the order.
     *
     * @pre Order MUST exist in the table.
     */
    void reduce_order( order_id_t id, order_qty_t canceled_qty )
    {
        assert( canceled_qty > order_qty_t{ 0 } );
        auto it = m_book_private_data.order_refs_index.find( id );

        assert( it != m_book_private_data.order_refs_index.end() );

        this->reduce_order( it, canceled_qty );
    }

    /**
     * @brief Modify order.
     *
     * @pre Order MUST exist in the table.
     */
    void modify_order( order_t modified_order )
    {
        assert( modified_order.qty > order_qty_t{ 0 } );

        auto it = m_book_private_data.order_refs_index.find( modified_order.id );
        assert( it != m_book_private_data.order_refs_index.end() );

        modify_order( it, modified_order );
    }
    /// @}

    /**
     * @name Order manipulation API semi-private routines.
     *
     * This must functions are intended to be used with Book implementation
     * classes.
     */
    /// @{
    void delete_order( order_refs_index_t::iterator_t it )
    {
        const auto & ref =
            *( m_book_private_data.order_refs_index.access_value( it )
                   ->access_order_reference() );

        auto [ lvl, lvl_ref ] = derived().level_at( ref.price() );
        lvl->delete_order( ref );

        if( 0ULL == lvl->orders_count() )
        {
            // To retire the level we use reference
            // to avoid additional lookup.
            derived().retire_level( lvl_ref );
        }

        // Remove order from index.
        m_book_private_data.order_refs_index.erase( it );
    }

    void execute_order( order_refs_index_t::iterator_t it, order_qty_t exec_qty )
    {
        const auto order = m_book_private_data.order_refs_index.access_value( it )
                               ->access_order();

        assert( !derived().empty() );
        assert( order.price == derived().top_price().value() );

        assert( order.qty >= exec_qty );

        if( order.qty == exec_qty )
        {
            this->delete_order( it );
        }
        else
        {
            auto & ref =
                *( m_book_private_data.order_refs_index.access_value( it )
                       ->access_order_reference() );

            ref = derived().top_level().reduce_qty( ref, exec_qty );
        }
    }

    void reduce_order( order_refs_index_t::iterator_t it,
                       order_qty_t canceled_qty )
    {
        const auto order = m_book_private_data.order_refs_index.access_value( it )
                               ->access_order();

        assert( order.qty > canceled_qty );

        auto & ref = *( m_book_private_data.order_refs_index.access_value( it )
                            ->access_order_reference() );

        ref = derived()
                  .level_at( order.price )
                  .first->reduce_qty( ref, canceled_qty );
    }

    void modify_order( order_refs_index_t::iterator_t it, order_t modified_order )
    {
        auto & ref = *( m_book_private_data.order_refs_index.access_value( it )
                            ->access_order_reference() );

        const auto old_order =
            m_book_private_data.order_refs_index.access_value( it )
                ->access_order();

        assert( old_order.id == modified_order.id );

        if( old_order.price == modified_order.price )
        {
            auto * old_lvl = derived().level_at( old_order.price ).first;
            old_lvl->delete_order( ref );
            ref.copy_from( old_lvl->add_order( modified_order ) );
        }
        else
        {
            // We must first ask for new level,
            // Because it might trigger storage change (reallocation)
            // which might invalidate old_lvl_ref part
            // (which for linear storage is vector::iterator).
            // So we ask for new level first thus the storage
            // would insert a new level if necessary
            // and then we ask for old level which cannot change
            // the storage of levels because the order is still located on it.
            auto * new_lvl = derived().level_at( modified_order.price ).first;

            auto [ old_lvl, old_lvl_ref ] = derived().level_at( old_order.price );
            old_lvl->delete_order( ref );
            ref.copy_from( new_lvl->add_order( modified_order ) );

            // If the old level becomes empty we retire it.
            if( 0ULL == old_lvl->orders_count() )
            {
                derived().retire_level( old_lvl_ref );
                // Note after the above line
                // new_lvl might be invalid
                // because retiring might mean storage change
            }
        }
    }
    /// @}

    /**
     * @brief A reference to book internals (shared with oposite trade side).
     */
    book_private_data_t & m_book_private_data;
};

}  // namespace jacobi::book
