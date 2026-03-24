#include <jacobi/book/soa_price_level2.hpp>

#include <random>
#include <algorithm>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/transform.hpp>

#include <gtest/gtest.h>

namespace jacobi::book
{

namespace /* anonymous */
{

using details::unified_soa_position_t;
using details::soa_price_level_data_access_t;
using details::soa_price_level_data_access_u8_t;
using details::soa_price_level_data_access_u16_t;
using details::soa_price_level_data_access_u32_t;
using details::raw_buffer_t;
using details::data_buf_accessor_type;

// NOLINTNEXTLINE
TEST( JacobiBookDetailsSoaPriceLevelDataAccessU8, BufferRequiredSize )
{
    using soa_price_level_data_access_t =
        soa_price_level_data_access_t< std::uint8_t >;

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 2 ),
               8 + 2 * 8 + 2 * 4 + 4 * 2 );

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 8 ), 124 );
    EXPECT_EQ( soa_price_level_data_access_t::required_size( 16 ), 236 );
    EXPECT_EQ( soa_price_level_data_access_t::required_size( 17 ), 250 );

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 32 ), 460 );
    EXPECT_EQ( soa_price_level_data_access_t::required_size( 35 ), 502 );
    EXPECT_EQ( soa_price_level_data_access_t::required_size( 72 ), 1020 );

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 254 ), 3568 );
}

// NOLINTNEXTLINE
TEST( JacobiBookDetailsSoaPriceLevelDataAccessU16, BufferRequiredSize )
{
    using soa_price_level_data_access_t =
        soa_price_level_data_access_t< std::uint16_t >;

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 2 ),
               8 + 2 * 8 + 2 * 4 + 4 * 4 );

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 255 ), 4096 );

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 1023 ), 16384 );
}

// NOLINTNEXTLINE
TEST( JacobiBookDetailsSoaPriceLevelDataAccessU32, BufferRequiredSize )
{
    using soa_price_level_data_access_t =
        soa_price_level_data_access_t< std::uint32_t >;

    EXPECT_EQ( soa_price_level_data_access_t::required_size( 65535 ), 1310724 );
    EXPECT_EQ( soa_price_level_data_access_t::required_size( 65536 ), 1310744 );
    EXPECT_EQ( soa_price_level_data_access_t::required_size( 100000 ), 2000024 );
}

// NOLINTNEXTLINE
TEST( JacobiBookDetailsSoaPriceLevelDataAccess, ReadWriteU32 )
{
    union
    {
        std::byte buf[ 4 ];
        std::uint32_t i;
    } data{};

    EXPECT_EQ( 0, details::soa_buf_read_u32( data.buf ) );

    data.i = 123;
    EXPECT_EQ( 123, details::soa_buf_read_u32( data.buf ) );

    data.i = 0x12345678U;
    EXPECT_EQ( 0x12345678U, details::soa_buf_read_u32( data.buf ) );

    details::soa_buf_write_u32( 1, data.buf );
    EXPECT_EQ( 1, data.i );

    details::soa_buf_write_u32( 220, data.buf );
    EXPECT_EQ( 220, data.i );

    details::soa_buf_write_u32( 0xFFFFFFFFU, data.buf );
    EXPECT_EQ( 0xFFFFFFFFU, data.i );
}

//
// aligned_buffer_t
//

class aligned_buffer_t
{
public:
    explicit aligned_buffer_t( std::size_t size, std::size_t alignment = 8 )
        : m_size{ size }
        , m_alignment{ alignment }
        , m_ptr{ static_cast< std::byte * >(
              ::operator new( size, std::align_val_t{ alignment } ) ) }
    {
    }

    ~aligned_buffer_t()
    {
        ::operator delete( m_ptr, std::align_val_t{ m_alignment } );
    }

    aligned_buffer_t( const aligned_buffer_t & )             = delete;
    aligned_buffer_t( aligned_buffer_t && )                  = delete;
    aligned_buffer_t & operator=( const aligned_buffer_t & ) = delete;
    aligned_buffer_t & operator=( aligned_buffer_t && )      = delete;

    [[nodiscard]] raw_buffer_t span() noexcept { return { m_ptr, m_size }; }

    [[nodiscard]] std::span< const std::byte > span() const noexcept
    {
        return { m_ptr, m_size };
    }

private:
    std::size_t m_size{};
    std::size_t m_alignment{};
    std::byte * m_ptr{};
};

//
// JacobiBookDetailsSoaPriceLevelDataAccessTests
//

template < typename T >
class JacobiBookDetailsSoaPriceLevelDataAccessTests : public ::testing::Test
{
protected:
    void SetUp() override {}

    using index_t       = T;
    using data_access_t = soa_price_level_data_access_t< index_t >;
};

using JacobiBookDetailsSoaPriceLevelDataAccessTestTypes =
    ::testing::Types< std::uint8_t, std::uint16_t, std::uint32_t >;

TYPED_TEST_SUITE( JacobiBookDetailsSoaPriceLevelDataAccessTests,
                  JacobiBookDetailsSoaPriceLevelDataAccessTestTypes );

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, InitializeLayout )
{
    using this_type_t = std::remove_reference_t< decltype( *this ) >;
    using soa_price_level_data_access_t = this_type_t::data_access_t;

    constexpr std::uint32_t cap = 4;
    aligned_buffer_t buf{ soa_price_level_data_access_t::required_size( cap ) };

    soa_price_level_data_access_t::initialize_layout( buf.span(), cap );
    soa_price_level_data_access_t level{ buf.span() };

    EXPECT_EQ( level.size(), 0 );
    EXPECT_EQ( level.capacity(), cap );
    EXPECT_TRUE( level.empty() );
    EXPECT_FALSE( level.full() );

    const auto * links = level.links();

    // Active-list head anchor points to itself.
    EXPECT_EQ( links[ 0 ].prev, 0 );
    EXPECT_EQ( links[ 0 ].next, 0 );

    if constexpr( !std::is_same_v<
                      std::uint8_t,
                      typename soa_price_level_data_access_t::short_index_t > )
    {
        // Virgin cursor node.

        // Free-list head anchor should forward link to itself.
        // and prev must point to first node to be used if free list is empty
        // which is 2 (first usable node - not an anchor).
        EXPECT_EQ( links[ 1 ].next, 1 );
        EXPECT_EQ( links[ 1 ].prev, 2 );
    }

    std::vector< std::uint32_t > free_nodes;
    for( auto i = links[ 1 ].next; i != 1; i = links[ i ].next )
    {
        free_nodes.push_back( i );
    }

    if constexpr( !std::is_same_v<
                      std::uint8_t,
                      typename soa_price_level_data_access_t::short_index_t > )
    {
        const std::vector< std::uint32_t > expected{};
        EXPECT_EQ( free_nodes, expected );
    }
    else
    {
        const std::vector< std::uint32_t > expected{ 2, 3, 4, 5 };
        EXPECT_EQ( free_nodes, expected );
    }
}

