#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <fmt/format.h>

#include <CLI/CLI.hpp>

#include <boost/unordered/unordered_flat_map.hpp>

#include <jacobi/snapshots/events_snapshots.hpp>

#include <jacobi/book/vocabulary_types.hpp>

namespace jacobi::benchmark
{

//
// config_t
//

struct config_t
{
    std::string input_file_path;
    std::string output_file_path;
    std::uint64_t id_base = 1;
};

//
// process_files()
//

void reset_ids( const config_t & cfg )
{

    fmt::print( "--- Processing ---\n"
                "Input:   {}\n"
                "Output:  {}\n"
                "ID Base: {}\n",
                cfg.input_file_path,
                cfg.output_file_path,
                cfg.id_base );

    auto events = snapshots::read_events_from_file( cfg.input_file_path );

    using id_mapping_t =
        boost::unordered::unordered_flat_map< book::order_id_t,
                                              std::uint64_t,
                                              book::order_id_custom_hash_t >;

    std::srand( std::time( {} ) );
    id_mapping_t ids;
    auto id_mapper = [ &, next_id = cfg.id_base ]( auto id ) mutable
    {
        auto it = ids.find( id );
        if( ids.end() == it )
        {
            using vtype_t     = id_mapping_t::value_type;
            const auto new_id = next_id++;
            // make ids more interesting.
            next_id += std::rand() % 1000;
            it = ids.insert( vtype_t{ id, new_id } ).first;
        }
        return it->second;
    };

    for( auto & ev : events )
    {
        const std::uint64_t effective_id = id_mapper( ev.order_id() );
        ev.u.common.id                   = effective_id;
    }

    std::ofstream output( cfg.output_file_path,
                          output.binary | output.trunc | output.out );
    if( !output.is_open() )
    {
        throw std::runtime_error( "Failed to open output file: "
                                  + cfg.output_file_path );
    }

    output.write( (const char *)events.data(),
                  events.size() * sizeof( snapshots::update_record_image_t ) );

    output.close();

    fmt::print( "Processing complete!\n"
                "ids mapped: {}\n",
                ids.size() );
}

}  // namespace jacobi::benchmark

//
// main()
//
int main( int argc, char * argv[] )
{
    try
    {
        CLI::App app{ "Update IDs Util" };
        jacobi::benchmark::config_t config;

        app.add_option(
               "-i,--input", config.input_file_path, "Path to input file" )
            ->required()
            ->check( CLI::ExistingFile );

        app.add_option(
               "-o,--output", config.output_file_path, "Path to output file" )
            ->required();

        app.add_option( "-b,--base", config.id_base, "Base value to reset IDs" )
            ->default_val( config.id_base );

        CLI11_PARSE( app, argc, argv );

        jacobi::benchmark::reset_ids( config );
    }
    catch( const std::exception & ex )
    {
        fmt::print( "Error: {}\n", ex.what() );
        return 1;
    }

    return 0;
}
