#pragma once

#include <stdexcept>

#include <fmt/format.h>

#include <benchmark/benchmark.h>

#include <jacobi/snapshots/events_snapshots.hpp>

#include "../profiling.hpp"

namespace jacobi::bench
{

namespace details
{

[[nodiscard]] inline bool full_file_is_measured(
    std::size_t n,
    std::pair< std::size_t, std::size_t > rng )
{
    return 0 == rng.first && n == rng.second;
}

}  // namespace details

//
// single_book_benchmark()
//

/**
 * @brief Run a benchmark for single book.
 *
 * @note If profiling is enabled and full file is measured
 *       the whole run is profiled as a single process.
 *       It allows to run long measurements. But at the cost of more noise
 *       because it would also measure the preparation and cleanup
 *       routines.
 *
 * @note If profiling is enabled and only a range is measured
 *       then each range-handling run is measured.
 *       That means short intervals of measurements.
 *       Beacuse even if the range is of size 100'000'000
 *       (at the moment of initial development the max events count
 *       for a single book available for testing was less than 4M)
 *       then the measurements would take a few seconds at most.
 */
template < typename Book_Type, typename Book_Params >
void single_book_benchmark(
    benchmark::State & state,
    std::string_view events_file_name,
    std::span< const snapshots::update_record_image_t > events,
    std::pair< std::size_t, std::size_t > measured_range,
    const Book_Params & book_params,
    int profiling_mode = 0 )
{

    if( measured_range.first >= events.size()
        || measured_range.second > events.size()
        || measured_range.first >= measured_range.second )
    {
        throw std::runtime_error{ fmt::format(
            "Failed to run a benchmark for range[{}, {}), records in file: {}",
            measured_range.first,
            measured_range.second,
            events.size() ) };
    }

    if( details::full_file_is_measured( events.size(), measured_range ) )
    {
        state.SetLabel(
            fmt::format( "{}({}K)", events_file_name, events.size() / 1'000ULL ) );

        const pid_t perf_pid = start_perf_profiling( profiling_mode );

        for( auto _ : state )
        {
            state.PauseTiming();
            {
                // =======================
                // DO NOT COUNT BOOK CREATION.
                Book_Type book{ book_params };
                // =======================
                state.ResumeTiming();

                for( const auto ev : events )
                {
                    handle_single_event( book, ev );
                }

                state.PauseTiming();
                // =======================
                // DO NOT COUNT BOOK destruction.
            }
            // =======================
            state.ResumeTiming();
        }

        stop_perf_profiling( perf_pid );

        state.SetItemsProcessed( state.iterations() * events.size() );
    }
    else
    {
        state.SetLabel( fmt::format(
            "{}({}K,r=[{}, {}), {}K)",
            events_file_name,
            events.size() / 1'000ULL,
            measured_range.first,
            measured_range.second,
            ( measured_range.second - measured_range.first ) / 1'000ULL ) );

        for( auto _ : state )
        {
            state.PauseTiming();

            {
                // =======================
                // DO NOT COUNT BOOK CREATION AND FIRST N EVENTS.
                Book_Type book{ book_params };

                for( auto i = 0ULL; i < measured_range.first; ++i )
                {
                    handle_single_event( book, events[ i ] );
                }

                const pid_t perf_pid = start_perf_profiling( profiling_mode );

                // =======================
                state.ResumeTiming();

                for( auto i = measured_range.first; i < measured_range.second;
                     ++i )
                {
                    handle_single_event( book, events[ i ] );
                }

                state.PauseTiming();
                stop_perf_profiling( perf_pid );
                // =======================
                // DO NOT COUNT BOOK destruction.
            }
            // =======================
            state.ResumeTiming();
        }

        state.SetItemsProcessed(
            state.iterations()
            * ( measured_range.second - measured_range.first ) );
    }
}

//
// multi_book_benchmark()
//

/**
 * @brief Run a benchmark for a multiple books.
 *
 * @note If profiling is enabled each range-handling run is measured.
 *       That means short intervals of measurements.
 *       Beacuse even if the range is of size 100'000'000
 *       (at the moment of initial development the max events count
 *       for a single book available for testing was less than 4M)
 *       then the measurements would take a few seconds at most.
 */
template < typename Book_Type, typename Book_Params >
void multi_book_benchmark(
    benchmark::State & state,
    std::string_view events_file_name,
    std::span< const snapshots::update_record_image_t > events,
    std::pair< std::size_t, std::size_t > measured_range,
    const Book_Params & book_params,
    int profiling_mode = 0 )
{
    if( measured_range.first >= events.size()
        || measured_range.second > events.size()
        || measured_range.first >= measured_range.second )
    {
        throw std::runtime_error{ fmt::format(
            "Failed to run a benchmark for range[{}, {}), records in file: {}",
            measured_range.first,
            measured_range.second,
            events.size() ) };
    }

    const std::size_t books_count =
        1
        + std::max_element( events.begin(),
                            events.end(),
                            []( const auto & r1, const auto & r2 )
                            { return r1.book_id < r2.book_id; } )
              ->book_id;

    if( books_count > 500 )
    {
        throw std::runtime_error{ fmt::format(
            "{} is too many books, file {} ", books_count, events_file_name ) };
    }

    std::vector< Book_Type > books;
    books.reserve( books_count );

    if( details::full_file_is_measured( events.size(), measured_range ) )
    {
        state.SetLabel(
            fmt::format( "{}({}K)", events_file_name, events.size() / 1'000ULL ) );

        const pid_t perf_pid = start_perf_profiling( profiling_mode );

        for( auto _ : state )
        {
            state.PauseTiming();
            {
                // =======================
                // DO NOT COUNT BOOK CREATION.
                for( auto i = 0U; i < books_count; ++i )
                {
                    books.emplace_back( book_params );
                }
                // =======================
                state.ResumeTiming();

                for( auto i = 0ULL; i < events.size(); ++i )
                {
                    handle_single_event( books[ events[ i ].book_id ],
                                         events[ i ] );
                }

                state.PauseTiming();
                // =======================
                // DO NOT COUNT BOOK destruction.
            }
            // =======================
            state.ResumeTiming();
        }

        stop_perf_profiling( perf_pid );

        state.SetItemsProcessed( state.iterations() * events.size() );
    }
    else
    {
        state.SetLabel( fmt::format(
            "{}({}K,r=[{}, {}), {}K)",
            events_file_name,
            events.size() / 1'000ULL,
            measured_range.first,
            measured_range.second,
            ( measured_range.second - measured_range.first ) / 1'000ULL ) );

        for( auto _ : state )
        {
            state.PauseTiming();

            {
                // =======================
                // DO NOT COUNT BOOK CREATION AND FIRST N EVENTS.
                for( auto i = 0U; i < books_count; ++i )
                {
                    books.emplace_back( book_params );
                }

                for( auto i = 0ULL; i < measured_range.first; ++i )
                {
                    handle_single_event( books[ events[ i ].book_id ],
                                         events[ i ] );
                }

                const pid_t perf_pid = start_perf_profiling( profiling_mode );

                // =======================
                state.ResumeTiming();

                for( auto i = measured_range.first; i < measured_range.second;
                     ++i )
                {
                    handle_single_event( books[ events[ i ].book_id ],
                                         events[ i ] );
                }

                state.PauseTiming();
                stop_perf_profiling( perf_pid );
                books.clear();
                // =======================
                // DO NOT COUNT BOOK destruction.
            }
            // =======================
            state.ResumeTiming();
        }

        state.SetItemsProcessed(
            state.iterations()
            * ( measured_range.second - measured_range.first ) );
    }
}

}  // namespace jacobi::bench