using zipped_orders_data_t =
    std::vector< std::pair< std::uint64_t, std::uint32_t > >;

template < typename Range >
[[nodiscard]] zipped_orders_data_t collect_orders( Range && range )
{
    zipped_orders_data_t res;
    res.reserve( 16 );
    for( const auto & [ id, qty ] : range )
    {
        res.emplace_back( type_safe::get( id ), type_safe::get( qty ) );
    }
    return res;
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests,
            PushBackStoresValuesAndPreservesForwardAndReverseOrder )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint32_t cap = 4;
    aligned_buffer_t buf{ soa_price_level_data_access_t::required_size( cap ) };

    soa_price_level_data_access_t::initialize_layout( buf.span(), cap );
    soa_price_level_data_access_t level{ buf.span() };

    const auto p1 = level.push_back( order_id_t{ 101 }, order_qty_t{ 11 } );
    const auto p2 = level.push_back( order_id_t{ 202 }, order_qty_t{ 22 } );
    const auto p3 = level.push_back( order_id_t{ 303 }, order_qty_t{ 33 } );

    EXPECT_EQ( p1.node_link_index(), 2 );
    EXPECT_EQ( p2.node_link_index(), 3 );
    EXPECT_EQ( p3.node_link_index(), 4 );

    EXPECT_EQ( level.size(), 3 );

    EXPECT_FALSE( level.empty() );
    EXPECT_FALSE( level.full() );

    const auto forward = collect_orders( level );
    const auto reverse =
        collect_orders( ranges::make_subrange( level.rbegin(), level.rend() ) );

    const zipped_orders_data_t expected_forward{
        { 101, 11 },
        { 202, 22 },
        { 303, 33 },
    };

    const zipped_orders_data_t expected_reverse{
        { 303, 33 },
        { 202, 22 },
        { 101, 11 },
    };

    EXPECT_EQ( forward, expected_forward );
    EXPECT_EQ( reverse, expected_reverse );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, PopAtRemovesMiddleNode )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint16_t cap = 6;
    aligned_buffer_t buf{ soa_price_level_data_access_t::required_size( cap ) };

    soa_price_level_data_access_t::initialize_layout( buf.span(), cap );
    soa_price_level_data_access_t level{ buf.span() };

    const auto p1 = level.push_back( order_id_t{ 10 }, order_qty_t{ 1 } );
    const auto p2 = level.push_back( order_id_t{ 20 }, order_qty_t{ 2 } );
    const auto p3 = level.push_back( order_id_t{ 30 }, order_qty_t{ 3 } );

    ASSERT_EQ( level.size(), 3 );

    level.pop_at( p2 );

    EXPECT_EQ( level.size(), 2 );
    EXPECT_FALSE( level.empty() );
    EXPECT_FALSE( level.full() );

    const auto forward = collect_orders( level );
    const auto reverse =
        collect_orders( ranges::make_subrange( level.rbegin(), level.rend() ) );

    const zipped_orders_data_t expected_forward{
        { 10, 1 },
        { 30, 3 },
    };

    const zipped_orders_data_t expected_reverse{
        { 30, 3 },
        { 10, 1 },
    };

    EXPECT_EQ( forward, expected_forward );
    EXPECT_EQ( reverse, expected_reverse );

    const auto * links = level.links();

    // Active list should now be HEAD <-> p1 <-> p3 <-> HEAD.
    EXPECT_EQ( links[ 0 ].next, p1.node_link_index() );
    EXPECT_EQ( links[ 0 ].prev, p3.node_link_index() );

    EXPECT_EQ( links[ p1.node_link_index() ].prev, 0 );
    EXPECT_EQ( links[ p1.node_link_index() ].next, p3.node_link_index() );

    EXPECT_EQ( links[ p3.node_link_index() ].prev, p1.node_link_index() );
    EXPECT_EQ( links[ p3.node_link_index() ].next, 0 );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, FullAndEmptyState )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint16_t cap = 4;
    aligned_buffer_t buf{ soa_price_level_data_access_t::required_size( cap ) };

    soa_price_level_data_access_t::initialize_layout( buf.span(), cap );
    soa_price_level_data_access_t level{ buf.span() };

    EXPECT_TRUE( level.empty() );
    EXPECT_FALSE( level.full() );
    EXPECT_EQ( 0, level.size() );

    const auto p1 = level.push_back( order_id_t{ 1 }, order_qty_t{ 10 } );
    EXPECT_FALSE( level.empty() );
    EXPECT_FALSE( level.full() );
    EXPECT_EQ( 1, level.size() );

    const auto p2 = level.push_back( order_id_t{ 2 }, order_qty_t{ 20 } );
    const auto p3 = level.push_back( order_id_t{ 3 }, order_qty_t{ 30 } );
    EXPECT_FALSE( level.empty() );
    EXPECT_FALSE( level.full() );
    EXPECT_EQ( 3, level.size() );

    const auto p4 = level.push_back( order_id_t{ 4 }, order_qty_t{ 40 } );
    EXPECT_FALSE( level.empty() );
    EXPECT_TRUE( level.full() );
    EXPECT_EQ( 4, level.size() );

    level.pop_at( p1 );
    EXPECT_FALSE( level.empty() );
    EXPECT_FALSE( level.full() );
    EXPECT_EQ( 3, level.size() );

    level.pop_at( p3 );
    level.pop_at( p2 );
    EXPECT_FALSE( level.empty() );
    EXPECT_FALSE( level.full() );
    EXPECT_EQ( 1, level.size() );

    level.pop_at( p4 );
    EXPECT_TRUE( level.empty() );
    EXPECT_FALSE( level.full() );
    EXPECT_EQ( 0, level.size() );

    const auto forward = collect_orders( level );
    EXPECT_TRUE( forward.empty() );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, ReuseNodes )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint16_t cap = 4;
    aligned_buffer_t buf{ soa_price_level_data_access_t::required_size( cap ) };

    soa_price_level_data_access_t::initialize_layout( buf.span(), cap );
    soa_price_level_data_access_t level{ buf.span() };

    const auto p1 = level.push_back( order_id_t{ 1001 }, order_qty_t{ 10 } );
    const auto p2 = level.push_back( order_id_t{ 1002 }, order_qty_t{ 20 } );
    const auto p3 = level.push_back( order_id_t{ 1003 }, order_qty_t{ 30 } );
    const auto p4 = level.push_back( order_id_t{ 1004 }, order_qty_t{ 40 } );

    EXPECT_GE( p1.node_link_index(), 2u );
    EXPECT_LT( p1.node_link_index(), static_cast< std::size_t >( 2 + cap ) );
    EXPECT_GE( p2.node_link_index(), 2u );
    EXPECT_LT( p2.node_link_index(), static_cast< std::size_t >( 2 + cap ) );
    EXPECT_GE( p3.node_link_index(), 2u );
    EXPECT_LT( p3.node_link_index(), static_cast< std::size_t >( 2 + cap ) );
    EXPECT_GE( p4.node_link_index(), 2u );
    EXPECT_LT( p4.node_link_index(), static_cast< std::size_t >( 2 + cap ) );

    ASSERT_EQ( level.size(), 4 );

    level.pop_at( p2 );
    level.pop_at( p3 );
    ASSERT_EQ( level.size(), 2 );
    ASSERT_FALSE( level.full() );

    const auto p5 = level.push_back( order_id_t{ 2000 }, order_qty_t{ 123 } );
    const auto p6 = level.push_back( order_id_t{ 3000 }, order_qty_t{ 42 } );

    EXPECT_EQ( level.size(), 4 );
    EXPECT_TRUE( level.full() );

    {
        const auto forward = collect_orders( level );
        const zipped_orders_data_t expected{
            { 1001, 10 },
            { 1004, 40 },
            { 2000, 123 },
            { 3000, 42 },
        };
        EXPECT_EQ( forward, expected );
    }

    EXPECT_GE( p5.node_link_index(), 2u );
    EXPECT_LT( p5.node_link_index(), static_cast< std::size_t >( 2 + cap ) );
    EXPECT_GE( p6.node_link_index(), 2u );
    EXPECT_LT( p6.node_link_index(), static_cast< std::size_t >( 2 + cap ) );

    level.pop_at( p1 );
    level.pop_at( p4 );
    ASSERT_EQ( level.size(), 2 );
    ASSERT_FALSE( level.full() );

    const auto p7 = level.push_back( order_id_t{ 4000 }, order_qty_t{ 99 } );
    const auto p8 = level.push_back( order_id_t{ 5000 }, order_qty_t{ 111 } );
    EXPECT_GE( p7.node_link_index(), 2u );
    EXPECT_LT( p7.node_link_index(), static_cast< std::size_t >( 2 + cap ) );
    EXPECT_GE( p8.node_link_index(), 2u );
    EXPECT_LT( p8.node_link_index(), static_cast< std::size_t >( 2 + cap ) );

    {
        const auto forward = collect_orders( level );
        const zipped_orders_data_t expected{
            { 2000, 123 },
            { 3000, 42 },
            { 4000, 99 },
            { 5000, 111 },
        };
        EXPECT_EQ( forward, expected );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, InvalidBuffer )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    // ===============================================================
    // Misaligned buffer
    {

        constexpr std::uint16_t cap = 4;
        aligned_buffer_t buf{
            4 + soa_price_level_data_access_t::required_size( cap )
        };

        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 1 ) ),
                      std::runtime_error );
        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 2 ) ),
                      std::runtime_error );
        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 4 ) ),
                      std::runtime_error );
    }

    // ===============================================================
    // Too small buffer
    {
        constexpr std::uint16_t cap = 4;
        aligned_buffer_t buf{ soa_price_level_data_access_t::required_size(
            cap ) };

        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 0, 1 ) ),
                      std::runtime_error );
        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 0, 2 ) ),
                      std::runtime_error );
        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 0, 4 ) ),
                      std::runtime_error );
        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer(
                          buf.span().subspan( 0, 7 ) ),
                      std::runtime_error );
    }

    // ===============================================================
    // Too small buffer for a given capacity
    {
        constexpr std::uint32_t cap = 4;
        aligned_buffer_t buf{ soa_price_level_data_access_t::required_size(
            cap ) };

        details::soa_buf_write_u32( 5, buf.span().data() + 2 );

        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer( buf.span() ),
                      std::runtime_error );
    }

    // ===============================================================
    // Size exceeds capacity
    {
        constexpr std::uint32_t cap = 4;
        aligned_buffer_t buf{ soa_price_level_data_access_t::required_size(
            cap ) };

        details::soa_buf_write_u32( cap + 1, buf.span().data() );

        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer( buf.span() ),
                      std::runtime_error );
    }

    // ===============================================================
    // Odd capacity
    {
        constexpr std::uint32_t cap = 5;
        aligned_buffer_t buf{ soa_price_level_data_access_t::required_size(
            cap ) };

        details::soa_buf_write_u32( cap, buf.span().data() + 2 );

        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer( buf.span() ),
                      std::runtime_error );
    }

    // ===============================================================
    // Too small capacity
    {
        constexpr std::uint32_t cap = 2;
        aligned_buffer_t buf{ soa_price_level_data_access_t::required_size(
            cap ) };

        details::soa_buf_write_u32( cap, buf.span().data() + 2 );

        EXPECT_THROW( soa_price_level_data_access_t::validate_buffer( buf.span() ),
                      std::runtime_error );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, RangeRoutines )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint32_t cap = 17;
    aligned_buffer_t buf{ soa_price_level_data_access_t::required_size( cap ) };

    soa_price_level_data_access_t::initialize_layout( buf.span(), cap );
    soa_price_level_data_access_t level{ buf.span() };

    // Forward
    EXPECT_EQ( level.begin(), level.end() );
    EXPECT_EQ( level.cbegin(), level.cend() );

    // Reverse
    EXPECT_EQ( level.rbegin(), level.rend() );
    EXPECT_EQ( level.crbegin(), level.crend() );

    (void)level.push_back( order_id_t{ 10 }, order_qty_t{ 100 } );
    (void)level.push_back( order_id_t{ 20 }, order_qty_t{ 200 } );
    (void)level.push_back( order_id_t{ 30 }, order_qty_t{ 300 } );

    {
        const auto orders = collect_orders( level );

        ASSERT_EQ( orders.size(), 3 );
        EXPECT_EQ( orders[ 0 ].first, 10 );
        EXPECT_EQ( orders[ 1 ].first, 20 );
        EXPECT_EQ( orders[ 2 ].first, 30 );

        EXPECT_EQ( orders[ 0 ].second, 100 );
        EXPECT_EQ( orders[ 1 ].second, 200 );
        EXPECT_EQ( orders[ 2 ].second, 300 );
    }

    {
        const auto orders = collect_orders(
            ranges::make_subrange( level.rbegin(), level.rend() ) );

        ASSERT_EQ( orders.size(), 3 );
        EXPECT_EQ( orders[ 0 ].first, 30 );
        EXPECT_EQ( orders[ 1 ].first, 20 );
        EXPECT_EQ( orders[ 2 ].first, 10 );

        EXPECT_EQ( orders[ 0 ].second, 300 );
        EXPECT_EQ( orders[ 1 ].second, 200 );
        EXPECT_EQ( orders[ 2 ].second, 100 );
    }

    {
        auto it = level.begin();
        EXPECT_EQ( type_safe::get( ( *it ).first ), 10 );
        ASSERT_NE( it, level.end() );

        ++it;
        EXPECT_EQ( type_safe::get( ( *it ).first ), 20 );
        ASSERT_NE( it, level.end() );

        ++it;
        ASSERT_NE( it, level.end() );
        EXPECT_EQ( type_safe::get( ( *it ).first ), 30 );

        ++it;
        EXPECT_EQ( it, level.end() );

        --it;
        ASSERT_NE( it, level.end() );
        EXPECT_EQ( type_safe::get( ( *it ).first ), 30 );

        --it;
        EXPECT_EQ( type_safe::get( ( *it ).first ), 20 );
        ASSERT_NE( it, level.end() );

        --it;
        EXPECT_EQ( type_safe::get( ( *it ).first ), 10 );
        ASSERT_NE( it, level.end() );
        ASSERT_EQ( it, level.begin() );
    }

    {
        auto it = level.rbegin();
        EXPECT_EQ( type_safe::get( ( *it ).first ), 30 );
        ASSERT_NE( it, level.rend() );

        ++it;
        EXPECT_EQ( type_safe::get( ( *it ).first ), 20 );
        ASSERT_NE( it, level.rend() );

        ++it;
        ASSERT_NE( it, level.rend() );
        EXPECT_EQ( type_safe::get( ( *it ).first ), 10 );

        ++it;
        EXPECT_EQ( it, level.rend() );

        --it;
        ASSERT_NE( it, level.rend() );
        EXPECT_EQ( type_safe::get( ( *it ).first ), 10 );

        --it;
        EXPECT_EQ( type_safe::get( ( *it ).first ), 20 );
        ASSERT_NE( it, level.rend() );

        --it;
        EXPECT_EQ( type_safe::get( ( *it ).first ), 30 );
        ASSERT_NE( it, level.rend() );
        ASSERT_EQ( it, level.rbegin() );
    }

    {
        // Reverse to Forward
        auto r_it = level.rbegin();
        EXPECT_EQ( type_safe::get( ( *r_it ).first ), 30 );

        auto base_it = r_it.base();
        EXPECT_EQ( base_it, level.end() );
    }
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests, SameTypeExpansion )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint32_t cap1 = 4;
    aligned_buffer_t buf1{ soa_price_level_data_access_t::required_size( cap1 ) };

    soa_price_level_data_access_t::initialize_layout( buf1.span(), cap1 );
    soa_price_level_data_access_t level_old{ buf1.span() };

    (void)level_old.push_back( order_id_t{ 10 }, order_qty_t{ 100 } );
    (void)level_old.push_back( order_id_t{ 20 }, order_qty_t{ 200 } );
    (void)level_old.push_back( order_id_t{ 30 }, order_qty_t{ 300 } );
    (void)level_old.push_back( order_id_t{ 40 }, order_qty_t{ 400 } );

    constexpr std::uint32_t cap2 = 8;
    aligned_buffer_t buf2{ soa_price_level_data_access_t::required_size( cap2 ) };

    soa_price_level_data_access_t::initialize_layout(
        buf2.span(), cap2, level_old );
    soa_price_level_data_access_t level_new{ buf2.span() };

    ASSERT_EQ( cap2, level_new.capacity() );
    ASSERT_EQ( cap1, level_new.size() );

    const zipped_orders_data_t expected_orders{
        { 10, 100 },
        { 20, 200 },
        { 30, 300 },
        { 40, 400 },
    };

    ASSERT_EQ( collect_orders( level_new ), expected_orders );
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests,
            SameTypeExpansionShuffledList )
{
    using soa_price_level_data_access_t =
        std::remove_reference_t< decltype( *this ) >::data_access_t;

    constexpr std::uint32_t cap1 = 4;
    aligned_buffer_t buf1{ soa_price_level_data_access_t::required_size( cap1 ) };

    soa_price_level_data_access_t::initialize_layout( buf1.span(), cap1 );
    soa_price_level_data_access_t level_old{ buf1.span() };

    const auto p1 = level_old.push_back( order_id_t{ 10 }, order_qty_t{ 100 } );
    const auto p2 = level_old.push_back( order_id_t{ 20 }, order_qty_t{ 200 } );

    level_old.pop_at( p1 );

    (void)level_old.push_back( order_id_t{ 30 }, order_qty_t{ 300 } );
    const auto p4 = level_old.push_back( order_id_t{ 40 }, order_qty_t{ 400 } );

    level_old.pop_at( p2 );
    level_old.pop_at( p4 );

    const auto p5 = level_old.push_back( order_id_t{ 50 }, order_qty_t{ 500 } );
    (void)level_old.push_back( order_id_t{ 60 }, order_qty_t{ 600 } );
    (void)level_old.push_back( order_id_t{ 70 }, order_qty_t{ 700 } );

    level_old.pop_at( p5 );
    (void)level_old.push_back( order_id_t{ 80 }, order_qty_t{ 800 } );

    constexpr std::uint32_t cap2 = 8;
    aligned_buffer_t buf2{ soa_price_level_data_access_t::required_size( cap2 ) };

    soa_price_level_data_access_t::initialize_layout(
        buf2.span(), cap2, level_old );
    soa_price_level_data_access_t level_new{ buf2.span() };

    ASSERT_EQ( cap2, level_new.capacity() );
    ASSERT_EQ( cap1, level_new.size() );

    {
        const zipped_orders_data_t expected_orders{
            { 30, 300 },
            { 60, 600 },
            { 70, 700 },
            { 80, 800 },
        };

        ASSERT_EQ( collect_orders( level_new ), expected_orders );
    }

    (void)level_new.push_back( order_id_t{ 90 }, order_qty_t{ 900 } );
    (void)level_new.push_back( order_id_t{ 100 }, order_qty_t{ 1000 } );

    ASSERT_EQ( cap2, level_new.capacity() );
    ASSERT_EQ( 6, level_new.size() );

    {
        const zipped_orders_data_t expected_orders{
            { 30, 300 }, { 60, 600 }, { 70, 700 },
            { 80, 800 }, { 90, 900 }, { 100, 1000 },
        };

        ASSERT_EQ( collect_orders( level_new ), expected_orders );
        const auto orders = collect_orders( level_new );
    }
}

