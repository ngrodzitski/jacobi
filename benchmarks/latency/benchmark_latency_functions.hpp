#pragma once

#include <chrono>

namespace jacobi::bench
{

//
// latency_stats_t
//

struct latency_stats_t
{
    unsigned p50_nsec;
    unsigned p90_nsec;
    unsigned p99_nsec;
    unsigned p999_nsec;
    unsigned p9999_nsec;
    unsigned p99999_nsec;
    unsigned max_nsec;

    double mean;
    double std_dev;
};

//
// analyze_latency_measurements()
//

/**
 * @brief Calculates stats for a given sequence of measurements.
 *
 * @note: Modifies the input span (sorts it) to calculate percentiles.
 */
[[nodiscard]] inline latency_stats_t analyze_latency_measurements(
    std::span< unsigned > measurements )
{
    latency_stats_t res{};

    if( !measurements.empty() )
    {
        // Percentiles:
        std::sort( measurements.begin(), measurements.end() );

        auto get_raw_percentile = [ & ]( double p )
        {
            if( p < 0.0 || p > 1.0 ) return 0U;

            auto idx =
                static_cast< std::size_t >( std::ceil( p * measurements.size() ) );
            idx = std::min( idx, measurements.size() - 1 );
            return measurements[ idx ];
        };

        res.p50_nsec    = get_raw_percentile( 0.50 );
        res.p90_nsec    = get_raw_percentile( 0.90 );
        res.p99_nsec    = get_raw_percentile( 0.99 );
        res.p999_nsec   = get_raw_percentile( 0.999 );
        res.p9999_nsec  = get_raw_percentile( 0.9999 );
        res.p99999_nsec = get_raw_percentile( 0.99999 );
        res.max_nsec    = measurements.back();

        // Normal Distribution Params
        const auto sum =
            std::accumulate( measurements.begin(), measurements.end(), 0ULL );
        res.mean = static_cast< double >( sum / measurements.size() )
                   + static_cast< double >( sum % measurements.size() )
                         / measurements.size();

        // Variance calculation: sum((x - mean)^2) / N
        double sq_diff_sum = 0.0;
        for( const auto & val : measurements )
        {
            double diff = static_cast< double >( val ) - res.mean;
            sq_diff_sum += ( diff * diff );
        }
        const double variance = sq_diff_sum / measurements.size();
        res.std_dev           = std::sqrt( variance );
    }

    return res;
}

/**
 * @brief Calculates and prints stats for a given sequence of measurements.
 *
 * @note: Modifies the input span (sorts it) to calculate percentiles.
 */
inline void print_latency_stats( std::string_view title, latency_stats_t stats )
{
    // Helper to convert ns to us (microseconds) for display
    auto to_us = []( auto ns ) -> double { return ns / 1000.0; };

    fmt::print(
        "{:20} {:20} PX: {:>9} {:>9} {:>9} {:>9} {:>9} {:>9} {:>12}\n",
        fmt::format( "{}:", title ),
        fmt::format(
            "N({:.3f}, {:.3f});", to_us( stats.mean ), to_us( stats.std_dev ) ),
        to_us( stats.p50_nsec ),
        to_us( stats.p90_nsec ),
        to_us( stats.p99_nsec ),
        to_us( stats.p999_nsec ),
        to_us( stats.p9999_nsec ),
        to_us( stats.p99999_nsec ),
        to_us( stats.max_nsec ) );
}

//
// print_latency_stats_header()
//

[[nodiscard]] inline std::string make_latency_stats_header()
{
    return fmt::format(
        "{:20} {:20}     {:>9} {:>9} {:>9} {:>9} {:>9} {:>9} {:>12}",
        "Benchmark",
        "Distribution",
        "P50",
        "P90",
        "P99",
        "P999",
        "P9999",
        "P99999",
        "MAX" );
}

//
// single_book_latency_benchmark()
//

/**
 * @brief Run latency benchmark for single book.
 *
 */
template < typename Book_Type, typename Book_Params >
std::vector< unsigned > single_book_latency_benchmark(
    std::span< const snapshots::update_record_image_t > events,
    std::size_t skip_first_n,
    const Book_Params & book_params,
    std::size_t measurements_count )
{
    if( 100'000ULL > measurements_count )
    {
        throw std::runtime_error{ "Measurements count must be at least 100K" };
    }

    std::vector< unsigned > res;
    res.resize( measurements_count );

    const auto measurable_events = events.subspan( skip_first_n );
    if( measurable_events.empty() )
    {
        throw std::runtime_error{ "measurable range cannot be empty" };
    }

    for( auto i = 0ULL; i < measurements_count; ++i )
    {
        Book_Type book{ book_params };
        for( const auto event : events.subspan( 0, skip_first_n ) )
        {
            handle_single_event( book, event );
        }

        for( auto j = 0ULL; j < measurable_events.size() && i < measurements_count;
             ++j, ++i )
        {
            const auto start = std::chrono::steady_clock::now();
            handle_single_event( book, measurable_events[ j ] );
            auto diff = std::chrono::steady_clock::now() - start;

            using std::chrono::duration_cast;
            using std::chrono::nanoseconds;
            res[ i ] = duration_cast< nanoseconds >( diff ).count();
        }
    }

    return res;
}

}  // namespace jacobi::bench
