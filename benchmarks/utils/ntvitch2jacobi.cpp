#include <stdexcept>
#include <map>

#include <CLI/CLI.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/counter.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/pipeline.hpp>

#include <jacobi/snapshots/events_snapshots.hpp>

#include <jacobi/ntvitch/ntvitch.hpp>
#include <jacobi/ntvitch/ntvitch_fmt.hpp>

#include "../map_levels_storage_book_types.hpp"

namespace jacobi::benchmark
{

//
// config_t
//

struct config_t
{
    std::string input_file_path;
    std::string output_dir;
    std::string out_file_suffix;
    std::uint64_t filter_min_events{};
};

using book_traits_t = bench::bsn1_plvl11_refIX3_t;
using book_t        = book::std_book_t< book_traits_t >;

std::int64_t normalize_price( std::uint32_t p )
{
    if( p <= 10'000 ) return p;

    return 10'000LL + ( p - 10'000LL ) / 100LL;
}

//
// books_context_t
//

struct books_context_t
{
    explicit books_context_t() { books.reserve( 10'000ULL ); }

    struct book_item_data_t
    {
        std::string symbol;
        book_t book;

        template < typename Msg >
        auto trade_side( book::order_id_t id, const Msg * msg ) const
        {
            const auto & order_refs_index = book.order_refs_index();

            const auto order_ref_it = order_refs_index.find( id );

            if( order_ref_it == order_refs_index.end() )
            {
                throw std::runtime_error(
                    fmt::format( "invalid order_id={} for '{}' book, msg: {}",
                                 id,
                                 symbol,
                                 *msg ) );
            }

            return order_refs_index.access_value( order_ref_it )->get_trade_side();
        }

        book::order_t get_order( book::order_id_t id ) const
        {
            const auto & order_refs_index = book.order_refs_index();

            const auto order_ref_it = order_refs_index.find( id );

            if( order_ref_it == order_refs_index.end() )
            {
                throw std::runtime_error( fmt::format(
                    "invalid order_id={} for '{}' book", id, symbol ) );
            }

            return order_refs_index.access_value( order_ref_it )->access_order();
        }
    };

    std::vector< book_item_data_t > books;
    std::vector< snapshots::update_record_image_t > all_events;

    /**
     * @brief Filter books that has not enough events.
     *
     * Removes events for books that have less events than @c min_events_cnt.
     */
    std::vector< snapshots::update_record_image_t > filter_events(
        std::size_t min_events_cnt ) const
    {
        auto events = all_events;

        const book::bsn_t min_bsn{ min_events_cnt };
        std::erase_if( events,
                       [ & ]( const auto & ev ) {
                           return books.at( ev.book_id - 1 ).book.bsn() < min_bsn;
                       } );

        std::vector< std::uint32_t > updated_ids( events.size(), 0 );

        std::uint32_t book_id_counter = 1;

        for( auto & ev : events )
        {
            const auto old_book_id = ev.book_id;
            if( updated_ids[ old_book_id ] == 0 )
            {
                updated_ids[ old_book_id ] = book_id_counter++;
            }
            ev.book_id = updated_ids[ old_book_id ];
        }

        return events;
    }

    std::map< std::string, std::vector< snapshots::update_record_image_t > >
    make_single_book_events( std::size_t min_events_cnt ) const
    {
        const book::bsn_t min_bsn{ min_events_cnt };

        std::vector< std::vector< snapshots::update_record_image_t > >
            events_sinks;
        events_sinks.resize( books.size() );

        for( auto i = 0ULL; i < books.size(); ++i )
        {
            if( books[ i ].book.bsn() >= min_bsn )
            {
                events_sinks[ i ].reserve( type_safe::get( min_bsn ) );
            }
        }

        for( const auto & ev : all_events )
        {
            if( books[ ev.book_id - 1 ].book.bsn() >= min_bsn )
            {
                events_sinks[ ev.book_id - 1 ].push_back( ev );
                events_sinks[ ev.book_id - 1 ].back().book_id = 1;
            }
        }

        std::map< std::string, std::vector< snapshots::update_record_image_t > >
            res;
        for( auto i = 0ULL; i < books.size(); ++i )
        {
            if( books[ i ].book.bsn() >= min_bsn )
            {
                res[ books[ i ].symbol ] = std::move( events_sinks[ i ] );
            }
        }
        return res;
    }

    /**
     * @brief Handlers for specific types of messages.
     */
    /// @{
    void process_message( const ntvitch::system_event_t * msg )
    {
        fmt::print( "\n[system_event] {}\n", *msg );
    }

    void process_message( const ntvitch::stock_directory_t * msg )
    {
        fmt::print( "\n[stock_directory] {}\n", *msg );
        if( books.size() < msg->stock_locate() )
        {
            books.resize( msg->stock_locate() );
        }

        books.at( msg->stock_locate() - 1 ).symbol =
            msg->stock().substr( 0, msg->stock().find( ' ' ) );
    }

    void process_message( const ntvitch::stock_trading_action_t * msg )
    {
        fmt::print( "\n[stock_trading_action] {}\n", *msg );
    }

    template < typename Msg >
    void process_add_order_message( const Msg * msg )
    {
        all_events.emplace_back( snapshots::update_record_image_t{} );
        auto & ev = all_events.back();

        ev.book_id = msg->stock_locate();
        ev.op_code =
            static_cast< std::uint8_t >( snapshots::book_operation::add_order );
        ev.ts = msg->buy_sell() == ntvitch::buy_sell_indicator::sell ? 0 : 1;

        ev.u.add_order.id    = msg->order_reference_number();
        ev.u.add_order.qty   = msg->shares();
        ev.u.add_order.price = normalize_price( msg->price() );

        books.at( ev.book_id - 1 )
            .book.add_order( ev.u.add_order.make_order(), ev.trade_side() );
    }

    void process_message( const ntvitch::add_order_t * msg )
    {
        process_add_order_message( msg );
    }

    void process_message( const ntvitch::add_order_mpid_t * msg )
    {
        process_add_order_message( msg );
    }

    int errors = 2;

    template < book::trade_side Trade_Side, typename Msg >
    void process_exec_order_message_impl( const Msg * msg )
    {
        assert( msg->stock_locate() > 0 );

        const auto & table = books.at( msg->stock_locate() - 1 )
                                 .book.template xxx_side< Trade_Side >();

        all_events.emplace_back( snapshots::update_record_image_t{} );
        auto & ev  = all_events.back();
        ev.book_id = msg->stock_locate();

        if( const auto first_order = table.first_order();
            type_safe::get( first_order.id ) == msg->order_reference_number() )
        {
            // This order is first in queue
            // So we take it as execute order op.
            ev.op_code = static_cast< std::uint8_t >(
                snapshots::book_operation::exec_order );

            ev.u.exec_order.id = msg->order_reference_number();
            ev.u.exec_order.q  = msg->executed_shares();

            books.at( ev.book_id - 1 )
                .book.execute_order( ev.order_id(),
                                     book::order_qty_t{ ev.u.exec_order.q } );
        }
        else
        {
            // This order is not first in queue,
            // but it is order_executed_with_price so it is allowed
            // Or it is SMP or Min Quantity or whatever
            // we will treat it as reduce or delete.

            const auto order = books.at( ev.book_id - 1 )
                                   .get_order( book::order_id_t{
                                       msg->order_reference_number() } );

            if( const auto qty = book::order_qty_t{ msg->executed_shares() };
                order.qty > qty )
            {
                // Partial fill => treat it as reduce_order operation.
                ev.op_code = static_cast< std::uint8_t >(
                    snapshots::book_operation::reduce_order );
                ev.u.reduce_order.id = msg->order_reference_number();
                ev.u.reduce_order.q  = type_safe::get( qty );

                books.at( ev.book_id - 1 ).book.reduce_order( ev.order_id(), qty );
            }
            else
            {
                // Full fill => treat it as delete_order operation.
                ev.op_code = static_cast< std::uint8_t >(
                    snapshots::book_operation::delete_order );
                ev.u.delete_order.id = msg->order_reference_number();

                books.at( ev.book_id - 1 ).book.delete_order( ev.order_id() );
            }
        }
    }

    template < typename Msg >
    void process_exec_order_message( const Msg * msg )
    {
        const auto trade_side =
            books.at( msg->stock_locate() - 1 )
                .trade_side( book::order_id_t{ msg->order_reference_number() },
                             msg );

        if( trade_side == book::trade_side::sell )
        {
            process_exec_order_message_impl< book::trade_side::sell >( msg );
        }
        else
        {
            process_exec_order_message_impl< book::trade_side::buy >( msg );
        }
    }

    void process_message( const ntvitch::order_executed_t * msg )
    {
        process_exec_order_message( msg );
    }

    void process_message( const ntvitch::order_executed_with_price_t * msg )
    {
        process_exec_order_message( msg );
    }

    void process_message( const ntvitch::order_cancel_t * msg )
    {
        all_events.emplace_back( snapshots::update_record_image_t{} );
        auto & ev = all_events.back();

        ev.book_id = msg->stock_locate();
        ev.op_code =
            static_cast< std::uint8_t >( snapshots::book_operation::reduce_order );
        ev.u.reduce_order.id = msg->order_reference_number();
        ev.u.reduce_order.q  = msg->canceled_shares();

        books.at( ev.book_id - 1 )
            .book.reduce_order( ev.order_id(),
                                book::order_qty_t{ ev.u.exec_order.q } );
    }

    void process_message( const ntvitch::order_delete_t * msg )
    {
        all_events.emplace_back( snapshots::update_record_image_t{} );
        auto & ev = all_events.back();

        ev.book_id = msg->stock_locate();
        ev.op_code =
            static_cast< std::uint8_t >( snapshots::book_operation::delete_order );
        ev.u.delete_order.id = msg->order_reference_number();

        books.at( ev.book_id - 1 ).book.delete_order( ev.order_id() );
    }

    void process_message( const ntvitch::order_replace_t * msg )
    {
        const auto trade_side =
            books.at( msg->stock_locate() - 1 )
                .trade_side(
                    book::order_id_t{ msg->original_order_reference_number() },
                    msg );

        {
            all_events.emplace_back( snapshots::update_record_image_t{} );
            auto & ev = all_events.back();

            ev.book_id = msg->stock_locate();
            ev.op_code = static_cast< std::uint8_t >(
                snapshots::book_operation::delete_order );
            ev.u.delete_order.id = msg->original_order_reference_number();

            books.at( ev.book_id - 1 ).book.delete_order( ev.order_id() );
        }

        all_events.emplace_back( snapshots::update_record_image_t{} );
        auto & ev = all_events.back();

        ev.book_id = msg->stock_locate();
        ev.op_code =
            static_cast< std::uint8_t >( snapshots::book_operation::add_order );
        ev.ts = trade_side == book::trade_side::sell ? 0 : 1;

        ev.u.add_order.id    = msg->new_order_reference_number();
        ev.u.add_order.qty   = msg->shares();
        ev.u.add_order.price = normalize_price( msg->price() );

        books.at( ev.book_id - 1 )
            .book.add_order( ev.u.add_order.make_order(), ev.trade_side() );
    }

    void process_message( const ntvitch::trade_non_cross_t * /*msg*/ ) {}

    void process_message( const ntvitch::cross_trade_t * /*msg*/ ) {}

    void process_message( const ntvitch::broken_trade_t * /*msg*/ ) {}

    void process_message( const ntvitch::noii_t * /*msg*/ ) {}

    /// @}

    template < ntvitch::message_type Message_Type >
    void handle_itch_message( std::span< const char > msg_buf )
    {
        using msg_t = ntvitch::itch_message_t< Message_Type >;

        if( msg_t::image_size != msg_buf.size() )
        {
            throw std::runtime_error{ "invalid message" };
        }

        const msg_t * msg = reinterpret_cast< const msg_t * >( msg_buf.data() );

        process_message( msg );
    }

    void handle_message( std::span< const char > msg_buf )
    {
        if( msg_buf.empty() )
        {
            throw std::runtime_error{ "message block of size zero" };
        }

        const auto msg_type = static_cast< ntvitch::message_type >( msg_buf[ 0 ] );

        switch( msg_type )
        {
            case ntvitch::message_type::system_event:
                handle_itch_message< ntvitch::message_type::system_event >(
                    msg_buf );
                break;
            case ntvitch::message_type::stock_directory:
                handle_itch_message< ntvitch::message_type::stock_directory >(
                    msg_buf );
                break;
            case ntvitch::message_type::stock_trading_action:
                handle_itch_message< ntvitch::message_type::stock_trading_action >(
                    msg_buf );
                break;
            case ntvitch::message_type::add_order:
                handle_itch_message< ntvitch::message_type::add_order >( msg_buf );
                break;
            case ntvitch::message_type::add_order_mpid:
                handle_itch_message< ntvitch::message_type::add_order_mpid >(
                    msg_buf );
                break;
            case ntvitch::message_type::order_executed:
                handle_itch_message< ntvitch::message_type::order_executed >(
                    msg_buf );
                break;
            case ntvitch::message_type::order_executed_with_price:
                handle_itch_message<
                    ntvitch::message_type::order_executed_with_price >( msg_buf );
                break;
            case ntvitch::message_type::order_cancel:
                handle_itch_message< ntvitch::message_type::order_cancel >(
                    msg_buf );
                break;
            case ntvitch::message_type::order_delete:
                handle_itch_message< ntvitch::message_type::order_delete >(
                    msg_buf );
                break;
            case ntvitch::message_type::order_replace:
                handle_itch_message< ntvitch::message_type::order_replace >(
                    msg_buf );
                break;
            case ntvitch::message_type::trade_non_cross:
                handle_itch_message< ntvitch::message_type::trade_non_cross >(
                    msg_buf );
                break;
            case ntvitch::message_type::cross_trade:
                handle_itch_message< ntvitch::message_type::cross_trade >(
                    msg_buf );
                break;
            case ntvitch::message_type::broken_trade:
                handle_itch_message< ntvitch::message_type::broken_trade >(
                    msg_buf );
                break;
            case ntvitch::message_type::noii:
                handle_itch_message< ntvitch::message_type::noii >( msg_buf );
                break;
                // default:
                //     fmt::print( "WARNING: unhandled message {}\n", msg_type );
        }
    }
};

//
// io_counter64_t
//

/**
 * @brief Custom counter implementation to overcomes 2gb files size limit
 *
 * Implementation adopted from here:
 * https://github.com/boostorg/iostreams/blob/dacdb3db57e9f372ad575f02a45b5efb439d8889/include/boost/iostreams/filter/counter.hpp
 */
class io_counter64_t
{
public:
    using char_type = char;

    struct category
        : boost::iostreams::dual_use
        , boost::iostreams::filter_tag
        , boost::iostreams::multichar_tag
        , boost::iostreams::optimally_buffered_tag
    {
    };

    explicit io_counter64_t( std::size_t first_byte = 0 )
        : m_bytes( first_byte )
    {
    }

    std::size_t bytes() const { return m_bytes; }
    std::streamsize optimal_buffer_size() const { return 0; }

    template < typename Source >
    std::streamsize read( Source & src, char_type * s, std::streamsize n )
    {
        std::streamsize result = boost::iostreams::read( src, s, n );
        if( result == -1 ) return -1;
        m_bytes += result;
        return result;
    }

    template < typename Sink >
    std::streamsize write( Sink & snk, const char_type * s, std::streamsize n )
    {
        std::streamsize result = boost::iostreams::write( snk, s, n );
        m_bytes += result;
        return result;
    }

private:
    std::size_t m_bytes;
};

//
// store_data()
//

void store_data( const std::string & filename,
                 std::span< const snapshots::update_record_image_t > data )
{
    std::fstream s{ filename, s.binary | s.trunc | s.out };
    assert( s.is_open() );

    s.write( (const char *)data.data(),
             data.size() * sizeof( snapshots::update_record_image_t ) );
    s.close();
}

//
// convert_events()
//

void convert_events( const config_t & cfg )
{
    // 1. Memory-map the COMPRESSED file.
    // The OS handles paging the compressed bytes into RAM as needed.
    boost::iostreams::mapped_file_source mapped_file;
    mapped_file.open( cfg.input_file_path );

    if( !mapped_file.is_open() )
    {
        throw std::runtime_error{ fmt::format( "Failed to open or map file: {}\n",
                                               cfg.input_file_path ) };
    }

    boost::iostreams::array_source src( mapped_file.data(), mapped_file.size() );

    io_counter64_t progress_cnt;

    fmt::print( "Open file {}: OK\n", cfg.input_file_path );

    // 2. Create the filtering stream and set up the pipeline
    boost::iostreams::filtering_istream input;

    if( cfg.input_file_path.ends_with( ".gz" ) )
    {
        fmt::print( "Add gzip decompressor...\n" );
        input.push( boost::iostreams::gzip_decompressor() );
    }

    input.push( boost::ref( progress_cnt ) );
    input.push( src );

    books_context_t books_context{};
    std::vector< char > buffer( 1U << 16 );
    std::uint16_t length_buf;

    fmt::print( "Start parsing data...\n" );

    const std::size_t input_percent =
        std::max< std::size_t >( mapped_file.size() / 100, 1 );

    std::size_t bytes_interval_start = 0;

    while( input.read( reinterpret_cast< char * >( &length_buf ),
                       sizeof( length_buf ) ) )
    {
        const auto msg_len = JACOBI_NTVITCH_SWAP16( length_buf );

        if( !input.read( buffer.data(), msg_len ) )
        {
            fmt::print( std::cerr,
                        "ERROR: incomplete message buffer: {} of {} expected\n",
                        input.gcount(),
                        msg_len );
            return;
        }

        books_context.handle_message(
            std::span< const char >{ buffer.data(), msg_len } );

        const auto bytes_interval_length =
            progress_cnt.bytes() - bytes_interval_start;

        if( bytes_interval_length > input_percent )
        {
            bytes_interval_start =
                progress_cnt.bytes() - progress_cnt.bytes() % input_percent;

            fmt::print( "PROGRESS: {:02}% ({}/{}) \n",
                        progress_cnt.bytes() / input_percent,
                        progress_cnt.bytes(),
                        mapped_file.size() );
        }
    }

    if( 0 != input.gcount() )
    {
        fmt::print( std::cerr, "ERROR: incomplete msg len \n" );
    }

    fmt::print( "\nParsing {} complete\n", cfg.input_file_path );
    fmt::print( "\nStore single book data\n", cfg.input_file_path );

    {
        const auto fname = fmt::format(
            "{}/{}all.jacobi_data", cfg.output_dir, cfg.out_file_suffix );

        store_data( fname, books_context.all_events );
        fmt::print( "\nStore all data: {} (events_cnt={})\n",
                    fname,
                    books_context.all_events.size() );
    }

    {
        const auto filtered_events =
            books_context.filter_events( cfg.filter_min_events );

        const auto fname = fmt::format(
            "{}/{}all_filtered.jacobi_data", cfg.output_dir, cfg.out_file_suffix );

        store_data( fname, filtered_events );
        fmt::print( "\nStore all filtered data: {} (events_cnt={})\n",
                    fname,
                    books_context.all_events.size() );
    }

    {
        const auto singles =
            books_context.make_single_book_events( cfg.filter_min_events );

        int fcount = 1;
        for( const auto & s : singles )
        {
            const auto fname = fmt::format( "{}/{}{}.jacobi_data",
                                            cfg.output_dir,
                                            cfg.out_file_suffix,
                                            s.first );

            store_data( fname, s.second );
            fmt::print( "\nStore single data: {} (events_cnt={}) {:3}/{}\n",
                        fname,
                        s.second.size(),
                        fcount++,
                        singles.size() );
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
        CLI::App app{ "Convert Nasdaq TotalView-ITCH 5.0 data to jacobi_data" };
        jacobi::benchmark::config_t cfg;

        app.add_option( "-i,--input", cfg.input_file_path, "Path to input file" )
            ->required()
            ->check( CLI::ExistingFile );

        app.add_option( "-o,--output-dir", cfg.output_dir, "Path to output dir" )
            ->required()
            ->check( CLI::ExistingDirectory );

        app.add_option( "-s,--output-suffix",
                        cfg.out_file_suffix,
                        "Add suffix to output file name" );

        app.add_option( "-m,--filter-min-events",
                        cfg.filter_min_events,
                        "Store results only for book with at least N events" );

        CLI11_PARSE( app, argc, argv );

        jacobi::benchmark::convert_events( cfg );
    }
    catch( const std::exception & ex )
    {
        fmt::print( std::cerr, "Error: {}\n", ex.what() );
        return 1;
    }

    return 0;
}