//
// run_clone_preserves_state_and_positions()
//

template < typename Dst_Accessor, typename Src_Accessor >
void run_clone_preserves_state_and_positions()
{
    constexpr std::uint32_t src_cap = 6;
    constexpr std::uint32_t dst_cap = 10;

    aligned_buffer_t src_buf{ Src_Accessor::required_size( src_cap ) };
    Src_Accessor::initialize_layout( src_buf.span(), src_cap );
    Src_Accessor src{ src_buf.span() };

    // Build a non-trivial source state:
    (void)src.push_back( order_id_t{ 101 }, order_qty_t{ 11 } );
    const auto pos_2 = src.push_back( order_id_t{ 202 }, order_qty_t{ 22 } );
    (void)src.push_back( order_id_t{ 303 }, order_qty_t{ 33 } );
    (void)src.push_back( order_id_t{ 404 }, order_qty_t{ 44 } );
    (void)src.push_back( order_id_t{ 505 }, order_qty_t{ 55 } );
    (void)src.push_back( order_id_t{ 606 }, order_qty_t{ 66 } );

    src.pop_at( pos_2 );
    (void)src.push_back( order_id_t{ 707 }, order_qty_t{ 77 } );

    const zipped_orders_data_t expected_initial{
        { 101, 11 }, { 303, 33 }, { 404, 44 },
        { 505, 55 }, { 606, 66 }, { 707, 77 },
    };

    ASSERT_EQ( src.size(), 6 );
    ASSERT_EQ( collect_orders( src ), expected_initial );

    aligned_buffer_t dst_buf{ Dst_Accessor::required_size( dst_cap ) };
    Dst_Accessor::initialize_layout( dst_buf.span(), dst_cap, src );
    Dst_Accessor dst{ dst_buf.span() };

    EXPECT_EQ( dst.size(), 6 );
    EXPECT_EQ( dst.capacity(), dst_cap );

    EXPECT_EQ( collect_orders( dst ), expected_initial );

    (void)dst.push_back( order_id_t{ 808 }, order_qty_t{ 88 } );
    (void)dst.push_back( order_id_t{ 909 }, order_qty_t{ 99 } );

    EXPECT_EQ( dst.size(), 8 );

    const zipped_orders_data_t expected_final{
        { 101, 11 }, { 303, 33 }, { 404, 44 }, { 505, 55 },
        { 606, 66 }, { 707, 77 }, { 808, 88 }, { 909, 99 },
    };
    EXPECT_EQ( collect_orders( dst ), expected_final );
}

