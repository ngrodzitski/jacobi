// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table implementation based on
 *       linear (vector) ([diff(price, base_price )]=>price_level) implementation.
 */

#pragma once

#include <type_traits>
#include <vector>

#include <range/v3/view/reverse.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/subrange.hpp>

#include <jacobi/book/orders_table_crtp_base.hpp>
#include <jacobi/book/vocabulary_types.hpp>

namespace jacobi::book::linear
{

inline namespace v1
{

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
 *     * vector storage starts ([0] element) with the price level
 *       for price0 which is the level that is most far away from the oposite side.
 *     * At end of the vector ([N-1] element N is the size of the vector)
 *       the top level is stored, so that the levels that experience most of the
 *       events (those one's that are close to BBO) are at the end of the storage.
 *     * The above means that is a new level(s) must be added it means appending
 *       at the end of the vector. And if some levels at the end becomes empty
 *       it means we need to pop some elements at the end.
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
        return m_price_levels.back().price();
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
        return m_price_levels.back().orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        return m_price_levels | ranges::views::reverse
               | ranges::views::filter( []( const auto & lvl )
                                        { return !lvl.empty(); } );
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
        return top_level().first_order();
    }

private:
    inline static constexpr unsigned overprovision_count_unit = 16;
    inline static constexpr unsigned default_storage_capacity =
        4 * overprovision_count_unit;

    /**
     * @brief A type alias for prices' levels' storage.
     */
    using levels_table_t = std::vector< price_level_t >;

    [[nodiscard]] static levels_table_t make_initial_storage()
    {
        levels_table_t res;
        res.reserve( default_storage_capacity );
        return res;
    }

    /**
     * @brief Trade side biased price comparator set.
     */
    using price_ops_t = order_price_operations_t< Side_Indicator >;

    /**
     * @brief Get the index in the levels storage associated with a given price.
     */
    [[nodiscard]] std::size_t make_storage_index( order_price_t p ) const noexcept
    {
        assert( price_ops_t{}.le( p, m_price0 ) );

        const auto dist = price_ops_t{}.distance( p, m_price0 );

        assert( dist >= order_price_t{ 0 } );

        return static_cast< std::size_t >( type_safe::get( dist ) );
    }

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, typename levels_table_t::iterator >
    level_at( order_price_t price )
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbitwise-instead-of-logical"
        const bool is_new_base_price =
            price_ops_t{}.lt( m_price0, price ) | m_price_levels.empty();
#pragma clang diagnostic pop

        if( is_new_base_price ) [[unlikely]]
        {
            // Here: we must handle new base price.
            if( m_price_levels.empty() ) [[unlikely]]
            {
                // We are on empty book.
                // Let's make a nice reserve around
                // first price that appears within orders table.
                //
                // We will set the base price to 2*overprovision_count_unit
                // backward and reserve the vector to be 4*overprovision_count_unit
                // the level for this exact price would be created
                // later as a result of handling
                m_price0 = price_ops_t{}.advance_backward(
                    price, order_price_t{ 2 * overprovision_count_unit } );
                m_price_levels.emplace_back(
                    this->m_book_private_data.price_levels_factory
                        .make_price_level( m_price0 ) );
            }
            else
            {
                // Add some more, so we unlikely to do it often
                // even if the price goes in one direction
                // constantly and pushing orders to be created
                // below the base price we have.
                const auto new_price0 = price_ops_t{}.advance_backward(
                    price, order_price_t{ overprovision_count_unit } );

                const auto levels_to_add =
                    static_cast< std::size_t >( type_safe::get(
                        price_ops_t{}.distance( m_price0, new_price0 ) ) );

                // We will create a new vector and add first N=levels_to_add
                // created by price_levels_factory
                // and then we will move currently stored levels into new array.

                levels_table_t new_levels_table;
                new_levels_table.reserve( std::max< std::size_t >(
                    levels_to_add
                        + std::min(
                            m_price_levels.capacity(),
                            m_price_levels.size() + overprovision_count_unit ),
                    default_storage_capacity ) );

                for( auto price_val = new_price0;
                     price_ops_t{}.lt( m_price0, price_val );
                     price_val = price_ops_t{}.advance_forward( price_val ) )
                {
                    new_levels_table.emplace_back(
                        this->m_book_private_data.price_levels_factory
                            .make_price_level( price_val ) );
                }

                std::move( begin( m_price_levels ),
                           end( m_price_levels ),
                           std::back_inserter( new_levels_table ) );

                swap( m_price_levels, new_levels_table );
                m_price0 = new_price0;
            }
        }

        if( auto trade_side_biased_min_price_available =
                m_price_levels.back().price();
            price_ops_t{}.lt( price, trade_side_biased_min_price_available ) )
        {
            while(
                price_ops_t{}.lt( price, trade_side_biased_min_price_available ) )
            {
                trade_side_biased_min_price_available =
                    price_ops_t{}.advance_forward(
                        trade_side_biased_min_price_available );
                m_price_levels.emplace_back(
                    this->m_book_private_data.price_levels_factory
                        .make_price_level(
                            trade_side_biased_min_price_available ) );
            }
        }

        const auto ix = make_storage_index( price );
        assert( ix < m_price_levels.size() );

        auto it = m_price_levels.begin() + ix;

        assert( it->price() == price );
        return std::make_pair( &( *it ), it );
    }

    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        assert( !this->empty() );
        return m_price_levels.back();
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( [[maybe_unused]] levels_table_t::iterator lvl_it )
    {
        // We simply check empty levels at the end of the storage.
        while( !m_price_levels.empty() && m_price_levels.back().empty() )
        {
            m_price_levels.pop_back();
        }
    }

