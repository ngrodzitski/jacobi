// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Defines Order Book.
 */

#pragma once

#include <memory>
#include <optional>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <range/v3/view/take.hpp>
#include <range/v3/view/reverse.hpp>

#include <jacobi/book/vocabulary_types.hpp>

namespace jacobi::book
{

//
// BSN_Counter_Concept
//

/**
 * @brief A concept that defines what BSN counter API is like .
 */
template < typename T >
concept BSN_Counter_Concept = requires( T & counter ) {
    { counter.inc() };
    { std::as_const( counter ).value() } -> std::same_as< bsn_t >;
};

//
// std_bsn_counter_t
//

/**
 * @brief Standard bsn counter.
 */
class std_bsn_counter_t
{
public:
    void inc() noexcept { ++m_bsn; }
    [[nodiscard]] bsn_t value() const noexcept { return m_bsn; }

private:
    bsn_t m_bsn{};
};

static_assert( BSN_Counter_Concept< std_bsn_counter_t > );

//
// void_bsn_counter_t
//

/**
 * @brief Void bsn counter.
 */
struct void_bsn_counter_t
{
    constexpr void inc() const noexcept {}
    [[nodiscard]] constexpr bsn_t value() const noexcept { return bsn_t{ 0 }; }
};

static_assert( BSN_Counter_Concept< void_bsn_counter_t > );

//
// Book_Traits_Concept
//

/**
 * @brief A concept that defines what book traits must guarantee.
 */
template < typename Book_Traits >
concept Book_Traits_Concept = requires {
    // BSN (book sequence number) Counter mixin.
    typename Book_Traits::bsn_counter_t;
    requires BSN_Counter_Concept< typename Book_Traits::bsn_counter_t >;

    // A private data used by book implementation
    // For example: instance of order reference index
    //              that trade_sides uses to locate orders in the storage.
    typename Book_Traits::impl_data_t;

    // Sell side of the book.
    typename Book_Traits::sell_orders_table_t;

    // Buy side of the book.
    typename Book_Traits::buy_orders_table_t;
};

//
// Book_Init_Params_Concept
//

template < typename Book_Traits, typename Book_Init_Params >
concept Book_Init_Params_Concept = requires( const Book_Init_Params & params ) {
    {
        params.bsn_counter()
    } -> std::same_as< typename Book_Traits::bsn_counter_t >;
    { params.impl_data() } -> std::same_as< typename Book_Traits::impl_data_t >;
    {
        params.sell_orders_table(
            std::declval< typename Book_Traits::impl_data_t & >() )
    } -> std::same_as< typename Book_Traits::sell_orders_table_t >;
    {
        params.buy_orders_table(
            std::declval< typename Book_Traits::impl_data_t & >() )
    } -> std::same_as< typename Book_Traits::buy_orders_table_t >;
};

//
// book_t
//

/**
 * @brief Main class that provides a book routine.
 *
 * Acts as the aggregate of both trades sides.
 * Provides a single entry point to add/delete/modify/execute orders.
 */
template < Book_Traits_Concept Book_Traits >
class book_t
{
public:
    using bsn_counter_t               = typename Book_Traits::bsn_counter_t;
    using impl_data_t                 = typename Book_Traits::impl_data_t;
    using sell_orders_table_t         = typename Book_Traits::sell_orders_table_t;
    using buy_orders_table_t          = typename Book_Traits::buy_orders_table_t;
    using order_refs_index_t          = typename impl_data_t::order_refs_index_t;
    using order_refs_index_iterator_t = typename order_refs_index_t::iterator_t;

    template < typename Book_Init_Params >
        requires Book_Init_Params_Concept< Book_Traits, Book_Init_Params >
    explicit book_t( const Book_Init_Params & params )
        : m_book_ctx{ std::make_unique< book_ctx_t >( params ) }
    {
    }

    book_t( const book_t & )             = delete;
    book_t & operator=( const book_t & ) = delete;
    book_t( book_t && )                  = default;
    book_t & operator=( book_t && )      = default;

    /**
     * @brief Get Book Sequence Number.
     */
    [[nodiscard]] bsn_t bsn() const
        noexcept( noexcept( m_book_ctx->bsn_counter.value() ) )
    {
        assert( m_book_ctx );
        return m_book_ctx->bsn_counter.value();
    }

    /**
     * @name Access trade-side specific orders tables.
     * @return [description]
     */
    /// @{
private:
    [[nodiscard]] sell_orders_table_t & mutable_sell() noexcept
    {
        return m_book_ctx->sell;
    };
    [[nodiscard]] buy_orders_table_t & mutable_buy() noexcept
    {
        return m_book_ctx->buy;
    };