// NOLINTNEXTLINE
TEST( JacobiBookDetailsSoaPriceLevelDataAccess, UpgradeToU16Expansion )
{
    run_clone_preserves_state_and_positions< soa_price_level_data_access_u16_t,
                                             soa_price_level_data_access_u8_t >();
}

// NOLINTNEXTLINE
TEST( JacobiBookDetailsSoaPriceLevelDataAccess, UpgradeToU32Expansion )
{
    run_clone_preserves_state_and_positions< soa_price_level_data_access_u32_t,
                                             soa_price_level_data_access_u16_t >();
}

// NOLINTNEXTLINE
TYPED_TEST( JacobiBookDetailsSoaPriceLevelDataAccessTests,
            ExpansionUpgradeLinkType )
{
    constexpr std::uint32_t cap1 = 4;
    aligned_buffer_t buf1{ soa_price_level_data_access_u8_t::required_size(
        cap1 ) };

    soa_price_level_data_access_u8_t::initialize_layout( buf1.span(), cap1 );
    soa_price_level_data_access_u8_t level_old{ buf1.span() };

    (void)level_old.push_back( order_id_t{ 10 }, order_qty_t{ 100 } );
    (void)level_old.push_back( order_id_t{ 20 }, order_qty_t{ 200 } );
    (void)level_old.push_back( order_id_t{ 30 }, order_qty_t{ 300 } );
    (void)level_old.push_back( order_id_t{ 40 }, order_qty_t{ 400 } );

    constexpr std::uint32_t cap2 = 8;
    aligned_buffer_t buf2{ soa_price_level_data_access_u16_t::required_size(
        cap2 ) };

    soa_price_level_data_access_u16_t::initialize_layout(
        buf2.span(), cap2, level_old );
    soa_price_level_data_access_u16_t level_new{ buf2.span() };

    ASSERT_EQ( cap2, level_new.capacity() );
    ASSERT_EQ( cap1, level_new.size() );

    {
        const zipped_orders_data_t expected_orders{
            { 10, 100 },
            { 20, 200 },
            { 30, 300 },
            { 40, 400 },
        };

        EXPECT_EQ( collect_orders( level_new ), expected_orders );
    }

    (void)level_new.push_back( order_id_t{ 11 }, order_qty_t{ 101 } );
    (void)level_new.push_back( order_id_t{ 21 }, order_qty_t{ 201 } );
    (void)level_new.push_back( order_id_t{ 31 }, order_qty_t{ 301 } );
    (void)level_new.push_back( order_id_t{ 41 }, order_qty_t{ 401 } );

    constexpr std::uint32_t cap3 = 64 * 1024 + 1;
    aligned_buffer_t buf3{ soa_price_level_data_access_u32_t::required_size(
        cap3 ) };

    soa_price_level_data_access_u32_t::initialize_layout(
        buf3.span(), cap3, level_new );
    soa_price_level_data_access_u32_t level_u32{ buf3.span() };

    {
        const zipped_orders_data_t expected_orders{
            { 10, 100 }, { 20, 200 }, { 30, 300 }, { 40, 400 },
            { 11, 101 }, { 21, 201 }, { 31, 301 }, { 41, 401 },
        };

        EXPECT_EQ( collect_orders( level_u32 ), expected_orders );
    }
}

