// Copyright (c) 2026 Nicolai Grodzitski
// SPDX-License-Identifier: BSL-1.0

/**
 * @file Orders table implementation based on hot-cold storages.
 *       Cold storage is base on ordered-map (price=>price_level)
 *       Hot storage is a vector of size N (N is a power of 2)
 *       for accessing price levels near the top.
 */

#pragma once

#include <type_traits>
#include <map>
#include <bit>
#include <stdexcept>

#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/take.hpp>

#include <jacobi/book/vocabulary_types.hpp>
#include <jacobi/book/price_level.hpp>
#include <jacobi/book/orders_table_crtp_base.hpp>

namespace jacobi::book::mixed::hot_cold
{

namespace details
{

/**
 * @name Constraints for the hot storage.
 */
/// @{
inline static constexpr std::size_t default_hot_levels_count = 32ull;
inline static constexpr std::size_t min_hot_levels_count     = 8ull;
inline static constexpr std::size_t max_hot_levels_count     = 4096ull;
/// @}

}  // namespace details

//
// hot_range_selection
//

/**
 * @brief Control flag for hot level range selection.
 */
enum class hot_range_selection
{
    /**
     * Selects hot price level starting with current best price
     * and includes as much as there exist from that point
     */
    best_price_and_further,

    /**
     * Selects hot price level starting from the head of the
     * storage, which might include the a few empty levels before
     * a level with best price appears.
     */
    storage_head_and_further,
};

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
 *     * Splits a storage into hot and cold levels.
 *     * Hot levels are the ones located near the current top (near the BBO).
 *       Hot levels are stored in a vector for a quick access.
 *       And leverages the assumption that most of events are happened
 *       at hot levels.
 *     * Cold levels are stored in the map c-like container.
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
    using typename base_type_t::book_private_data_t;

    using price_ops_t = order_price_operations_t< Side_Indicator >;

    explicit orders_table_t( std::size_t hot_levels_count,
                             book_private_data_t & data )
        : base_type_t{ data }
    {
        // We only allow `2^x` values for hot levels,
        // so that we can use binary AND (`&`) with a mask (like `0b1111`)
        // instead of integer DIV (`/`) to lookup entries in the hot storage.
        // So e.g. instead of 40 we would take 64, or instead of 100 => 128.
        hot_levels_count = std::bit_ceil( hot_levels_count );

        if( details::min_hot_levels_count > hot_levels_count )
        {
            throw std::out_of_range{ fmt::format(
                "unable to construct "
                "jacobi::book::mixed::hot_cold::orders_table_t "
                "with hot_levels_count={} (min allowed is {})",
                hot_levels_count,
                details::min_hot_levels_count ) };
        }
        if( details::max_hot_levels_count < hot_levels_count )
        {
            throw std::out_of_range{ fmt::format(
                "unable to construct "
                "jacobi::book::mixed::hot_cold::orders_table_t "
                "with hot_levels_count={} (max allowed is {})",
                hot_levels_count,
                details::max_hot_levels_count ) };
        }

        m_hot_levels_mask = hot_levels_count - 1;

        // We choose the initial head price
        // in such a way that hot levels a positioned at the
        // extreme ends of the price tape:
        //
        //                 ...
        //        [7]  105  Smax     Smax - max value for sell price
        //        [6]  103  Smax-1
        //        [5]  102  Smax-2
        //        [4]  101  Smax-3
        //        [3]  100  Smax-4
        //        [2]  100  Smax-5                  HOT LEVELS
        //        [1]  100  Smax-6                  SELL
        //  HEAD [*0]  100  Smax-7
        //             ...
        //             ...
        //             ...
        //          7+Bmax  [0*]  HEAD
        //          6+Bmax  [1]                     BUY
        //          5+Bmax  [2]                     HOT LEVELS
        //          4+Bmax  [3]
        //          3+Bmax  [4]
        //          2+Bmax  [5]
        //          1+Bmax  [6]
        //            Bmax  [7]   Bmax - max value for buy price
        const auto initial_head_price = price_ops_t{}.advance_forward(
            price_ops_t::max_value,
            order_price_t{ static_cast< int >( hot_levels_count - 1 ) } );

        // fmt::print("HC LINE: {}; initial_head_price={}\n", __LINE__,
        // initial_head_price);

        make_hot_storage_initial_state( initial_head_price, hot_levels_count );

        // fmt::print("HC LINE: {}\n", __LINE__ );
        // Initialy when no top orders exist we consider top market order to be
        // at the end of hot levels. Which is basically the extreme price for given
        // trade side (price_manipulations::bottom_value()).
        m_top_level_virtual_index =
            static_cast< unsigned >( hot_storage_size() ) - 1;
        // fmt::print("HC LINE: {}; m_top_level_virtual_index={}\n", __LINE__,
        // m_top_level_virtual_index );
    }

