#include "../benchmark_helpers.hpp"
#include "./benchmark_throughput_functions.hpp"
#include "./benchmark_generator.hpp"

namespace jacobi::bench
{

//
// bench_root()
//

template < typename Book_Traits >
void bench_root( benchmark::State & state )
{
    try
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

        const auto events_file = get_events_file_name();
        const auto events      = snapshots::read_events_from_file( events_file );

        if( events.empty() )
        {
            throw std::runtime_error{ "Failed to run a benchmark with empty file: "
                                      + events_file };
        }

        multi_book_benchmark< book_t, book_init_params_t >(
            state,
            events_file,
            events,
            get_events_range(
                std::make_pair< std::size_t, std::size_t >( 0, events.size() ) ),
            params,
            get_profiling_mode() );
    }
    catch( const std::exception & ex )
    {
        state.SkipWithError( ex.what() );
    }
}

JACOBI_GENERATE_BENCHMARKS_XXX_STORAGE( bench_root )

}  // namespace jacobi::bench

BENCHMARK_MAIN();