using details::std_soa_buffers_pool_t;

// NOLINTNEXTLINE
TEST( JacobiBookDetailsStdSoaBuffersPool, SoaBufCapIndex )
{
    EXPECT_EQ( 0, std_soa_buffers_pool_t::soa_buf_cap_index( 1 ) );

    std::size_t i = 1;
    for( auto cap : std_soa_buffers_pool_t::soa_bufs_capacities )
    {
        EXPECT_EQ( i, std_soa_buffers_pool_t::soa_buf_cap_index( cap + 1 ) )
            << "i=" << i << "; cap=" << cap;
        ++i;
    }
}

//
// JacobiBookDetailsStdSoaBuffersPoolTests
//

class JacobiBookDetailsStdSoaBuffersPoolTests : public ::testing::Test
{
protected:
    details::std_soa_buffers_pool_t pool;

    template < typename Accessor >
    void expect_initialized_empty_buffer( raw_buffer_t buf,
                                          std::uint32_t expected_capacity )
    {
        ASSERT_NE( buf.data(), nullptr );
        EXPECT_EQ( reinterpret_cast< std::uintptr_t >( buf.data() )
                       % details::soa_buf_alignment,
                   0u );

        ASSERT_EQ( buf.size(), Accessor::required_size( expected_capacity ) );

        const auto validated_capacity = Accessor::validate_buffer( buf );
        EXPECT_EQ( validated_capacity, expected_capacity );

        Accessor accessor{ buf };
        EXPECT_EQ( accessor.size(), 0u );
        EXPECT_EQ( accessor.capacity(), expected_capacity );
        EXPECT_TRUE( accessor.empty() );
        EXPECT_FALSE( accessor.full() );

        EXPECT_EQ( std::distance( accessor.begin(), accessor.end() ), 0 );
        EXPECT_EQ( std::distance( accessor.rbegin(), accessor.rend() ), 0 );
    }
};

TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateBufferUsesU8Buckets )
{
    const std::pair< std::uint32_t, std::uint32_t > cases[] = {
        { 1, 17 }, { 18, 35 }, { 36, 72 }, { 73, 145 }, { 146, 254 },
    };

    for( const auto & [ request_capacity, bucket_capacity ] : cases )
    {
        auto [ buf, acc_type ] = pool.allocate_buffer( request_capacity );
        ASSERT_EQ( acc_type, data_buf_accessor_type::u8_links );
        expect_initialized_empty_buffer< soa_price_level_data_access_u8_t >(
            buf, bucket_capacity );
        pool.deallocate_buffer( buf );
    }
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateBufferUsesU16Buckets )
{
    const std::pair< std::uint32_t, std::uint32_t > cases[] = {
        { 255, 511 },
        { 512, 1022 },
    };

    for( const auto & [ request_capacity, bucket_capacity ] : cases )
    {
        auto [ buf, acc_type ] = pool.allocate_buffer( request_capacity );
        ASSERT_EQ( acc_type, data_buf_accessor_type::u16_links );
        expect_initialized_empty_buffer< soa_price_level_data_access_u16_t >(
            buf, bucket_capacity );
        pool.deallocate_buffer( buf );
    }
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests,
        AllocateBufferUsesCustomU16ForLargeRequest )
{
    const std::pair< std::uint32_t, std::uint32_t > cases[] = {
        { 1023, 1023 + 1023 / 4 },
        { 4'000, 5'000 },
        { 20'000, 25'000 },
    };

    for( const auto & [ request_capacity, expected_capacity ] : cases )
    {
        ASSERT_LE( expected_capacity,
                   static_cast< std::uint32_t >(
                       soa_price_level_data_access_u16_t::max_capacity ) );

        auto [ buf, acc_type ] = pool.allocate_buffer( request_capacity );
        ASSERT_EQ( acc_type, data_buf_accessor_type::u16_links );
        expect_initialized_empty_buffer< soa_price_level_data_access_u16_t >(
            buf, expected_capacity );
        pool.deallocate_buffer( buf );
    }
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests,
        AllocateBufferUsesCustomU32WhenU16WouldOverflow )
{
    const auto u16_max_capacity = static_cast< std::uint32_t >(
        soa_price_level_data_access_u16_t::max_capacity );

    // Smallest-ish request for which:
    //   request + request/4 > u16 max_capacity
    const auto request_capacity =
        static_cast< std::uint32_t >( ( u16_max_capacity * 4 ) / 5 + 1 );
    const auto expected_capacity =
        static_cast< std::uint32_t >( request_capacity + request_capacity / 4 );

    ASSERT_GT( expected_capacity, u16_max_capacity );

    auto [ buf, acc_type ] = pool.allocate_buffer( request_capacity );

    ASSERT_EQ( acc_type, data_buf_accessor_type::u32_links );

    expect_initialized_empty_buffer< soa_price_level_data_access_u32_t >(
        buf, expected_capacity );

    pool.deallocate_buffer( buf );
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateAndDeallocateManyBuffers )
{
    const std::uint32_t requests[] = {
        1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 0 ] + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 1 ] + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 2 ] + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 3 ] + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 4 ] + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 5 ] + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 6 ] + 1,
        4000,
    };

    std::vector< raw_buffer_t > buffers;
    buffers.reserve( std::size( requests ) );

    for( auto i = 0; i < 1000; ++i )
    {
        for( auto request : requests )
        {
            buffers.push_back( pool.allocate_buffer( request ).first );
        }

        if( i % 100 )
        {
            buffers.push_back( pool.allocate_buffer( 64 * 1024 ).first );
        }
    }

    for( auto buf : buffers )
    {
        EXPECT_NE( buf.data(), nullptr );
        EXPECT_GT( buf.size(), 8 );
        EXPECT_EQ( reinterpret_cast< std::uintptr_t >( buf.data() )
                       % details::soa_buf_alignment,
                   0 );
    }

    for( auto buf : buffers )
    {
        pool.deallocate_buffer( buf );
    }
}