    /**
     * @brief A storage for price levels.
     *
     * Each levels contain a queue of orders at a given price.
     */
    levels_table_t m_price_levels = make_initial_storage();

    /**
     * @brief A base price from which the levels start.
     */
    order_price_t m_price0{};
};

}  // namespace v1

namespace v2
{

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
 * @note Current implementation suggests that storage can only grow.
 *
 * Implements the following strategy:
 *     * vector storage starts ([0] element) with the price level
 *       for price0 which is the level that is most far away from the oposite side.
 *     * vector storage ends with the price level that is most closer to the
 * oposite side (e.g. lower-offer prices and higher bid-prices)
 *     * When trade side experiences a move forward (a new levels closer to the
 *       oposite side must be added) - new levels are pushed back.
 *     * When trade side experiences a move backward (a new levels further away
 *       from the oposite side must be added) they are inserted in the
 *       beginning of the vector storage. A little overprovision is considered
 *       so we are not hit by series of moving backwards.
 *     * If current top price level becomes empty we do not pop elements
 *       from the back but search thenext top price.
 *     * The top price is stored as a separate veriable.
 *     * if the top price is worse then storage base price it means
 *       the table is empty.
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

    // Reuse base constructors.
    using base_type_t::base_type_t;

    [[nodiscard]] bool empty() const noexcept
    {
        return price_ops_t{}.lt( m_price0, m_top_price );
    }

    /**
     * @brief Get top price.
     */
    [[nodiscard]] std::optional< order_price_t > top_price() const noexcept
    {
        if( empty() )
        {
            return std::nullopt;
        }
        return m_top_price;
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
        return m_price_levels[ make_storage_index( m_top_price ) ].orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        auto storage_rng = [ this ]
        {
            auto b = m_price_levels.begin();
            auto e = m_price_levels.begin();
            if( !empty() ) [[unlikely]]
            {
                e += make_storage_index( m_top_price ) + 1;
            }
            return ranges::subrange( b, e );
        }();

        return storage_rng | ranges::views::reverse
               | ranges::views::filter( []( const auto & lvl )
                                        { return !lvl.empty(); } );
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
        return top_level().first_order();
    }

private:
    inline static constexpr unsigned overprovision_count_unit = 16;
    inline static constexpr unsigned default_storage_capacity =
        4 * overprovision_count_unit;

