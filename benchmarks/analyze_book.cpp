#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <CLI/CLI.hpp>

#include <boost/unordered/unordered_flat_map.hpp>

#include <jacobi/snapshots/events_snapshots.hpp>

#include <jacobi/book/vocabulary_types.hpp>

#include "map_levels_storage_book_types.hpp"

namespace jacobi::benchmark
{

using book_traits_t = bench::bsn1_plvl11_refIX3_t;
using book_t        = book::std_book_t< book_traits_t >;
//
// config_t
//

struct config_t
{
    std::string input_file_path;
    std::vector< std::size_t > range;  // [A, B)
    bool all_time_only{ false };
    std::uint32_t min_diff_top    = 0;
    std::uint32_t min_diff_bottom = 0;
    std::uint32_t book_id         = 1;
};

//
// orders_table_properties_t
//

struct orders_table_properties_t
{
    book::order_price_t::value_type top_diff{};
    book::order_price_t::value_type bottom_diff{};

    std::optional< book::order_price_t > top;
    std::optional< book::order_price_t > bottom;
    std::optional< book::order_price_t > all_time_top;
    std::optional< book::order_price_t > all_time_bottom;

    /**
     * @brief Update metrics with a given book state.
     *
     * @return  Two bool flags: top-bottom changed, all-time top-bottom changed.
     */
    template < typename Orders_Table >
    std::pair< bool, bool > update_from( Orders_Table & table )
    {
        book::order_price_operations_t< Orders_Table::trade_side_indicator > ops;

        auto prev_top    = table.top_price();
        auto prev_bottom = prev_top;

        // Just a big number as default.
        top_diff    = 1'000'000'000;
        bottom_diff = 1'000'000'000;

        if( prev_top )
        {
            auto lvls_range = table.levels_range();
            auto it         = lvls_range.end();
            --it;
            prev_bottom = it->price();

            if( top )
            {
                top_diff    = std::abs( type_safe::get( *top - *prev_top ) );
                bottom_diff = std::abs( type_safe::get( *bottom - *prev_bottom ) );
            }
        }

        std::swap( prev_top, top );
        std::swap( prev_bottom, bottom );

        const bool changed    = top != prev_top || bottom != prev_bottom;
        bool all_time_changed = false;

        if( changed )
        {
            const auto candidate_all_time_top =
                ops.min( all_time_top.value_or( *top ), *top );
            const auto candidate_all_time_bottom =
                ops.max( all_time_bottom.value_or( *bottom ), *bottom );

            all_time_changed = all_time_top != candidate_all_time_top
                               || all_time_bottom != candidate_all_time_bottom;

            all_time_top    = candidate_all_time_top;
            all_time_bottom = candidate_all_time_bottom;
        }

        return std::make_pair( changed, all_time_changed );
    }

    [[nodiscard]] std::string to_string() const
    {
        std::string res;

        auto dump = [ & ]( auto t, auto b )
        {
            if( t )
            {
                res += fmt::format( "[{:>6} : {:>6} : {:>6}]",
                                    type_safe::get( *t ),
                                    type_safe::get( *b ),
                                    std::abs( type_safe::get( *t - *b ) ) );
            }
            else
            {
                res += fmt::format( "[{:>6} : {:>6} : {:>6}]", '-', '-', '-' );
            }
        };

        dump( top, bottom );
        res += "  ";
        dump( all_time_top, all_time_bottom );

        return res;
    }
};

//
// process_files()
//

void analyze_book( const config_t & cfg )
{
    fmt::print( "--- Processing ---\n"
                "Input:   {}\n"
                "book_id: {}\n"
                "range:  {{{}}}\n",
                cfg.input_file_path,
                cfg.book_id,
                fmt::join( cfg.range, ", " ) );

    const auto events = snapshots::read_events_from_file( cfg.input_file_path );

    auto range = cfg.range;
    if( range.empty() )
    {
        range.push_back( 0 );
        range.push_back( events.size() );
    }
    else
    {
        if( range[ 0 ] >= events.size() )
        {
            throw std::runtime_error( fmt::format(
                "invalid range, file events count: {}", events.size() ) );
        }
        if( range[ 1 ] >= events.size() )
        {
            range[ 1 ] = events.size();
        }
    }

    book_t book{};

    auto i = 0ULL;
    for( ; i < range[ 0 ]; ++i )
    {
        handle_single_event( book, events[ i ] );
    }

    orders_table_properties_t buy_properties;
    orders_table_properties_t sell_properties;

    buy_properties.update_from( book.buy() );
    sell_properties.update_from( book.sell() );

    auto prev_i = i;

    auto dump_book_metrics = [ & ]
    {
        fmt::print( "{:>8}( +{:<6} )   B {:<46}        S {}\n",
                    i,
                    i - prev_i,
                    buy_properties.to_string(),
                    sell_properties.to_string() );
    };

    dump_book_metrics();

    for( ; i < range[ 1 ]; ++i )
    {
        handle_single_event( book, events[ i ] );

        const auto buy_update_flags  = buy_properties.update_from( book.buy() );
        const auto sell_update_flags = sell_properties.update_from( book.sell() );

        if( ( !cfg.all_time_only
              && ( buy_update_flags.first || sell_update_flags.first ) )
            || ( cfg.all_time_only
                 && ( buy_update_flags.second || sell_update_flags.second ) ) )
        {
            const auto max_diff_top =
                std::max( buy_properties.top_diff, sell_properties.top_diff );
            const auto max_diff_bottom = std::max( buy_properties.bottom_diff,
                                                   sell_properties.bottom_diff );

            bool dump_me =
                ( cfg.min_diff_top == 0 && cfg.min_diff_bottom == 0 )
                | ( cfg.min_diff_top != 0 && cfg.min_diff_top <= max_diff_top )
                | ( cfg.min_diff_bottom != 0
                    && cfg.min_diff_bottom <= max_diff_bottom );

            if( dump_me )
            {
                dump_book_metrics();
                prev_i = i;
            }
        }
    }
}

}  // namespace jacobi::benchmark

//
// main()
//
int main( int argc, char * argv[] )
{
    try
    {
        CLI::App app{ "Analyze book" };
        jacobi::benchmark::config_t cfg;

        app.add_option( "-i,--input", cfg.input_file_path, "Path to input file" )
            ->required()
            ->check( CLI::ExistingFile );

        app.add_option(
               "-r,--range", cfg.range, "Range [A, B) of events to measure" )
            ->expected( 2 );

        app.add_flag( "-a,--all-time-only",
                      cfg.all_time_only,
                      "Track only all-time metrics changes" );

        app.add_option( "--diff-top-filter",
                        cfg.min_diff_top,
                        "Filter out little differences in top/bottom change" )
            ->default_val( cfg.min_diff_top );

        app.add_option( "--diff-bottom-filter",
                        cfg.min_diff_bottom,
                        "Filter out little differences in top/bottom change" )
            ->default_val( cfg.min_diff_bottom );

        app.add_option( "-b,--book-id", cfg.book_id, "Book ID (default: 1)" )
            ->default_val( cfg.book_id );

        CLI11_PARSE( app, argc, argv );

        if( !cfg.range.empty() )
        {
            // Range must be sane: A < B
            if( cfg.range[ 0 ] >= cfg.range[ 1 ] )
            {
                throw std::runtime_error(
                    "Bad range [A, B): A must be strictly less than B" );
            }
        }

        jacobi::benchmark::analyze_book( cfg );
    }
    catch( const std::exception & ex )
    {
        fmt::print( "Error: {}\n", ex.what() );
        return 1;
    }

    return 0;
}