    explicit orders_table_t( book_private_data_t & data )
        : orders_table_t{ details::default_hot_levels_count, data }
    {
    }

    /**
     * @name Hot/Cold orders table specific API.
     */
    /// @{
    [[nodiscard]] std::size_t hot_levels_count() const noexcept
    {
        return hot_storage_size() - m_top_level_virtual_index;
    }

    [[nodiscard]] std::size_t cold_levels_count() const noexcept
    {
        return m_cold_levels.size();
    }
    /// @}

    /**
     * @name Access HOT levels.
     */
    /// @{
    [[nodiscard]] std::size_t hot_levels_storage_size() const noexcept
    {
        return m_hot_levels.size();
    }

    template < hot_range_selection Hot_Range_Selection =
                   hot_range_selection::best_price_and_further >
    [[nodiscard]] auto hot_levels_range() const noexcept
    {
        const auto segments = make_hot_levels_segments< Hot_Range_Selection >();
        return ranges::views::concat( segments.first, segments.second );
    }

    [[nodiscard]] auto hot_levels_range(
        hot_range_selection selection ) const noexcept
    {
        if( hot_range_selection::best_price_and_further == selection )
        {
            return hot_levels_range<
                hot_range_selection::best_price_and_further >();
        }
        return hot_levels_range< hot_range_selection::storage_head_and_further >();
    }
    /// @}

    /**
     * @name Access COLD levels.
     */
    ///@{
    [[nodiscard]] auto cold_levels_range() noexcept
    {
        return m_cold_levels | ranges::views::values;
    }

    [[nodiscard]] auto cold_levels_range() const noexcept
    {
        return m_cold_levels | ranges::views::values;
    }
    ///@}

    [[nodiscard]] bool empty() const noexcept { return top_level().empty(); }

    /**
     * @brief Get top price.
     */
    [[nodiscard]] std::optional< order_price_t > top_price() const noexcept
    {
        if( empty() )
        {
            return std::nullopt;
        }
        return top_level().price();
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
        return top_level().orders_qty();
    }