    /**
     * @brief A type alias for prices' levels' storage.
     */
    using levels_table_t = std::vector< price_level_t >;

    [[nodiscard]] static levels_table_t make_initial_storage()
    {
        levels_table_t res;
        res.reserve( default_storage_capacity );
        return res;
    }

    /**
     * @brief Trade side biased price comparator set.
     */
    using price_ops_t = order_price_operations_t< Side_Indicator >;

    /**
     * @brief Get the index in the levels storage associated with a given price.
     */
    [[nodiscard]] std::size_t make_storage_index( order_price_t p ) const noexcept
    {
        assert( price_ops_t{}.le( p, m_price0 ) );

        const auto dist = price_ops_t{}.distance( p, m_price0 );

        assert( dist >= order_price_t{ 0 } );

        return static_cast< std::size_t >( type_safe::get( dist ) );
    }

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, typename levels_table_t::iterator >
    level_at( order_price_t price )
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbitwise-instead-of-logical"
        const bool is_new_base_price =
            price_ops_t{}.lt( m_price0, price ) | m_price_levels.empty();
#pragma clang diagnostic pop
        if( is_new_base_price ) [[unlikely]]
        {
            // Here: we must handle new base price.
            if( m_price_levels.empty() ) [[unlikely]]
            {
                // We are on empty book.
                // Let's make a nice reserve around
                // first price that appears within orders table.
                //
                // We will set the base price to 2*overprovision_count_unit
                // backward and reserve the vector to be 4*overprovision_count_unit
                // the level for this exact price would be created
                // later as a result of handling
                m_price0 = price_ops_t{}.advance_backward(
                    price, order_price_t{ 2 * overprovision_count_unit } );

                // We also Init top price as well:
                m_top_price = m_price0;

                m_price_levels.emplace_back(
                    this->m_book_private_data.price_levels_factory
                        .make_price_level( m_price0 ) );
            }
            else
            {
                // Add some more, so we unlikely to do it often
                // even if the price goes in one direction
                // constantly and pushing orders to be created
                // below the base price we have.
                const auto new_price0 = price_ops_t{}.advance_backward(
                    price, order_price_t{ overprovision_count_unit } );

                const auto levels_to_add =
                    static_cast< std::size_t >( type_safe::get(
                        price_ops_t{}.distance( m_price0, new_price0 ) ) );

                // We will create a new vector and add first N=levels_to_add
                // created by price_levels_factory
                // and then we will move currently stored levels into new array.

                levels_table_t new_levels_table;
                new_levels_table.reserve( std::max< std::size_t >(
                    levels_to_add
                        + std::min(
                            m_price_levels.capacity(),
                            m_price_levels.size() + overprovision_count_unit ),
                    default_storage_capacity ) );

                for( auto price_val = new_price0;
                     price_ops_t{}.lt( m_price0, price_val );
                     price_val = price_ops_t{}.advance_forward( price_val ) )
                {
                    new_levels_table.emplace_back(
                        this->m_book_private_data.price_levels_factory
                            .make_price_level( price_val ) );
                }

                std::move( begin( m_price_levels ),
                           end( m_price_levels ),
                           std::back_inserter( new_levels_table ) );

                swap( m_price_levels, new_levels_table );
                m_price0 = new_price0;
            }
        }

        if( auto trade_side_biased_min_price_available =
                m_price_levels.back().price();
            price_ops_t{}.lt( price, trade_side_biased_min_price_available ) )
        {
            while(
                price_ops_t{}.lt( price, trade_side_biased_min_price_available ) )
            {
                trade_side_biased_min_price_available =
                    price_ops_t{}.advance_forward(
                        trade_side_biased_min_price_available );
                m_price_levels.emplace_back(
                    this->m_book_private_data.price_levels_factory
                        .make_price_level(
                            trade_side_biased_min_price_available ) );
            }
        }

        m_top_price   = price_ops_t{}.min( m_top_price, price );
        const auto ix = make_storage_index( price );
        assert( ix < m_price_levels.size() );

        auto it = m_price_levels.begin() + ix;

