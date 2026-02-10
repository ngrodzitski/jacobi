// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Defines standard types for Order Book.
 */
#pragma once

#include <cstdint>
#include <cassert>

#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

#include <jacobi/book/utils/lemire_hash.hpp>

namespace jacobi::book
{

//
// order_id_t
//

/**
 * @brief Strong type for order id.
 */
struct order_id_t
    : type_safe::strong_typedef< order_id_t, std::uint64_t >
    , type_safe::strong_typedef_op::equality_comparison< order_id_t >
    , type_safe::strong_typedef_op::increment< order_id_t >
{
    using strong_typedef::strong_typedef;
};
static_assert( std::is_trivially_copyable_v< order_id_t > );

//
// order_id_custom_hash_t
//

struct order_id_custom_hash_t : public utils::lemire_64bit_hash_t
{
    using base_t = utils::lemire_64bit_hash_t;

    [[nodiscard]] auto operator()( order_id_t id ) const
    {
        return ( *this )( type_safe::get( id ) );
    }

    using base_t::operator();
};

//
// order_qty_t
//

/**
 * @brief Strong type for order quantity.
 */
struct order_qty_t
    : type_safe::strong_typedef< order_qty_t, std::uint32_t >
    , type_safe::strong_typedef_op::equality_comparison< order_qty_t >
    , type_safe::strong_typedef_op::relational_comparison< order_qty_t >
    , type_safe::strong_typedef_op::integer_arithmetic< order_qty_t >
{
    using value_type = std::uint32_t;
    using strong_typedef::strong_typedef;

    [[nodiscard]] int to_int() const noexcept
    {
        assert( value_ <= 0x7FFF'FFFFULL );
        return static_cast< int >( value_ );
    }
};
static_assert( std::is_trivially_copyable_v< order_qty_t > );

//
// order_price_t
//

/**
 * @brief Strong type for order price.
 */
struct order_price_t
    : type_safe::strong_typedef< order_price_t, std::int64_t >
    , type_safe::strong_typedef_op::equality_comparison< order_price_t >
    , type_safe::strong_typedef_op::relational_comparison< order_price_t >
    , type_safe::strong_typedef_op::integer_arithmetic< order_price_t >
{
    using value_type = std::int64_t;
    using strong_typedef::strong_typedef;

    [[nodiscard]] int to_int() const noexcept
    {
        assert( value_ <= 0x7FFF'FFFFLL );
        assert( value_ >= -0x8FFF'FFFFLL );
        return static_cast< int >( value_ );
    }
};

static_assert( std::is_trivially_copyable_v< order_price_t > );

inline void swap( order_price_t & a, order_price_t & b ) noexcept
{
    using std::swap;
    swap( static_cast< order_price_t::value_type & >( a ),
          static_cast< order_price_t::value_type & >( b ) );
}

//
// order_t
//

/**
 * @brief Represents a single order in the book.
 */
struct order_t
{
    order_id_t id;
    order_qty_t qty;
    order_price_t price;
};
static_assert( std::is_trivially_copyable_v< order_t > );

//
// trade_side
//

/**
 * @brief  Enumeration to indicate Trade Side of the book.
 *
 * Aligns with CFE encoding.
 */
enum class trade_side : std::uint8_t
{
    buy  = 'B',
    sell = 'S'
};

//
// trade_side_to_index
//

/**
 * @brief Convert trade_size to a index in a range of [0,1].
 */
[[nodiscard]] constexpr inline std::uint8_t trade_side_to_index(
    trade_side ts ) noexcept
{
    return trade_side::sell == ts;
}

//
// order_price_operations_t
//

/**
 * Acts as a unification point for building up a book.
 * In order to use same algorithms for both sell and buy sides
 * we need to be able to express operations and conditions in unified way.
 * for example to answer the question whether the price `X` is higher in priority
 * than the price `Y` for SELL side we must do `X < Y` comparison
 * while for BUY side we need `X > Y`. The purpose of this class is
 * to provide common language for such questions while do necessary
 * operations as necessary for a given trade side.
 *
 */
template < trade_side Side_Indicator >
struct order_price_operations_t
{
    /**
     * @brief The bottom aka MAX price of the trade side
     *        (best price for a given trade side assuming someone
     *        is crazy enough to match it).
     *
     * * For sell side "best" means the highest price (we can't sell any higher).
     * * For buy side "best" means the lowest price (we can't buy lower).
     *
     * This is used for shifting the hot/cold levels so that we
     * know if we are near the bottom and we will not run into having levels
     * with price lower than that (which would be an overflow).
     *
     * Here is an example (assume price is 8-bit int => [-128, +127], N=5):
     * @code
     *              ...
     *              127  s s s s <---- absolute best sell price for seller.
     *              126  s s s s       (Sell bottom).
     *              125  s s s s
     *              124  s s s s
     *              123  s s s s
     *              122
     *              121
     *              ...
     *              ...
     *              ...
     *    b b b b  -124
     *    b b b b  -125
     *    b b b b  -126
     *    b b b b  -127         (Buy bottom).
     *    b b b b  -128   <---- absolute best buy price for buyer.
     * @endcode
     */
    inline static constexpr order_price_t max_value = order_price_t{
        trade_side::buy == Side_Indicator ?
            std::numeric_limits< order_price_t::value_type >::min() :
            std::numeric_limits< order_price_t::value_type >::max()
    };

    /**
     * @brief  The opposite of max value.
     *
     * Here is an example (assume price is 8-bit int => [-128, +127], N=5):
     * @code
     *              ...
     *              127  s s s s <---- absolute best sell price for seller.
     *              126  s s s s       (Sell bottom).
     *              125  s s s s
     *              124  s s s s
     *              123  s s s s
     *              122
     *              121
     *              ...
     *              ...
     *              ...
     *    b b b b  -124
     *    b b b b  -125
     *    b b b b  -126
     *    b b b b  -127         (Buy bottom).
     *    b b b b  -128   <---- absolute best buy price for buyer.
     * @endcode
     */
    inline static constexpr order_price_t min_value = order_price_t{
        trade_side::buy == Side_Indicator ?
            std::numeric_limits< order_price_t::value_type >::max() :
            std::numeric_limits< order_price_t::value_type >::min()
    };

    /**
     * @brief `a` Less Than 'b'.
     */
    [[nodiscard]] bool lt( order_price_t a, order_price_t b ) const noexcept
    {
        if constexpr( trade_side::buy == Side_Indicator )
        {
            return a > b;
        }
        else
        {
            return a < b;
        }
    }

    /**
     * @brief `a` Less orEqual 'b'.
     */
    [[nodiscard]] bool le( order_price_t a, order_price_t b ) const noexcept
    {
        if constexpr( trade_side::buy == Side_Indicator )
        {
            return a >= b;
        }
        else
        {
            return a <= b;
        }
    }

    //
    // cmp_t
    //

    /**
     * @brief A comparator to be used with map types.
     *
     * Provides a comparator that makes ordering in a way
     * that values closer to the opposite trade side come first.
     *
     * @code
     *             ...
     *                  [5]  105 s s s s s s s s s s
     *                  [4]  104 s s s s s s s s s s s s s s s
     *  ordering for    [3]  103 s s s s s
     *  Sell side       [2]  102 s s s s s s s s s
     *                  [1]  101 s s s
     *                  [0]  100 s s s s
     *                        99
     *                 b b b  98  [0]  ordering for Byu side
     *             b b b b b  97  [1]
     *                   b b  96  [2]
     *           b b b b b b  95  [3]
     *             b b b b b  94  [4]
     *                       ...
     * @endcode
     */
    struct cmp_t
    {
        /**
         * @brief A "less" operator for using with std::map-like containers.
         */
        bool operator()( order_price_t a, order_price_t b ) const noexcept
        {
            return order_price_operations_t{}.lt( a, b );
        }
    };

    /**
     * @brief Get the trade-side biased minimum of the prices.
     */
    [[nodiscard]] order_price_t min( order_price_t a,
                                     order_price_t b ) const noexcept
    {
        if constexpr( trade_side::buy == Side_Indicator )
        {
            return std::max( a, b );
        }
        else
        {
            return std::min( a, b );
        }
    }

    /**
     * @brief Calculate the distance between 2 prices.
     *
     * The distance respects the ordering of the price for a given trade side.
     * It means that that when `a` is "less" aka
     * "closer to the opposite trade side than `b`
     * the result is a positive value of order-price.
     *
     * @code
     * // For example let's say:
     * a=100
     * b=200
     *
     * // For Buy side:
     * distance( a, b ) == -100
     * distance( b, a ) == 100
     *
     * // For Sell side:
     * distance( a, b ) == 100
     * distance( b, a ) == -100
     *
     *             ...
     *             105 s s s s s s s        B           A
     *             104 s s s s s s s        ^           v
     *             103 s s s s s s          ^ d = 4     v d = -4
     *             102 s s s s              ^           v
     *             101 s s s s s s          A           B
     *             100 s s
     *              99
     *       b b b  98       B            A
     *   b b b b b  97       ^            v
     *         b b  96       ^ d = -4     v d = 4
     * b b b b b b  95       ^            v
     *   b b b b b  94       A            B
     *             ...
     *
     * @endcode
     *
     */
    [[nodiscard]] order_price_t distance( order_price_t a,
                                          order_price_t b ) const noexcept
    {
        if constexpr( trade_side::buy == Side_Indicator )
        {
            return a - b;
        }
        else
        {
            return b - a;
        }
    }

    /**
     * @brief A safe distance calculation between 2 varaibles.
     *        accounts for cases the distance between a and b
     *        cannot be represented with int64
     *        (which is order_price_t underlying type)
     *
     * @pre `lt(a, b)==true`
     */
    [[nodiscard]] std::uint64_t safe_u64_distance( order_price_t a,
                                                   order_price_t b ) const noexcept
    {
        assert( le( a, b ) );

        const auto aa = static_cast< std::uint64_t >( type_safe::get( a ) );
        const auto bb = static_cast< std::uint64_t >( type_safe::get( b ) );

        if constexpr( trade_side::buy == Side_Indicator )
        {
            return aa - bb;
        }
        else
        {
            return bb - aa;
        }
    }

    /**
     * @brief Advances price forward (in the meaning of a given trade side).
     *
     * The direction forward is the direction towards the opposite trade side.
     *
     * @code
     *             ...
     *            vv         105 s s s s s s s s s s
     * Sell FWD   vv         104 s s s s s s s s s s s s s s s
     * direction  vv         103 s s s s s
     *            vv         102 s s s s s s s s s
     *            vv         101 s s s
     *                       100 s s s s
     *                        99
     *                 b b b  98    ^^
     *             b b b b b  97    ^^  Buy FWD
     *                   b b  96    ^^  Direction
     *           b b b b b b  95    ^^
     *             b b b b b  94    ^^
     *
     * // For example let's say:
     * price=100
     * delta=25
     *
     * // For Buy side:
     * advance_forward( price, delta ) == 125
     *
     * // For Sell side:
     * advance_forward( price, delta ) == 75
     * @endcode
     *
     * @code
     *             ...
     *             105 s s s s s s s s s s s s
     *             104 s s s s s s s s s s s s
     *             103 s s s s s s
     *             102 s s s s             SELL:
     *             101 s s s s s s      <- price
     *             100 s s              <- advance_forward( price, 1 )
     *              99
     *       b b b  98     BUY:
     *   b b b b b  97  <- advance_forward( price, 1 )
     *         b b  96  <- price
     * b b b b b b  95
     *   b b b b b  94
     *             ...
     * @endcode
     */
    [[nodiscard]] order_price_t advance_forward(
        order_price_t price,
        order_price_t delta = order_price_t{ 1 } ) const noexcept
    {
        if constexpr( trade_side::buy == Side_Indicator )
        {
            return price + delta;
        }
        else
        {
            return price - delta;
        }
    }

    /**
     * @brief Advances price backward (in the meaning of a given trade side).
     *
     * Does the opposite to `advance_forward(p, d)`.
     */
    [[nodiscard]] order_price_t advance_backward(
        order_price_t price,
        order_price_t delta = order_price_t{ 1 } ) const noexcept
    {
        if constexpr( trade_side::buy == Side_Indicator )
        {
            return price - delta;
        }
        else
        {
            return price + delta;
        }
    }
};

template < trade_side Side_Indicator >
using reverse_order_price_operations_t =
    std::conditional_t< trade_side::buy == Side_Indicator,
                        order_price_operations_t< trade_side::sell >,
                        order_price_operations_t< trade_side::buy > >;

//
// bsn_t
//

/**
 * @brief Strong type for Exchange Sequence Number.
 */
struct bsn_t
    : type_safe::strong_typedef< bsn_t, std::uint64_t >
    , type_safe::strong_typedef_op::equality_comparison< bsn_t >
    , type_safe::strong_typedef_op::relational_comparison< bsn_t >
    , type_safe::strong_typedef_op::integer_arithmetic< bsn_t >
{
    using value_type = std::uint64_t;
    using strong_typedef::strong_typedef;

    [[nodiscard]] int to_int() const noexcept
    {
        assert( value_ <= 0x7FFF'FFFFULL );
        return static_cast< int >( value_ );
    }

    [[nodiscard]] bsn_t prev() const noexcept
    {
        assert( value_ != 0ULL );
        return bsn_t{ value_ - 1ULL };
    }

    [[nodiscard]] bsn_t safe_prev() const noexcept
    {
        return bsn_t{ value_ != 0ULL ? value_ - 1ULL : 0ULL };
    }

    [[nodiscard]] bsn_t next() const noexcept { return bsn_t{ value_ + 1ULL }; }
};
static_assert( std::is_trivially_copyable_v< bsn_t > );

}  // namespace jacobi::book

namespace fmt
{

template <>
struct formatter< ::jacobi::book::trade_side >
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = std::begin( ctx );
        auto end = std::end( ctx );
        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }
        return it;
    }

    template < class Format_Context >
    auto format( ::jacobi::book::trade_side ts, Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{}",
            ::jacobi::book::trade_side::buy == ts ? "buy" : "sell" );
    }
};

template <>
struct formatter< ::jacobi::book::order_id_t >
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = std::begin( ctx );
        auto end = std::end( ctx );
        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }
        return it;
    }

    template < class Format_Context >
    auto format( const ::jacobi::book::order_id_t & id,
                 Format_Context & ctx ) const
    {
        return fmt::format_to( ctx.out(), "0x{:X}", type_safe::get( id ) );
    }
};