//
// expect_allocate_buffer_clone_works()
//

template < typename Dst_Accessor, typename Src_Accessor >
void expect_allocate_buffer_clone_works( std_soa_buffers_pool_t & pool,
                                         std::uint32_t requested_capacity,
                                         std::uint32_t expected_capacity,
                                         Src_Accessor & src,
                                         const std::string & tag )
{
    auto [ dst_buf, acc_type ] = pool.allocate_buffer( requested_capacity, src );

    ASSERT_EQ( acc_type, Dst_Accessor::accessor_type );

    ASSERT_NE( dst_buf.data(), nullptr );
    EXPECT_EQ( reinterpret_cast< std::uintptr_t >( dst_buf.data() )
                   % details::soa_buf_alignment,
               0 );

    ASSERT_EQ( dst_buf.size(), Dst_Accessor::required_size( expected_capacity ) )
        << "src.capacity()=" << src.capacity() << "; "
        << "requested_capacity=" << requested_capacity << "; "
        << "expected_capacity=" << expected_capacity << "; "
        << "tag=" << tag << ";";

    Dst_Accessor dst{ dst_buf };
    ASSERT_EQ( dst.size(), src.size() );
    EXPECT_EQ( dst.capacity(), expected_capacity );

    EXPECT_EQ( collect_orders( dst ), collect_orders( src ) )
        << "src.capacity()=" << src.capacity() << "; "
        << "requested_capacity=" << requested_capacity << "; "
        << "expected_capacity=" << expected_capacity << "; "
        << "tag=" << tag << ";";

    const auto * const src_links = src.links();
    const auto * const dst_links = dst.links();

    // Links must be at same positions
    ASSERT_EQ( src_links[ 0 ].prev, dst_links[ 0 ].prev )
        << "src.capacity()=" << src.capacity() << "; "
        << "requested_capacity=" << requested_capacity << "; "
        << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
        << ";";
    ASSERT_EQ( src_links[ 0 ].next, dst_links[ 0 ].next )
        << "src.capacity()=" << src.capacity() << "; "
        << "requested_capacity=" << requested_capacity << "; "
        << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
        << ";";
    if( std::is_same_v< Dst_Accessor, Src_Accessor > )
    {
        ASSERT_EQ( src_links[ 1 ].prev, dst_links[ 1 ].prev )
            << "src.capacity()=" << src.capacity() << "; "
            << "requested_capacity=" << requested_capacity << "; "
            << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
            << ";";
    }
    else
    {
        ASSERT_EQ( src.capacity() + details::extra_links_for_anchors,
                   dst_links[ 1 ].prev )
            << "src.capacity()=" << src.capacity() << "; "
            << "requested_capacity=" << requested_capacity << "; "
            << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
            << ";";
    }

    if constexpr( std::is_same_v< std::uint8_t,
                                  typename Dst_Accessor::short_index_t > )
    {
        ASSERT_EQ( src.capacity() + details::extra_links_for_anchors,
                   dst_links[ 1 ].next )
            << "src.capacity()=" << src.capacity() << "; "
            << "requested_capacity=" << requested_capacity << "; "
            << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
            << ";";
    }
    else
    {
        ASSERT_EQ( src_links[ 1 ].next, dst_links[ 1 ].next )
            << "src.capacity()=" << src.capacity() << "; "
            << "requested_capacity=" << requested_capacity << "; "
            << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
            << ";";
    }

    for( auto i = 0u; i < dst.size(); ++i )
    {
        const auto j = i + details::extra_links_for_anchors;
        ASSERT_EQ( src_links[ j ].prev, dst_links[ j ].prev )
            << "j=" << j << "; src.capacity()=" << src.capacity() << "; "
            << "requested_capacity=" << requested_capacity << "; "
            << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
            << ";";
        ASSERT_EQ( src_links[ j ].next, dst_links[ j ].next )
            << "j=" << j << "; src.capacity()=" << src.capacity() << "; "
            << "requested_capacity=" << requested_capacity << "; "
            << "expected_capacity=" << expected_capacity << "; " << "tag=" << tag
            << ";";
    }

    pool.deallocate_buffer( dst_buf );
}

//
// fill_with_orders()
//