    template < trade_side Trade_Side >
    [[nodiscard]] decltype( auto ) mutable_xxx_side() noexcept
    {
        if constexpr( trade_side::buy == Trade_Side )
        {
            return mutable_buy();
        }
        else
        {
            return mutable_sell();
        }
    }

public:
    [[nodiscard]] const sell_orders_table_t & sell() const noexcept
    {
        return m_book_ctx->sell;
    }

    [[nodiscard]] const buy_orders_table_t & buy() const noexcept
    {
        return m_book_ctx->buy;
    }

    template < trade_side Trade_Side >
    [[nodiscard]] decltype( auto ) xxx_side() const noexcept
    {
        if constexpr( trade_side::buy == Trade_Side )
        {
            return buy();
        }
        else
        {
            return sell();
        }
    }
    /// @}

    /**
     * @brief Check if book is empty (no orders at both sides)
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return buy().empty() && sell().empty();
    }

    //
    // bbo_t
    //

    /**
     * @brief A wrapper type for BBO.
     */
    struct bbo_t
    {
        std::optional< order_price_t > bid;
        std::optional< order_price_t > offer;
    };

    /**
     * @brief Gets current BBO of the book.
     */
    [[nodiscard]] bbo_t bbo() const
        noexcept( noexcept( sell().top_price() ) && noexcept( buy().top_price() ) )
    {
        return bbo_t{ .bid = buy().top_price(), .offer = sell().top_price() };
    }

    /**
     * @name Order manipulation API.
     */
    /// @{

    /**
     * @brief Add new order to this table.
     *
     * @pre Order MUST NOT exist in the table.
     *
     * @return An iterator to an entry in order-references' index.
     */
    template < trade_side Trade_Side >
    decltype( auto ) add_order( order_t order )
    {
        auto it = mutable_xxx_side< Trade_Side >().add_order( order );
        order_refs_index().access_value( it )->set_trade_side( Trade_Side );
        m_book_ctx->bsn_counter.inc();
        return it;
    }

    /**
     * @brief Add new order to this table.
     *
     * @pre Order MUST NOT exist in the table.
     *
     * @return An iterator to an entry in order-references' index.
     */
    decltype( auto ) add_order( order_t order, trade_side ts )
    {
        if( trade_side::sell == ts )
        {
            return add_order< trade_side::sell >( order );
        }
        else
        {
            return add_order< trade_side::buy >( order );
        }
    }

    /**
     * @brief Delete an order with a known iterator in the table of
     *        order-references' indexes.
     *
     * @pre Order MUST exist in the book.
     */
    template < trade_side Trade_Side >
    void delete_order( order_refs_index_iterator_t it )
    {
        mutable_xxx_side< Trade_Side >().delete_order( it );
        m_book_ctx->bsn_counter.inc();
    }

    /**
     * @brief Delete an order with a known iterator in the table of
     *        order-references' indexes.
     *
     * @pre Order MUST exist in the book.
     */
    void delete_order( order_refs_index_iterator_t it, trade_side ts )
    {
        if( trade_side::sell == ts )
        {
            delete_order< trade_side::sell >( it );
        }
        else
        {
            delete_order< trade_side::buy >( it );
        }
    }

    /**
     * @brief Delete an order with a given id.
     *
     * @pre Order MUST exist in the book.
     */
    void delete_order( order_id_t id )
    {
        auto it = order_refs_index().find( id );
        assert( it != order_refs_index().end() );

        delete_order( it,
                      order_refs_index().access_value( it )->get_trade_side() );
    }

    /**
     * @brief Perform execute action.
     *
     * @pre @c id MUST exist and MUST refer the first order in the table
     *      (first order at the top price).
     */
    template < trade_side Trade_Side >
    void execute_order( order_refs_index_iterator_t it, order_qty_t exec_qty )
    {
        mutable_xxx_side< Trade_Side >().execute_order( it, exec_qty );
        m_book_ctx->bsn_counter.inc();
    }

    /**
     * @brief Perform execute action.
     *
     * @pre @c id MUST exist and MUST refer the first order in the table
     *      (first order at the top price).
     */
    void execute_order( order_refs_index_iterator_t it,
                        order_qty_t exec_qty,
                        trade_side ts )
    {
        if( trade_side::sell == ts )
        {
            execute_order< trade_side::sell >( it, exec_qty );
        }
        else
        {
            execute_order< trade_side::buy >( it, exec_qty );
        }
    }

