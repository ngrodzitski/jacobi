#include <jacobi/snapshots/events_snapshots.hpp>

#include <fstream>
#include <stdexcept>

namespace jacobi::snapshots
{

//
// read_events_from_file()
//

std::vector< update_record_image_t > read_events_from_file(
    const std::string & filename )
{
    std::ifstream file( filename, std::ios::binary | std::ios::ate );

    if( !file.is_open() )
    {
        throw std::runtime_error( "Failed to open file: " + filename );
    }

    const std::streamsize file_size = file.tellg();
    const size_t record_size        = sizeof( update_record_image_t );

    // Ensure file size is a multiple of the record size
    if( 0 != ( file_size % record_size ) )
    {
        throw std::runtime_error( "bad file format, size must be 32*N bytes" );
    }

    // Calculate number of records and reserve memory
    const auto record_count =
        static_cast< std::size_t >( file_size ) / record_size;

    std::vector< update_record_image_t > result( record_count );

    if( 0 != record_count )
    {
        // Bulk read the whole data.
        file.seekg( 0, std::ios::beg );
        if( !file.read( reinterpret_cast< char * >( result.data() ), file_size ) )
        {
            throw std::runtime_error( "Failed to read book-events-snapshot data from "
                                      + filename );
        }
    }
    return result;
}

}  // namespace jacobi::snapshots
