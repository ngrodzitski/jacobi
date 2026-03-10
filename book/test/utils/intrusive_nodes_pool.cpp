#include <jacobi/book/utils/intrusive_nodes_pool.hpp>

#include <unordered_set>

#include <gtest/gtest.h>

namespace jacobi::book::utils
{

namespace /* anonymous */
{

//
// test_node_t
//

struct test_node_t
{
    int value{ 0 };

    boost::intrusive::list_member_hook<
        boost::intrusive::link_mode< boost::intrusive::safe_link > >
        hook;
};

using test_member_hook_t = boost::intrusive::
    member_hook< test_node_t, decltype( test_node_t::hook ), &test_node_t::hook >;

template < std::size_t PageSize >
using test_pool_t = jacobi::book::utils::
    intrusive_nodes_pool_t< test_node_t, test_member_hook_t, PageSize >;

// NOLINTNEXTLINE
TEST( JacobiBookIntrusiveNodesPool, DefaultConstructedPoolIsEmpty )
{
    test_pool_t< 4 > pool;

    EXPECT_EQ( pool.allocated_pages_count(), 0U );
    EXPECT_EQ( pool.free_nodes_count(), 0U );
}

TEST( JacobiBookIntrusiveNodesPool, InitialPageCountPreallocatesNodes )
{
    constexpr std::size_t page_size = 4;
    constexpr std::size_t pages_cnt = 3;

    test_pool_t< page_size > pool{ pages_cnt };

    EXPECT_EQ( pool.allocated_pages_count(), pages_cnt );
    EXPECT_EQ( pool.free_nodes_count(), pages_cnt * page_size );
}

TEST( JacobiBookIntrusiveNodesPool, AllocateConsumesOneFreeNode )
{
    test_pool_t< 4 > pool{ 1 };

    auto * node = pool.allocate();

    ASSERT_NE( node, nullptr );
    EXPECT_EQ( pool.allocated_pages_count(), 1U );
    EXPECT_EQ( pool.free_nodes_count(), 3U );
}

TEST( JacobiBookIntrusiveNodesPool, AllocateAddsPageWhenPoolIsEmpty )
{
    constexpr std::size_t page_size = 4;
    test_pool_t< page_size > pool{};

    auto * node = pool.allocate();

    ASSERT_NE( node, nullptr );
    EXPECT_EQ( pool.allocated_pages_count(), 1U );
    EXPECT_EQ( pool.free_nodes_count(), page_size - 1 );
}

TEST( JacobiBookIntrusiveNodesPool,
      MultipleAllocationsFromSinglePageReturnDistinctNodes )
{
    constexpr std::size_t page_size = 4;
    test_pool_t< page_size > pool{ 1 };

    std::unordered_set< test_node_t * > nodes;
    for( std::size_t i = 0; i < page_size; ++i )
    {
        auto * node = pool.allocate();
        ASSERT_NE( node, nullptr );
        EXPECT_TRUE( nodes.insert( node ).second );
    }

    EXPECT_EQ( pool.allocated_pages_count(), 1U );
    EXPECT_EQ( pool.free_nodes_count(), 0u );
}

TEST( JacobiBookIntrusiveNodesPool, ExhaustingPageTriggersSecondPageAllocation )
{
    constexpr std::size_t page_size = 4;
    test_pool_t< page_size > pool{ 1 };

    std::unordered_set< test_node_t * > nodes;
    for( std::size_t i = 0; i < page_size + 1; ++i )
    {
        auto * node = pool.allocate();
        ASSERT_NE( node, nullptr );
        EXPECT_TRUE( nodes.insert( node ).second );
    }

    EXPECT_EQ( pool.allocated_pages_count(), 2U );
    EXPECT_EQ( pool.free_nodes_count(), page_size - 1 );
}

TEST( JacobiBookIntrusiveNodesPool, DeallocateReturnsNodeToFreeList )
{
    test_pool_t< 4 > pool{ 1 };

    auto * node = pool.allocate();
    ASSERT_NE( node, nullptr );
    EXPECT_EQ( pool.free_nodes_count(), 3U );

    pool.deallocate( node );
    EXPECT_EQ( pool.free_nodes_count(), 4U );
}

TEST( JacobiBookIntrusiveNodesPool, DeallocatedNodeCanBeReused )
{
    test_pool_t< 4 > pool{ 1 };

    auto * n1                  = pool.allocate();
    [[maybe_unused]] auto * n2 = pool.allocate();
    [[maybe_unused]] auto * n3 = pool.allocate();
    [[maybe_unused]] auto * n4 = pool.allocate();

    pool.deallocate( n1 );
    auto * reused = pool.allocate();

    // Because we allocated all node in the page, and then deallocate one
    // it must be the only option for reuse, even if strategy of reusing
    // changes compared to current `free_nodes.push_front(node)`.
    EXPECT_EQ( reused, n1 );
}

TEST( JacobiBookIntrusiveNodesPool, AllocateManyTracksPageCountCorrectly )
{
    constexpr std::size_t page_size       = 4;
    constexpr std::size_t allocations_cnt = 10;

    test_pool_t< page_size > pool;

    std::unordered_set< test_node_t * > nodes;
    for( std::size_t i = 0; i < allocations_cnt; ++i )
    {
        auto * node = pool.allocate();
        ASSERT_NE( node, nullptr );
        EXPECT_TRUE( nodes.insert( node ).second );
    }

    // ceil(10 / 4) == 3
    EXPECT_EQ( pool.allocated_pages_count(), 3U );
    EXPECT_EQ( pool.free_nodes_count(), 2U );
}

}  // anonymous namespace

}  // namespace jacobi::book::utils