    /**
     * @brief Perform execute action.
     *
     * @pre @c id MUST exist and MUST refer the first order in the table
     *      (first order at the top price).
     */
    void execute_order( order_id_t id, order_qty_t exec_qty )
    {
        auto it = order_refs_index().find( id );
        assert( it != order_refs_index().end() );

        execute_order( it,
                       exec_qty,
                       order_refs_index().access_value( it )->get_trade_side() );
    }

    /**
     * @brief Reduce quantity of the order.
     *
     * @pre Order MUST exist in the book.
     */
    template < trade_side Trade_Side >
    void reduce_order( order_refs_index_iterator_t it, order_qty_t canceled_qty )
    {
        mutable_xxx_side< Trade_Side >().reduce_order( it, canceled_qty );
        m_book_ctx->bsn_counter.inc();
    }

    /**
     * @brief Reduce quantity of the order.
     *
     * @pre Order MUST exist in the book.
     */
    void reduce_order( order_refs_index_iterator_t it,
                       order_qty_t canceled_qty,
                       trade_side ts )
    {
        if( trade_side::sell == ts )
        {
            reduce_order< trade_side::sell >( it, canceled_qty );
        }
        else
        {
            reduce_order< trade_side::buy >( it, canceled_qty );
        }
    }

    /**
     * @brief Reduce quantity of the order.
     *
     * @pre Order MUST exist in the book.
     */
    void reduce_order( order_id_t id, order_qty_t canceled_qty )
    {
        auto it = order_refs_index().find( id );
        assert( it != order_refs_index().end() );

        reduce_order( it,
                      canceled_qty,
                      order_refs_index().access_value( it )->get_trade_side() );
    }

    /**
     * @brief Modify order.
     *
     * @pre Order MUST exist in the book.
     */
    template < trade_side Trade_Side >
    void modify_order( order_refs_index_iterator_t it, order_t modified_order )
    {
        mutable_xxx_side< Trade_Side >().modify_order( it, modified_order );
        m_book_ctx->bsn_counter.inc();
    }

    /**
     * @brief Modify order.
     *
     * @pre Order MUST exist in the book.
     */
    void modify_order( order_refs_index_iterator_t it,
                       order_t modified_order,
                       trade_side ts )
    {
        if( trade_side::sell == ts )
        {
            modify_order< trade_side::sell >( it, modified_order );
        }
        else
        {
            modify_order< trade_side::buy >( it, modified_order );
        }
    }

    /**
     * @brief Modify order.
     *
     * @pre Order MUST exist in the book.
     */
    void modify_order( order_t modified_order )
    {
        auto it = order_refs_index().find( modified_order.id );
        assert( it != order_refs_index().end() );

        modify_order( it,
                      modified_order,
                      order_refs_index().access_value( it )->get_trade_side() );
    }
    /// @}

    /**
     * @name Access orders.
     *
     * @note Index must not be used in disruptive way,
     *       so that book state is not broken.
     */
    /// @{
    [[nodiscard]] order_refs_index_t & order_refs_index() noexcept
    {
        return m_book_ctx->impl_data.order_refs_index;
    }

    [[nodiscard]] const order_refs_index_t & order_refs_index() const noexcept
    {
        return m_book_ctx->impl_data.order_refs_index;
    }
    /// @}

private:
    //
    // book_ctx_t
    //

    /**
     * @brief An aggregate to store book's internals
     *        and guarantee pointer immutability when necessary.
     */
    struct book_ctx_t
    {
        template < typename Book_Init_Params >
            requires Book_Init_Params_Concept< Book_Traits, Book_Init_Params >
        explicit book_ctx_t( const Book_Init_Params & params )
            : bsn_counter{ params.bsn_counter() }
            , impl_data{ params.impl_data() }
            , sell{ params.sell_orders_table( impl_data ) }
            , buy{ params.buy_orders_table( impl_data ) }
        {
        }

        [[no_unique_address]] bsn_counter_t bsn_counter{};
        impl_data_t impl_data;
        sell_orders_table_t sell;
        buy_orders_table_t buy;
    };

    std::unique_ptr< book_ctx_t > m_book_ctx;
};

//
// std_book_init_params_t
//

/**
 * @brief An implementation of initial book params
 *        that creates default constructed values that compose the book.
 */
template < Book_Traits_Concept Book_Traits >
struct std_book_init_params_t
{
    using bsn_counter_t       = typename Book_Traits::bsn_counter_t;
    using impl_data_t         = typename Book_Traits::impl_data_t;
    using sell_orders_table_t = typename Book_Traits::sell_orders_table_t;
    using buy_orders_table_t  = typename Book_Traits::buy_orders_table_t;

