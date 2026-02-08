#include <jacobi/book/utils/chunk_list.hpp>

#include <algorithm>
#include <random>

#include <range/v3/view/subrange.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/reverse.hpp>

#include <gtest/gtest.h>

namespace jacobi::book::utils
{

namespace /* anonymous */
{

static_assert( std::is_trivially_copyable_v< chunk_list_t< int >::iterator > );
static_assert(
    std::is_trivially_copyable_v< chunk_list_t< int >::const_iterator > );

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkListChunk, CanAppendToChunk )
{
    chunk_node_t< int > chunk;

    EXPECT_EQ( true, chunk.can_append_to_chunk() );
    EXPECT_EQ( 0, chunk.insert_position() );
    EXPECT_EQ( 0, chunk.stored_count() );

    EXPECT_EQ( 0, chunk.append( 0 ) );
    EXPECT_EQ( 1, chunk.insert_position() );
    EXPECT_EQ( 1, chunk.stored_count() );

    for( int i = 1; i < 64; ++i )
    {
        ASSERT_EQ( true, chunk.can_append_to_chunk() ) << "i=" << i;
        EXPECT_EQ( i, chunk.append( i ) ) << "i=" << i;
        EXPECT_EQ( i + 1, chunk.insert_position() ) << "i=" << i;
        EXPECT_EQ( i + 1, chunk.stored_count() );
    }

    ASSERT_EQ( false, chunk.can_append_to_chunk() );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkListChunk, InsertRemove )
{
    chunk_node_t< int > chunk;

    for( int i = 0; i < 64; ++i )
    {
        [[maybe_unused]] auto x = chunk.append( i );
    }

    for( int i = 0; i < 63; ++i )
    {
        chunk.erase( i );
    }

    EXPECT_EQ( 1, chunk.stored_count() );
    chunk.erase( 63 );
    EXPECT_TRUE( chunk.empty() );

    EXPECT_EQ( 0, chunk.stored_count() );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkListChunk, FindAllocated1 )
{
    chunk_node_t< int > chunk;

    for( int i = 0; i < 4; ++i )
    {
        ASSERT_EQ( chunk.insert_position(), i );
        [[maybe_unused]] auto x = chunk.append( i );
    }

    ASSERT_EQ( chunk.data[ 0 ], 0 );
    ASSERT_EQ( chunk.data[ 1 ], 1 );
    ASSERT_EQ( chunk.data[ 2 ], 2 );
    ASSERT_EQ( chunk.data[ 3 ], 3 );

    ASSERT_EQ( chunk.stored_count(), 4 );

    ASSERT_EQ( chunk.find_first_allocated(), 0 );
    ASSERT_EQ( chunk.find_last_allocated(), 3 );

    ASSERT_EQ( chunk.find_next_allocated( 0 ), 1 );
    ASSERT_EQ( chunk.find_next_allocated( 1 ), 2 );
    ASSERT_EQ( chunk.find_next_allocated( 2 ), 3 );
    ASSERT_EQ( chunk.find_next_allocated( 3 ), 64 );

    ASSERT_EQ( chunk.find_prev_allocated( 1 ), 0 );
    ASSERT_EQ( chunk.find_prev_allocated( 2 ), 1 );
    ASSERT_EQ( chunk.find_prev_allocated( 3 ), 2 );
    ASSERT_EQ( chunk.find_prev_allocated( 64 ), 3 );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkListChunk, FindAllocated2 )
{
    chunk_node_t< int > chunk;

    for( int i = 0; i < 55; ++i )
    {
        [[maybe_unused]] auto x = chunk.append( i );
    }

    for( int i = 2; i < 53; ++i )
    {
        chunk.erase( i );
    }

    ASSERT_EQ( chunk.data[ chunk.find_first_allocated() ], 0 );
    ASSERT_EQ( chunk.data[ chunk.find_last_allocated() ], 54 );

    ASSERT_EQ( chunk.find_next_allocated( 0 ), 1 );
    ASSERT_EQ( chunk.find_next_allocated( 1 ), 53 );
    ASSERT_EQ( chunk.find_next_allocated( 53 ), 54 );
    ASSERT_EQ( chunk.find_next_allocated( 54 ), 64 );

    ASSERT_EQ( chunk.find_prev_allocated( 54 ), 53 );
    ASSERT_EQ( chunk.find_prev_allocated( 53 ), 1 );
    ASSERT_EQ( chunk.find_prev_allocated( 1 ), 0 );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkList, Basic )
{
    chunk_list_t< int > chunk_list;
    for( int i = 0; i < 342; ++i )
    {
        [[maybe_unused]] auto x = chunk_list.push_back( i );
    }

    ASSERT_EQ( chunk_list.size(), 342 );

    auto it = chunk_list.begin();
    for( int i = 0; i < 342; ++i )
    {
        ASSERT_EQ( *it, i );
        ++it;
    }

    EXPECT_EQ( it, chunk_list.end() );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkList, EraseSingle )
{
    chunk_list_t< std::string > chunk_list;
    [[maybe_unused]] auto x = chunk_list.push_back( "1234" );

    ASSERT_EQ( chunk_list.size(), 1 );

    auto it1     = chunk_list.begin();
    auto it_next = it1;
    ++it_next;
    EXPECT_EQ( it_next, chunk_list.end() );

    auto it = chunk_list.erase( it1 );

    EXPECT_EQ( 0, chunk_list.size() );
    EXPECT_EQ( it, chunk_list.end() );
    EXPECT_EQ( it_next, chunk_list.end() );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkList, ReverseIteration )
{
    chunk_list_t< int > chunk_list;
    for( int i = 0; i < 567; ++i )
    {
        [[maybe_unused]] auto x = chunk_list.push_back( i );
    }

    auto it = chunk_list.end();

    for( int i = 566; i >= 0; --i )
    {
        --it;
        ASSERT_EQ( *it, i );
    }

    EXPECT_EQ( it, chunk_list.begin() );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkList, EraseAll )
{
    chunk_list_t< int > chunk_list;
    for( int i = 0; i < 567; ++i )
    {
        [[maybe_unused]] auto x = chunk_list.push_back( i );
    }

    auto it = chunk_list.begin();
    while( it != chunk_list.end() )
    {
        int v = *it;
        it    = chunk_list.erase( it );
        if( it != chunk_list.end() )
        {
            ASSERT_EQ( *it, v + 1 );
        }
    }

    EXPECT_EQ( 0, chunk_list.size() );
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkList, EraseEvery10th )
{
    chunk_list_t< int > chunk_list;
    for( int i = 0; i < 10000; ++i )
    {
        [[maybe_unused]] auto x = chunk_list.push_back( i );
    }

    auto it = chunk_list.begin();
    for( int i = 0; i < 10000; ++i )
    {
        if( i % 10 == 0 )
        {
            it = chunk_list.erase( it );
        }
        else
        {
            ++it;
        }
    }

    EXPECT_EQ( 10000 - 1000, chunk_list.size() );

    ASSERT_EQ( *( chunk_list.begin() ), 1 );

    it = chunk_list.begin();
    for( int i = 0; i < 10000; ++i )
    {
        if( i % 10 != 0 )
        {
            ASSERT_EQ( *it, i );
            ++it;
        }
    }
}

// NOLINTNEXTLINE
TEST( JacobiUtilsChunkList, EraseAndIterate )
{
    chunk_list_t< int > chunk_list;

    using it_t = chunk_list_t< int >::iterator;
    std::vector< it_t > iterators;

    int x = 0;
    for( ; x < 10000; ++x )
    {
        iterators.push_back( chunk_list.push_back( x ) );
    }

    std::random_device rd{};
    std::mt19937 g( rd() );

    int n = 100;
    while( n-- > 0 )
    {
        std::shuffle( iterators.begin(), iterators.end(), g );
        for( int i = 0; i < 2000; ++i )
        {
            chunk_list.erase( iterators.back() );
            iterators.pop_back();
        }

        for( int i = 0; i < 2000; ++i )
        {
            iterators.push_back( chunk_list.push_back( x++ ) );
        }

        ASSERT_EQ( chunk_list.size(), 10000 );
        std::size_t traverse_size = 0;

        ranges::for_each( chunk_list, [ & ]( int ) { ++traverse_size; } );

        ASSERT_EQ( traverse_size, 10000 );
    }
}

}  // anonymous namespace

}  // namespace jacobi::book::utils
