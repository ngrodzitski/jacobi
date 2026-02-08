#include <chrono>
#include <regex>

#include <CLI/CLI.hpp>

#include "../benchmark_helpers.hpp"
#include "./benchmark_latency_functions.hpp"
#include "./benchmark_generator.hpp"

namespace jacobi::bench
{

//
// bench_root()
//

template < typename Book_Traits >
void bench_root( std::string_view benchmark_name,
                 std::span< const snapshots::update_record_image_t > events,
                 std::pair< std::size_t, std::size_t > rng,
                 std::size_t measurements_count )
{
    using book_t = book::std_book_t< Book_Traits >;

#if !defined( JACOBI_MIXED_HOT_COLD )
    using book_init_params_t = book::std_book_init_params_t< Book_Traits >;
    book_init_params_t params{};
#else
    using book_init_params_t =
        book::mixed::hot_cold::std_book_init_params_t< Book_Traits >;
    book_init_params_t params{ get_hot_storage_size() };
#endif  // !defined( JACOBI_MIXED_HOT_COLD)

    auto measurements =
        single_book_latency_benchmark< book_t, book_init_params_t >(
            events.subspan( 0, rng.second ),
            rng.first,
            params,
            measurements_count );

    const auto stats = analyze_latency_measurements( measurements );
    print_latency_stats( benchmark_name, stats );
}

namespace /* anonymous */
{

//
// config_t
//

/**
 * @brief Application config.
 */
struct config_t
{
    std::string events_file_path;
    std::vector< std::size_t > range;  // [A, B)
    std::size_t measurements_count = 100'000ULL;
    std::string filter_regex;
};

//
// run_benchmarks()
//

void run_benchmarks( const config_t & cfg )
{
    const auto events = snapshots::read_events_from_file( cfg.events_file_path );

    if( events.empty() )
    {
        throw std::runtime_error{ "Failed to run a benchmark with empty file: "
                                  + cfg.events_file_path };
    }

    const auto r = std::make_pair( cfg.range.at( 0 ), cfg.range.at( 1 ) );

    const std::regex filter( cfg.filter_regex );

    fmt::print( "{}\n", make_latency_stats_header() );

    JACOBI_GENERATE_BENCHMARKS_XXX_STORAGE(
        jacobi::bench::bench_root, events, r, cfg.measurements_count, filter );
}

}  // anonymous namespace

}  // namespace jacobi::bench

//
// main()
//

int main( int argc, char * argv[] )
{
    try
    {
        CLI::App app{ "JACOBI latency benchmark" };
        jacobi::bench::config_t cfg;

        // Command Line Options

        // 1. Events Data File (Required)
        app.add_option(
               "-f,--file", cfg.events_file_path, "Path to events data file" )
            ->required()
            ->check( CLI::ExistingFile );

        // 2. Range (Optional, 2 integers)
        app.add_option(
               "-r,--range", cfg.range, "Range [A, B) of events to measure" )
            ->expected( 2 );

        // Measurements Count (Default 100K, Min 100K)
        app.add_option(
               "-n,--count", cfg.measurements_count, "Number of measurements" )
            ->default_val( size_t{ 100'000 } )
            ->check( CLI::Range( size_t{ 100'000 },
                                 std::numeric_limits< size_t >::max() ) );

        // 4. Filter (Regex)
        app.add_option( "--benchmark_filter",
                        cfg.filter_regex,
                        "Regex filter for benchmark names" );

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

        jacobi::bench::run_benchmarks( cfg );
    }
    catch( const std::exception & ex )
    {
        fmt::print( "Benchmark failed: {}", ex.what() );
        return 1;
    }

    return 0;
}