template <>
struct formatter< ::jacobi::book::order_qty_t >
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = std::begin( ctx );
        auto end = std::end( ctx );
        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }
        return it;
    }

    template < class Format_Context >
    auto format( const ::jacobi::book::order_qty_t & qty,
                 Format_Context & ctx ) const
    {
        return fmt::format_to( ctx.out(), "{}", type_safe::get( qty ) );
    }
};
template <>
struct formatter< ::jacobi::book::order_price_t >
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = std::begin( ctx );
        auto end = std::end( ctx );
        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }
        return it;
    }

    template < class Format_Context >
    auto format( const ::jacobi::book::order_price_t & qty,
                 Format_Context & ctx ) const
    {
        return fmt::format_to( ctx.out(), "{}", type_safe::get( qty ) );
    }
};

template <>
struct formatter< ::jacobi::book::order_t >
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = std::begin( ctx );
        auto end = std::end( ctx );
        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }
        return it;
    }

    template < class Format_Context >
    auto format( const ::jacobi::book::order_t & order,
                 Format_Context & ctx ) const
    {
        return fmt::format_to( ctx.out(), "[{} {}]", order.qty, order.id );
    }
};

template <>
struct formatter< ::jacobi::book::bsn_t >
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        auto it  = std::begin( ctx );
        auto end = std::end( ctx );
        if( it != end && *it != '}' )
        {
            throw fmt::format_error( "invalid format" );
        }
        return it;
    }

    template < class Format_Context >
    auto format( const ::jacobi::book::bsn_t & bsn, Format_Context & ctx ) const
    {
        return fmt::format_to( ctx.out(), "{}", type_safe::get( bsn ) );
    }
};

}  // namespace fmt