    [[nodiscard]] bsn_counter_t bsn_counter() const { return bsn_counter_t{}; }
    [[nodiscard]] impl_data_t impl_data() const { return impl_data_t{}; }

    [[nodiscard]] sell_orders_table_t sell_orders_table(
        impl_data_t & impl_data ) const
    {
        return sell_orders_table_t{ impl_data };
    }

    [[nodiscard]] buy_orders_table_t buy_orders_table(
        impl_data_t & impl_data ) const
    {
        return buy_orders_table_t{ impl_data };
    }
};

//
// std_book_t
//

/**
 * @brief A simplified book implementation that
 *        relies on standard init params and
 *        provides default (no arguments) constructor.
 */
template < Book_Traits_Concept Book_Traits >
class std_book_t : public book_t< Book_Traits >
{
public:
    using base_t = book_t< Book_Traits >;
    using base_t::base_t;

    explicit std_book_t()
        : base_t{ std_book_init_params_t< Book_Traits >{} }
    {
    }
};

}  // namespace jacobi::book

namespace fmt
{

template < typename Book_Traits >
struct formatter< ::jacobi::book::book_t< Book_Traits > >
{
    unsigned max_levels = 16;
    char print_type     = 'f';

    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = ctx.begin();
        auto end = ctx.end();

        if( it != end && *it >= '0' && *it <= '9' )
        {
            max_levels = 0;
            // Loop to parse the full integer
            while( it != end && *it >= '0' && *it <= '9' )
            {
                max_levels = max_levels * 10 + ( *it - '0' );
                ++it;
            }
        }

        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }

        return it;
    }

    template < class Format_Context >
    auto format( const ::jacobi::book::book_t< Book_Traits > & book,
                 Format_Context & ctx ) const
    {
        auto print_sell_block = [ & ]( auto rng )
        {
            for( const auto & lvl : rng )
            {
                if( print_type == 'C' && 0 == lvl.orders_count() )
                {
                    return;
                }

                auto r                 = lvl.orders_range();
                const auto orders_line = fmt::format( "{}", fmt::join( r, " " ) );
                const auto depth_str   = [ & ]
                {
                    if( lvl.empty() ) return std::string{};

                    return fmt::format(
                        "{}/{}", lvl.orders_qty(), lvl.orders_count() );
                }();

                fmt::format_to( ctx.out(),
                                "{:>79} S|{:^19}|  {:<76} ...\n",
                                depth_str,
                                type_safe::get( lvl.price() ),
                                std::string_view{ orders_line }.substr( 0, 76 ) );
            }
        };

        auto print_buy_block = [ & ]( auto rng )
        {
            for( const auto & lvl : rng )
            {
                if( print_type == 'C' && 0 == lvl.orders_count() )
                {
                    return;
                }
                auto r                 = lvl.orders_range_reverse();
                const auto orders_line = fmt::format( "{}", fmt::join( r, " " ) );
                const auto depth_str   = [ & ]
                {
                    if( lvl.empty() ) return std::string{};

                    return fmt::format(
                        "{}/{}", lvl.orders_qty(), lvl.orders_count() );
                }();

                std::string_view orders_line_sv{ orders_line };
                orders_line_sv.remove_prefix(
                    std::max< int >( 0, orders_line_sv.size() - 76 ) );

                fmt::format_to( ctx.out(),
                                "\n...{:>76}  |{:^19}|B {}",
                                orders_line_sv,
                                type_safe::get( lvl.price() ),
                                depth_str );
            }
        };

        print_sell_block( book.sell().levels_range()
                          | ranges::views::take( max_levels )
                          | ranges::views::reverse );
        fmt::format_to( ctx.out(), "{:>80}  * * *  * * *  * * *", "" );
        print_buy_block( book.buy().levels_range()
                         | ranges::views::take( max_levels ) );

        return ctx.out();
    }
};

template < typename Book_Traits >
struct formatter< ::jacobi::book::std_book_t< Book_Traits > >
    : public formatter< ::jacobi::book::book_t< Book_Traits > >
{
    using base_t = formatter< ::jacobi::book::book_t< Book_Traits > >;

    template < class Format_Context >
    auto format( const ::jacobi::book::book_t< Book_Traits > & book,
                 Format_Context & ctx ) const
    {
        return base_t::format( book, ctx );
    }
};

}  // namespace fmt