    /**
     * @name Access levels.
     */
    [[nodiscard]] auto levels_range() const noexcept
    {
        return ranges::views::concat(
            hot_levels_range()
                | ranges::views::filter( []( const auto & lvl )
                                         { return !lvl.empty(); } ),
            cold_levels_range() );
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

    [[nodiscard]] const price_level_t & top_level() const noexcept
    {
        return m_hot_levels[ make_hot_real_index( m_top_level_virtual_index ) ];
    }

private:
    /**
     * @brief Get top price level.
     *
     * @pre Table must not be empty.
     */
    [[nodiscard]] price_level_t & top_level() noexcept
    {
        return m_hot_levels[ make_hot_real_index( m_top_level_virtual_index ) ];
    }

    void make_hot_storage_initial_state( order_price_t head_price,
                                         std::size_t hot_levels_count )
    {
        assert( m_hot_levels.empty() );
        m_hot_levels.reserve( hot_levels_count );

        for( unsigned i = 0; i < hot_levels_count; ++i )
        {
            const auto price =
                price_ops_t{}.advance_backward( head_price, order_price_t{ i } );

            m_hot_levels.emplace_back(
                this->m_book_private_data.price_levels_factory.make_price_level(
                    price ) );
        }

        m_hot_head_real_index = 0;
    }

    void reset_hot_storage_initial_state( order_price_t head_price )
    {
        assert( !m_hot_levels.empty() );

        for( unsigned i = 0; i < m_hot_levels.size(); ++i )
        {
            const auto price =
                price_ops_t{}.advance_backward( head_price, order_price_t{ i } );

            m_hot_levels[ i ] =
                this->m_book_private_data.price_levels_factory.make_price_level(
                    price );
        }

        m_hot_head_real_index = 0;
    }

    /**
     * @name Helper routines to work with hot storage.
     */
    /// @{
    /**
     * @brief  Hot level storage size.
     */
    template < typename T = std::size_t >
    [[nodiscard]] T hot_storage_size() const noexcept
    {
        return static_cast< T >( m_hot_levels.size() );
    }

    /**
     * @brief  Hot level storage size.
     */
    template < typename T = std::size_t >
    [[nodiscard]] T hot_storage_half_size() const noexcept
    {
        return hot_storage_size< T >() / 2;
    }

    /**
     * @brief  Hot level storage size.
     */
    template < typename T = std::size_t >
    [[nodiscard]] T hot_storage_quarter_size() const noexcept
    {
        return hot_storage_size< T >() / 4;
    }

    /**
     * @brief  Get the real index in m_hot_levels in for a given
     *         logical index of the price level item.
     *
     * @param  i  Logical index.
     *
     * @return  Return a real index.
     */
    [[nodiscard]] std::size_t make_hot_real_index( unsigned i ) const noexcept
    {
        return ( m_hot_head_real_index + i ) & m_hot_levels_mask;
    }

    /**
     * @brief Figure out what would be a virtual pos of the price
     *        in hot levels array that is started with head_price.
     *
     * @code
     *   Note: to simplify the picture the price is int8.
     *
     *     [4]   127  s s s s
     *     [3]   126  s s s s
     *     [2]   125  s s s s
     *     [1]   124  s s s s
     *     [0]   123  s s s s <= Head of the hot levels array for Sell side.
     *    [-1]   122  [-2]       "position" of price `127` from the head is `4`.
     *    [-2]   121  [-1]
     * b b b b   120  [0]  <= Head of the hot levels array for Buy side.
     * b b b b   119  [1]     "position" of the price `118` from the head
     * b b b b   118  [2]     is `2`. The position of the price `122`
     * b b b b   117  [3]     from the head is `-2`.
     * b b b b   116  [4]
     * @endcode
     */
    [[nodiscard]] static constexpr order_price_t::value_type
    make_hot_virtual_index( order_price_t head_price,
                            order_price_t price ) noexcept
    {
        return type_safe::get( price_ops_t{}.distance( head_price, price ) );
    }

    /**
     * @name Make two the segments for hot levels.
     *
     * Since the arangement of a logical sequence of price levels in
     * a vector storage for hot level is not fixed (starting from the first
     * element) we need to be able to tell 1 or 2 values for a lengths of a
     * segments of a logical levels.
     *
     * For a matter of unification we always think that a range of hot levels
     * is always split in 2 segments:
     *
     *   * the one that starts with m_top_level_virtual_index or
     *     m_hot_head_real_index (depends on Hot_Range_Selection);
     *   * the one that starts with `m_hot_levels[0]`
     *     (the length for it might be 0).
     *
     * Here is an example of calculations:
     * @code
     * [0] X
     * [1] X
     * [2] X
     * [3] <--- m_hot_head_real_index=3
     * [4]
     * [5] X * <--- m_top_level_virtual_index=2 => real_pos=3+2=5
     * [6] X        all lvls starting from [5] and upto [3] (not including [3])
     * [7] X
     *
     * The segments would be:
     *     # ( [5] [6] [7] ) of length 3.
     *     # ( [0] [1] [2] ) of length 3.
     * ==== ==== ==== ==== ==== ==== ==== ==== ====
     * [0]
     * [1]
     * [2]
     * [3] X * <---m_top_level_virtual_index=5 => real_pos=(6+5)%8=3
     * [4] X
     * [5] X
     * [6] * <--- m_hot_head_real_index=6
     * [7]
     *
     * The segments would be:
     *     # ( [3] [4] [5] )  of length 3.
     *     # ( )              of length 0.
     * @endcode
     */
    ///@{
    using hot_level_segment_t       = std::span< price_level_t >;
    using const_hot_level_segment_t = std::span< const price_level_t >;

    template < hot_range_selection Hot_Range_Selection >
    [[nodiscard]] std::pair< hot_level_segment_t, hot_level_segment_t >
    make_hot_levels_segments() noexcept
    {
        if constexpr( hot_range_selection::best_price_and_further
                      == Hot_Range_Selection )
        {
            const auto count_of_levels_from_top = hot_levels_count();
            const std::size_t first_part_len =
                std::min( hot_storage_size()
                              - make_hot_real_index( m_top_level_virtual_index ),
                          count_of_levels_from_top );

            const std::size_t second_part_len =
                count_of_levels_from_top - first_part_len;

            return std::make_pair(
                hot_level_segment_t{ &m_hot_levels[ make_hot_real_index(
                                         m_top_level_virtual_index ) ],
                                     first_part_len },
                hot_level_segment_t{ &m_hot_levels[ 0 ], second_part_len } );
        }
        else
        {
            return std::make_pair(
                hot_level_segment_t{ &m_hot_levels[ m_hot_head_real_index ],
                                     m_hot_levels.size() - m_hot_head_real_index },
                hot_level_segment_t{
                    &m_hot_levels[ 0 ],
                    static_cast< std::size_t >( m_hot_head_real_index ) } );
        }
    }

    template < hot_range_selection Hot_Range_Selection >
    [[nodiscard]] std::pair< const_hot_level_segment_t, const_hot_level_segment_t >
    make_hot_levels_segments() const noexcept
    {
        // static_assert( std::is_same_v<decltype(*this), std::string>);
        return const_cast< orders_table_t & >( *this )
            .template make_hot_levels_segments< Hot_Range_Selection >();
    }
    /// @}

    /**
     * @brief Write access HOT levels (private).
     */
    template < hot_range_selection Hot_Range_Selection =
                   hot_range_selection::best_price_and_further >
    [[nodiscard]] auto hot_levels_range() noexcept
    {
        const auto segments = make_hot_levels_segments< Hot_Range_Selection >();
        return ranges::views::concat( segments.first, segments.second );
    }

    /**
     * @brief  Get access to a price level of a given price.
     */
    [[nodiscard]] std::pair< price_level_t *, order_price_t > level_at(
        order_price_t price )
    {
        const auto current_head_price =
            m_hot_levels[ make_hot_real_index( 0 ) ].price();

        if( price_ops_t{}.le( current_head_price, price ) ) [[likely]]
        {
            // Here: the level is either in hot storage or in cold storage.

            if( const auto hot_virtual_index =
                    price_ops_t{}.safe_u64_distance( current_head_price, price );
                hot_virtual_index < hot_storage_size() ) [[likely]]
            {
                // Maybe that is a new top:
                m_top_level_virtual_index = std::min< unsigned >(
                    static_cast< unsigned >( hot_virtual_index ),
                    m_top_level_virtual_index );

                const auto i = make_hot_real_index(
                    static_cast< unsigned >( hot_virtual_index ) );

                assert( m_hot_levels[ i ].price() == price );

                return std::make_pair( &m_hot_levels[ i ], price );
            }

            auto it = m_cold_levels.lower_bound( price );
            if( it == end( m_cold_levels ) || it->first != price )
            {
                it = m_cold_levels.insert(
                    it,
                    std::make_pair( price,
                                    this->m_book_private_data.price_levels_factory
                                        .make_price_level( price ) ) );
            }
            return std::make_pair( &( it->second ), price );
        }

        // Here: the level is moving this trade side forward,
        //       because the asked price level is neither in
        //       hot storage not in cold and the price is
        //       lesser than the min-price in a hot storage.
        //       see make_hot_pos documentation.

        // Reconsider the hot/cold storage with a new top price.
        const auto [ new_head_price, hot_head_diff ] = [ & ]
        {
            // That is a standard diff value
            // aka how many levels forward we need to move
            // the hot storage so that
            // a new top price would be positioned in the middle of the
            // range which is covered as cold levels.
            //
            // C   Current hot range.
            // D   Next hot range.
            //                 ********************
            //                 * SELL TRADE SIDE  *
            //                 ********************
            //               C
            //              ###_____________
            //              ###_____________           @@@
            //              ###_____________           @@@      FWD
            //              ###_____________           @@@      Direction
            //              ###_____________           @@@    (towards oposite
            //              ###_____________         @@@@@@@   side)
            //   Current    ###_______D_____          @@@@@
            //      HEAD => HHH______###____ ----+     @@@
            //              _________###____     |      @
            //              _________###____     |
            //    Asked     _________###____     |  That much we
            //     Level => ***______***____     |  need to go forward
            //              _________###____     |
            //              _________###____     |
            //  NEW HEAD => _________HHH____-----+
            //              ________________
            //
            const size_t candidate_diff_1 =
                // This much just to make new head at the position
                // of the asked price:
                price_ops_t{}.safe_u64_distance( price, current_head_price )
                // And a little bit more, so that after moving the hot range
                // top price would be in the middle of it:
                + hot_storage_half_size() - 1;

            // The edge case:
            // in case the hot-prices range is already near the extreme
            // (aka min values) so we are a little bit off the min price
            // and the standard diff value would suggest that some levels
            // are at prices that is not representable by
            // order_price_t::value_type (int64).
            //
            // C   Current hot range.
            // D   Default (standard) hot range after move
            //     to include asked price.
            // E   Edge hot range, the one that respects type defined
            //     representable range (int64) and includes the asked price.
            //
            //               ********************
            //               * SELL TRADE SIDE  *
            //               ********************
            //             C
            //            ###_______________________
            //            ###_______________________
            //            ###_______________________         @@@
            //            ###_______________________         @@@   FWD
            //            ###________________E______         @@@   Direction
            //            ###_______________###_____         @@@
            //            ###_______D_______###_____       @@@@@@@
            //            ###______###______###_____        @@@@@
            //            _________###______###_____         @@@
            //            _________###______###_____          @
            //            _________###______###_____
            //   MIN      ***______***______***_____ <= asked price
            //   Value => _________###______###_____
            //           /xxxxxxx  ###  xxxxxxxxxxxx
            //  Prices  / xxxxxxx  ###  xxxxxxxxxxxx xxx - virtual prices,
            //  out of /  xxxxxxx       xxxxxxxxxxxx       not representable
            //  int64     xxxxxxxxxxxxxxxxxxxxxxxxxx       with int64
            //            xxxxxxxxxxxxxxxxxxxxxxxxxx
            const auto candidate_diff_2 = price_ops_t{}.safe_u64_distance(
                price_ops_t::min_value, current_head_price );

            order_price_t head_price;
            std::size_t head_diff;
            if( candidate_diff_1 < candidate_diff_2 ) [[likely]]
            {
                // Likely the default move is what we need.
                head_price = price_ops_t{}.advance_forward(
                    current_head_price,
                    order_price_t{ static_cast< order_price_t::value_type >(
                        candidate_diff_1 ) } );

                head_diff = candidate_diff_1;
            }
            else
            {
                head_price = price_ops_t::min_value;
                head_diff  = candidate_diff_2;
            }

            return std::make_pair( head_price, head_diff );
        }();

        // So we will try to move the price so that
        // a new top price level will be placed at the
        // middle of the hot storage, and there exist two cases
        // (Buy side is used for illustration so that you don't
        //  stick with understanding the mechanics only under one angle):
        //               *******************
        //               * BUY TRADE SIDE  *
        //               *******************
        //
        //     TARGET             |  Case 1    | Case 2
        //     HOT RANGE          |  Before    | Before
        //    _____               | __         |  __
        // [ ]      <= NEW        |   |        |    |
        // [ ] Q1      Head       |   |        |    |
        // [ ]_____               |   |        |    |
        // [ ]                    |   |  hot   |    |             @
        // [ ] Q2                 |    > head  |    |            @@@
        // [*]_____<= NEW top     |   |  diff  |    |  hot      @@@@@
        // [ ]        level aka   |   |        |     > head    @@@@@@@
        // [ ] Q3     asked price |   |        |    |  diff      @@@
        // [1]_____               |   |        |    |            @@@
        // [ ]                    | __|        |    |            @@@
        // [ ] Q4                ***<= Before  |    |            @@@
        // [ ]_____               |    head    |    |                  FWD
        // [ ]                    |            |    |            Direction
        // [ ]                    |            |  __|
        // [2]                    |           *** <= Before
        // [ ]                    |            |     head
        //
        // Case 1: we need to move a little bit,
        //         and the new and old ranges overlap,
        //         so some entries in hot storage will stay
        //         tricky case.
        //
        // Case 2: No overlap of ranges. Simple case

        // Move to cold storage levels
        // that would be evicted from hot storage.
        ranges::for_each(
            hot_levels_range< hot_range_selection::storage_head_and_further >()
                | ranges::views::reverse | ranges::views::take( hot_head_diff ),
            [ & ]( auto & lvl )
            {
                if( !lvl.empty() )
                {
                    const auto p = lvl.price();
                    m_cold_levels.insert( std::make_pair( p, std::move( lvl ) ) );
                }
            } );

        m_hot_head_real_index = make_hot_real_index(
            hot_storage_size() - hot_head_diff % hot_storage_size() );
        m_top_level_virtual_index =
            make_hot_virtual_index( new_head_price, price );

        // Update price levels as necessary.
        ranges::for_each(
            hot_levels_range< hot_range_selection::storage_head_and_further >()
                | ranges::views::take( hot_head_diff ),
            [ &, price_offset = 0 ]( auto & lvl ) mutable
            {
                const auto price = price_ops_t{}.advance_backward(
                    new_head_price, order_price_t{ price_offset++ } );

                lvl = this->m_book_private_data.price_levels_factory
                          .make_price_level( price );
            } );

        return std::make_pair(
            &m_hot_levels[ make_hot_real_index( m_top_level_virtual_index ) ],
            price );
    }

    /**
     * @brief  Perform sliding hot levels window down the bottom
     *         of a given trade side.
     *
     * @param shift_size  Numbers of hot levels to shift.
     */
    void slide_hot_storage_down( unsigned shift_size )
    {
        assert( shift_size <= hot_storage_half_size() );

        // The tail price of the range as it slides,
        // W need to track it because once it positioned on the edge
        // of the int64 range we stop sliding even if the new top price
        // is not positioned in the middle of the hot range.
        // This is an edge case handling, normally you don't have prices near
        // the extreme values of int64 (espexially when it is given
        // the price is normalized).
        //            xxxxxxxxxxx
        //  Prices    xxxxxxxxxxx xxx - virtual prices,
        //  out of    xxxxxxxxxxx       not representable
        //  int64     xxxxxxxxxxx       with int64
        //            xxxxxxxxxxx
        //            ___________  <= EXTREME SELL
        //            ___________     PRICE VALUE
        //            ___________
        //   TAIL  => ___###_____         @@@                **********
        //   PRICE    ___###_____         @@@   FWD          *        *
        //            ___###_____         @@@   Direction    *  SEll  *
        //            ___###_____         @@@                *  TRADE *
        //            ___###_____       @@@@@@@              *  SIDE  *
        //            ___###_____        @@@@@               *        *
        //            ___###_____         @@@                **********
        //    HEAD => ___###_____          @
        //            ___________
        //            ___________
        //            ___________
        //            ...
        //            ...
        //            ...
        //            ___________
        //            ___________
        //            ___________
        //    HEAD => ___###_____          @
        //            ___###_____         @@@                **********
        //            ___###_____        @@@@@               *        *
        //            ___###_____       @@@@@@@              *  BUY   *
        //            ___###_____         @@@                *  TRADE *
        //            ___###_____         @@@  FWD           *  SIDE  *
        //            ___###_____         @@@  Direction     *        *
        //   TAIL     ___###_____         @@@                **********
        //   PRICE => ___###_____
        //            ___________
        //            ___________     EXTREME BUY
        //            ___________  <= PRICE VALUE
        //           /xxxxxxxxxxx
        //  Prices  / xxxxxxxxxxx xxx - virtual prices,
        //  out of /  xxxxxxxxxxx       not representable
        //  int64     xxxxxxxxxxx       with int64
        //            xxxxxxxxxxx

        auto tail_price =
            m_hot_levels[ make_hot_real_index( m_hot_levels.size() - 1 ) ].price();

        auto cold_it = begin( m_cold_levels );

        for( ; shift_size != 0; --shift_size )
        {
            if( tail_price == price_ops_t::max_value ) [[unlikely]]
            {
                // That means we cannot go further than that
                break;
            }

            tail_price = price_ops_t{}.advance_backward( tail_price );

            // Move existing levels from cold cache.
            assert( m_hot_levels[ m_hot_head_real_index ].empty() );

            if( cold_it != end( m_cold_levels ) && cold_it->first == tail_price )
            {
                m_hot_levels[ m_hot_head_real_index ] =
                    std::move( cold_it++->second );
            }
            else
            {
                // The level is empty (that's why it is missing from cold)
                // so we simply update the price of the level
                m_hot_levels[ m_hot_head_real_index ] =
                    this->m_book_private_data.price_levels_factory
                        .make_price_level( tail_price );
            }

            // Advance head:
            m_hot_head_real_index = make_hot_real_index( 1 );

            assert( m_top_level_virtual_index > 0 );
            --m_top_level_virtual_index;
        }

        m_cold_levels.erase( begin( m_cold_levels ), cold_it );
    }

    /**
     * @brief Drop current top level row of the hot table.
     *
     * That means that our hot levels might be adjusted.
     */
    void drop_hot_top_level()
    {
        auto hot_levels_to_examine = hot_levels_range() | ranges::views::drop( 1 );

        auto new_top_it =
            ranges::find_if( hot_levels_to_examine,
                             []( const auto & lvl ) { return !lvl.empty(); } );

        if( new_top_it != end( hot_levels_to_examine ) ) [[likely]]
        {

            // CASE 1:
            // We got a non-empty level in hot range.
            // So a new head is located in current hot storage.
            // We possibly need to slide the hot storage range
            // so that new top is located in middle 2 quarters. of the range.
            //
            const auto old_top_price =
                m_hot_levels[ make_hot_real_index( m_top_level_virtual_index ) ]
                    .price();
            const auto new_top_lvl_virtual_index_in_old_range =
                m_top_level_virtual_index
                + static_cast< unsigned >( price_ops_t{}.safe_u64_distance(
                    old_top_price, new_top_it->price() ) );

            assert( new_top_lvl_virtual_index_in_old_range
                    < hot_storage_size< order_price_t::value_type >() );

            m_top_level_virtual_index = new_top_lvl_virtual_index_in_old_range;

            const bool is_in_the_last_quarter_of_the_hot_storage =
                hot_storage_size() - new_top_lvl_virtual_index_in_old_range
                <= hot_storage_quarter_size();

            if( !is_in_the_last_quarter_of_the_hot_storage ) [[likely]]
            {
                // The new top is still in middle 2 quarters
                // so we don't have to move the top.
                //    __________
                // [ ]
                // [ ] Q1
                // [ ]__________   __
                // [*]               |
                // [*] Q2            |
                // [*]__________     | New top level is stil
                // [*]               | somewhere in the middle
                // [*] Q3            | of the hot range.
                // [*]__________   __|
                // [ ]
                // [ ] Q4
                // [ ]__________
                return;
            }

            // The new top is in last quarter of the hot array.
            // Need to shift the hot storage down because it looks like
            // the tendency is to go toowards the bottom of this trade side.
            //    __________
            // [ ]
            // [ ] Q1                                              @
            // [ ]__________                                      @@@
            // [ ]                                               @@@@@
            // [ ] Q2                                           @@@@@@@
            // [ ]__________  <=== Consider move hot levels       @@@
            // [ ]                 so that new top is here.       @@@  FWD
            // [ ] Q3                                             @@@  Direction
            // [ ]__________   __                                 @@@
            // [*]               |
            // [*] Q4             >  <==  HERE: position
            // [*]__________   __|        of new top in hot storage
            //                            Our candidate is HERE
            //
            //  So to make hot levels aligned with current trend
            //  we will shift the the hot levels, so that
            //  it current top would be in the middle:
            //  Buttom of the Q2.

            // clang-format off
//    _______
// [*]       <-- OLD-head-pos (X)       __  Remove from hot storage.
// [*] Q1                           [ ]   |
// [*]_______                       [ ]   |
// [*] Q2                     _____ [ ] __|
// [*]_______                       [*] <-- NEW-head-pos (Y)       @
// [*]                          Q1  [*]                           @@@
// [*] Q3                     _____ [*]                          @@@@@
// [*]_______                       [*]                         @@@@@@@
// [*]         (I)              Q2  [*]     (J)                   @@@
// [*] Q4  <-- new-top within _____ [*] <-- new top will be       @@@  FWD
// [*]_______  old hot              [*] __  positioned here.      @@@  Direction
// [ ]         storage          Q3  [*]   |                       @@@
// [ ]                        _____ [*]   |
// [ ]                              [*]   | levels pulled
// [ ]                          Q4  [*]   | from cold storage
// [ ]                        _____ [*] __|
            // clang-format on

            // To now how much to shift to the bottom of the trade side
            // we consider the following:
            // X - I = Y - J
            //
            // And a shift is:
            // X - Y = I - J
            const auto shift_size = new_top_lvl_virtual_index_in_old_range
                                    - ( hot_storage_half_size() - 1 );

            assert( shift_size < m_hot_levels.size() );

            slide_hot_storage_down( shift_size );
            return;
        }
        else
        {
            // CASE 2:
            // Here: it means we have only a single hot level with orders.
            //       and now that level became empty.
            //       And a new top level can only be found in cold storage.

            // We have 2 subcases here:
            // 2.1. we got levels in cold storage (cold storage non-empty);
            // 2.2. cold storage is empty (it means that the order table is empty).

            // In both cases we will ned the following value:
            // Head price for the range at the extreme position for a given trade
            // side:
            //            xxxxxxxxxxx
            //  Prices    xxxxxxxxxxx xxx - virtual prices,
            //  out of    xxxxxxxxxxx       not representable
            //  int64     xxxxxxxxxxx       with int64
            //            xxxxxxxxxxx
            //            ___###_____         @@@
            //            ___###_____         @@@   FWD
            //            ___###_____         @@@   Direction
            //            ___###_____         @@@
            //            ___###_____       @@@@@@@
            //            ___###_____        @@@@@
            //            ___###_____         @@@
            //    HEAD => ___###_____          @
            //            ____E______  ********************
            //            ___________  * SEll TRADE  SIDE *
            //            ___________  ********************
            //
            //            ...
            //            ...
            //            ...
            //
            //
            //            ___________  ********************
            //            ___________  * BUY  TRADE  SIDE *
            //            ____E______  ********************
            //    HEAD => ___###_____          @
            //            ___###_____         @@@
            //            ___###_____        @@@@@
            //            ___###_____       @@@@@@@
            //            ___###_____         @@@
            //            ___###_____         @@@  FWD
            //   EXTREME  ___###_____         @@@  Direction
            //   MIN      ___###_____         @@@
            //   Value => ___###_____
            //           /xxxxxxxxxxx
            //  Prices  / xxxxxxxxxxx xxx - virtual prices,
            //  out of /  xxxxxxxxxxx       not representable
            //  int64     xxxxxxxxxxx       with int64
            //            xxxxxxxxxxx
            const auto edge_case_head_price = price_ops_t{}.advance_forward(
                price_ops_t::max_value,
                order_price_t{
                    static_cast< order_price_t::value_type >( m_hot_levels.size() )
                    - 1 } );

            if( !m_cold_levels.empty() ) [[likely]]
            {
                // CASE 2.1

                // In this case we have at least 1 level in a cold storage.
                auto cold_it = m_cold_levels.begin();
                // And that would be our new top price
                const auto new_top_price = cold_it->first;

                // Calculate the new head price so that
                // the first cold level will be at the bottom of the
                // first half in newly created hot storage.
                //       _____
                // [HEAD]
                // [    ] Q1                                      @
                // [    ]_____                                   @@@
                // [    ]                                       @@@@@
                // [    ] Q2                                   @@@@@@@
                // [ TOP]_____  <=== Consider move hot levels    @@@
                // [    ]            so that new top is here.    @@@  FWD
                // [    ] Q3                                     @@@  Direction
                // [    ]_____                                   @@@
                // [    ]
                // [    ] Q4
                // [    ]_____

                order_price_t head_price_offset_to_middle{
                    hot_storage_half_size< order_price_t::value_type >() - 1
                };

                const auto extreme_case_middle_price =
                    price_ops_t{}.advance_backward( edge_case_head_price,
                                                    head_price_offset_to_middle );

                const order_price_t legit_head_price =
                    price_ops_t{}.lt( new_top_price, extreme_case_middle_price ) ?
                        // CASE 2.1.1 New range position
                        //            is not hitting extreme case
                        price_ops_t{}.advance_forward(
                            new_top_price, head_price_offset_to_middle )
                        // CASE 2.1.2 New range position goes to extremes.
                        :
                        edge_case_head_price;

                m_top_level_virtual_index =
                    static_cast< unsigned >( price_ops_t{}.safe_u64_distance(
                        legit_head_price, new_top_price ) );

                auto p = legit_head_price;
                for( auto & lvl : m_hot_levels )
                {
                    const auto price =
                        std::exchange( p, price_ops_t{}.advance_backward( p ) );

                    if( end( m_cold_levels ) != cold_it
                        && cold_it->first == price )
                    {
                        lvl = std::move( cold_it++->second );
                    }
                    else
                    {
                        lvl = this->m_book_private_data.price_levels_factory
                                  .make_price_level( price );
                    }
                }

                // Erase levels that were moved to hot storage.
                m_cold_levels.erase( begin( m_cold_levels ), cold_it );

                m_hot_head_real_index = 0;
            }
            else
            {
                // Completely empty.
                // Let's recreate the initial state.
                reset_hot_storage_initial_state( edge_case_head_price );
                m_top_level_virtual_index =
                    static_cast< unsigned >( hot_storage_size() ) - 1;
            }
        }
    }

    /**
     * @brief Handle a price level becoming empty
     */
    void retire_level( order_price_t price )
    {
        const auto head_level_price =
            m_hot_levels[ m_hot_head_real_index ].price();

        assert( price_ops_t{}.le( head_level_price, price ) );

        const auto virtual_index_in_hot =
            price_ops_t{}.safe_u64_distance( head_level_price, price );

        assert( virtual_index_in_hot >= m_top_level_virtual_index );

        if( m_top_level_virtual_index == virtual_index_in_hot )
        {
            // Retire current top level.
            drop_hot_top_level();
            return;
        }
        else if( virtual_index_in_hot >= hot_storage_size< std::uint64_t >() )
        {
            assert( 0 != m_cold_levels.count( price ) );
            m_cold_levels.erase( price );
        }
    }

    /**
     * @brief A storage of price levels in a book.
     *
     * Vector is a memory arrangement to keep the hot price levels together.
     * But semantically the logical head of the list fluctuates.
     * That means that the position where the head might be in the middle of the
     * vector. That is because hot levels are basically a sliding window
     * on the tape of all prices. And on event the "window" slides
     * we prefer not to rellocate all elemnts so that the head is alway at
     * pos=0.
     *
     * Here is an example (buy side, count of_hot levels=5):
     * @code
     * Adding an order at price level 102.
     * Current best bid is 100, so we need to add 2 new levels (101, 102)
     * // Logical picture.
     * // BEFORE   AFTER
     *    100      102
     *    99       101
     *    98       100
     *    97       99
     *    96       98
     *            <97> goes to cold (if not mpty)
     *            <96> goes to cold (if not mpty)
     * // Vector modification
     * Before:
     * [0*]  [1]  [2]  [3]  [4]      * - head_pos=0
     * 100   99   98   97   96
     *
     * After:
     * [0]   [1]  [2]  [3*] [4]      * - head_pos=3
     * 100   99   98   102  101
     * @endcode
     *
     * Moving window towards this side "lower" prices is less straightforward
     * in terms of what trigers it but the same adjustments to
     * what is the header are applied.
     */
    std::vector< price_level_t > m_hot_levels;

    /**
     * @brief What is the position of a current head in the array of
     *        hot price levels.
     */
    unsigned m_hot_head_real_index{};

    /**
     * @brief A position (relative to head) of the first price level that
     *        has an order o it (BBO).
     */
    unsigned m_top_level_virtual_index{};

    /**
     * @brief The bitmask for quick real position calculations
     *        in the hot levels starage.
     */
    std::size_t m_hot_levels_mask;

    using cold_comparator_t = typename price_ops_t::cmp_t;

    /**
     * @brief A type alias for cold' levels' storage.
     */
    using cold_levels_table_t =
        std::map< order_price_t, price_level_t, cold_comparator_t >;

    /**
     * @brief A storage for a cold price levels.
     *
     * Stores orders that do not fit into hot range.
     * Levels in that storage must contain orders either orders or
     * meaningfull stats (executed orders numbers).
     */
    cold_levels_table_t m_cold_levels;
};

}  // namespace jacobi::book::mixed::hot_cold