template < typename Accessor >
void fill_with_orders( Accessor & acc, std::uint64_t id, std::uint32_t qty )
{
    // Add until FULL.
    while( !acc.full() )
    {
        (void)acc.push_back( order_id_t{ id++ }, order_qty_t{ qty++ } );
    }

    auto positions = ranges::views::iota( std::uint32_t{ 0 }, acc.capacity() )
                     | ranges::views::transform(
                         []( std::uint32_t v ) {
                             return unified_soa_position_t{
                                 v + details::extra_links_for_anchors
                             };
                         } )
                     | ranges::to< std::vector< unified_soa_position_t > >();

    std::random_device rd;
    std::mt19937 g( rd() );
    std::shuffle( positions.begin(), positions.end(), g );

    // Remove some at random positions.
    while( positions.size() > acc.capacity() / 2 )
    {
        acc.pop_at( positions.back() );
        positions.pop_back();
    }

    // Add more until FULL again.
    while( !acc.full() )
    {
        (void)acc.push_back( order_id_t{ id++ }, order_qty_t{ qty++ } );
    }
};

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateWithStdCapacities )
{
    auto [ buf0, acc_type0 ] = pool.allocate_buffer( 1 );
    ASSERT_EQ( acc_type0, data_buf_accessor_type::u8_links );

    soa_price_level_data_access_u8_t src0{ buf0 };

    fill_with_orders( src0, 1, 1 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u8_t >(
        pool,
        src0.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 1 ],
        src0,
        "18 => 35" );

    // ===============================================================
    auto [ buf1, acc_type1 ] = pool.allocate_buffer( src0.capacity() + 1, src0 );
    ASSERT_EQ( acc_type1, data_buf_accessor_type::u8_links );

    soa_price_level_data_access_u8_t src1{ buf1 };
    fill_with_orders( src1, 1000, 100 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u8_t >(
        pool,
        src1.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 2 ],
        src1,
        "36 => 72" );

    // ===============================================================
    auto [ buf2, acc_type2 ] = pool.allocate_buffer( src1.capacity() + 1, src1 );
    ASSERT_EQ( acc_type2, data_buf_accessor_type::u8_links );

    soa_price_level_data_access_u8_t src2{ buf2 };
    fill_with_orders( src2, 2000, 200 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u8_t >(
        pool,
        src2.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 3 ],
        src2,
        "73 => 145" );

    // ===============================================================
    auto [ buf3, acc_type3 ] = pool.allocate_buffer( src2.capacity() + 1, src2 );
    ASSERT_EQ( acc_type3, data_buf_accessor_type::u8_links );

    soa_price_level_data_access_u8_t src3{ buf3 };
    fill_with_orders( src3, 3000, 300 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u8_t >(
        pool,
        src3.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 4 ],
        src3,
        "146 => 254" );

    // ===============================================================
    auto [ buf4, acc_type4 ] = pool.allocate_buffer( src3.capacity() + 1, src3 );
    ASSERT_EQ( acc_type4, data_buf_accessor_type::u8_links );

    soa_price_level_data_access_u8_t src4{ buf4 };
    fill_with_orders( src4, 4000, 400 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u16_t >(
        pool,
        src4.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 5 ],
        src4,
        "255 => 511" );

    // ===============================================================
    auto [ buf5, acc_type5 ] = pool.allocate_buffer( src4.capacity() + 1, src4 );
    ASSERT_EQ( acc_type5, data_buf_accessor_type::u16_links );

    soa_price_level_data_access_u16_t src5{ buf5 };
    fill_with_orders( src5, 5000, 500 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u16_t >(
        pool,
        src5.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 6 ],
        src5,
        "512 => 1022" );

    // ===============================================================
    auto [ buf6, acc_type6 ] = pool.allocate_buffer( src4.capacity() + 1, src5 );
    ASSERT_EQ( acc_type6, data_buf_accessor_type::u16_links );

    soa_price_level_data_access_u16_t src6{ buf6 };
    fill_with_orders( src6, 6000, 600 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u16_t >(
        pool,
        src6.capacity() + 1,
        std_soa_buffers_pool_t::soa_bufs_capacities[ 6 ],
        src6,
        "1023 => ..." );
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateForAtLeast1024Capacity )
{
    auto [ buf0, acc_type0 ] = pool.allocate_buffer( 512 );
    ASSERT_EQ( acc_type0, data_buf_accessor_type::u16_links );

    soa_price_level_data_access_u16_t src0{ buf0 };

    ASSERT_EQ( src0.capacity(), 1022 );

    fill_with_orders( src0, 1, 1 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u16_t >(
        pool, src0.capacity() + 1, 1278, src0, "1022 => 1278" );
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateBecomesBoss )
{
    // Growth is defined as:
    //
    //            n = (n+1) + (n+1)/4
    //
    // After few iteration starting with n=1023 (last standard capacity).
    // becomes 56870 (preceded by 45495) and the next
    // will be 71088 for which we need u32-links.
    auto [ buf0, acc_type0 ] = pool.allocate_buffer( 45495 + 1 );
    ASSERT_EQ( acc_type0, data_buf_accessor_type::u16_links );

    soa_price_level_data_access_u16_t src0{ buf0 };

    ASSERT_EQ( src0.capacity(), 56870 );

    fill_with_orders( src0, 1, 1 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u32_t >(
        pool, src0.capacity() + 1, 71088, src0, "56870 => 71088" );
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, AllocateLikeABoss )
{
    auto [ buf0, acc_type0 ] = pool.allocate_buffer( 56870 + 1 );
    ASSERT_EQ( acc_type0, data_buf_accessor_type::u32_links );

    soa_price_level_data_access_u32_t src0{ buf0 };

    ASSERT_EQ( src0.capacity(), 71088 );

    fill_with_orders( src0, 1, 1 );

    expect_allocate_buffer_clone_works< soa_price_level_data_access_u32_t >(
        pool,
        src0.capacity() + 1,
        src0.capacity() + 1 + ( src0.capacity() + 1 ) / 4,
        src0,
        "71088 => ..." );
}

// NOLINTNEXTLINE
TEST_F( JacobiBookDetailsStdSoaBuffersPoolTests, DeallocateDummyBuffer )
{
    pool.deallocate_buffer( std_soa_buffers_pool_t::make_zero_capacity_buffer() );
}

}  // anonymous namespace

}  // namespace jacobi::book