        assert( it->price() == price );
        return std::make_pair( &( *it ), it );
    }

    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        assert( !this->empty() );
        return m_price_levels[ make_storage_index( m_top_price ) ];
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( levels_table_t::iterator lvl_it )
    {
        assert( price_ops_t{}.le( m_top_price, lvl_it->price() ) );

        if( m_top_price == lvl_it->price() )
        {
            // Step back the top price by one minimum.
            m_top_price = price_ops_t{}.advance_backward( m_top_price );

            // Reverse(lvl_it) wil point to the prev element
            // of the element that lvl_it is pointing,
            // Which is what we want because we already "inspected"
            // and since it is retired - we know it is empty.
            for( auto rev_it = std::make_reverse_iterator( lvl_it );
                 rev_it != m_price_levels.rend() && rev_it->empty();
                 ++rev_it )
            {
                m_top_price = price_ops_t{}.advance_backward( m_top_price );
            }

            if( empty() ) [[unlikely]]
            {
                // Reset the storage.
                m_price_levels = make_initial_storage();
            }
        }
    }

    /**
     * @brief A storage for price levels.
     *
     * Each levels contain a queue of orders at a given price.
     */
    levels_table_t m_price_levels = make_initial_storage();

    /**
     * @brief A base price from which the levels start.
     */
    order_price_t m_price0{};

    /**
     * @brief Current top price.
     */
    order_price_t m_top_price = price_ops_t{}.advance_backward( m_price0 );
};

}  // namespace v2

namespace v3
{

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
 *     * Only non-empty levels are stored in the storage.
 *     * Elements in a vector (storage) ordered as necessary for a given trade side
 *       and accessed via binary search.
 *     * If a new level must be inserted it is inserted with keeping the order.
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
        return m_price_levels.back().price();
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
        return m_price_levels.back().orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        return m_price_levels | ranges::views::reverse;
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
        return top_level().first_order();
    }

private:
    inline static constexpr unsigned overprovision_count_unit = 16;
    inline static constexpr unsigned default_storage_capacity =
        4 * overprovision_count_unit;

    /**
     * @brief A type alias for prices' levels' storage.
     */
    using levels_table_t = std::vector< price_level_t >;

    [[nodiscard]] static levels_table_t make_initial_storage()
    {
        levels_table_t res;
        res.reserve( default_storage_capacity );
        return res;
    }

    /**
     * @brief Trade side biased price comparator set.
     */
    using price_ops_t = order_price_operations_t< Side_Indicator >;

    /**
     * @brief Get the index in the levels storage associated with a given price.
     */
    [[nodiscard]] levels_table_t::iterator find_price_level(
        order_price_t p ) noexcept
    {
        auto it = std::lower_bound( begin( m_price_levels ),
                                    end( m_price_levels ),
                                    p,
                                    []( const auto & a, auto p )
                                    { return price_ops_t{}.lt( p, a.price() ); } );
        assert( end( m_price_levels ) == it
                || price_ops_t{}.le( it->price(), p ) );

        return it;
    }

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, typename levels_table_t::iterator >
    level_at( order_price_t price )
    {
        auto it = find_price_level( price );

        if( end( m_price_levels ) == it )
        {
            m_price_levels.emplace_back(
                this->m_book_private_data.price_levels_factory.make_price_level(
                    price ) );

            return std::make_pair( &m_price_levels.back(),
                                   end( m_price_levels ) - 1 );
        }

        if( it->price() != price )
        {
            it = m_price_levels.insert(
                it,
                this->m_book_private_data.price_levels_factory.make_price_level(
                    price ) );
        }

        return std::make_pair( &( *it ), it );
    }

    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        assert( !this->empty() );
        return m_price_levels.back();
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( levels_table_t::iterator lvl_it )
    {
        m_price_levels.erase( lvl_it );
    }

    /**
     * @brief A storage for price levels.
     *
     * Each levels contain a queue of orders at a given price.
     */
    levels_table_t m_price_levels = make_initial_storage();
};

}  // namespace v3

}  // namespace jacobi::book::linear
