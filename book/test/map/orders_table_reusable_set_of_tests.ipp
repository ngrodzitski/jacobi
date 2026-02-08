// Contains reusable set of tests for orders table implementation.
// Requires the upper level file to define a few names:
//
// * JACOBI_BOOK_TYPE_ORDERS_TABLE
// * JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
// * JACOBI_BOOK_ORDERS_TABLE_TYPE
//
//

#define JACOBI_WRAPPER_TEST( Suite, Name ) TEST( Suite, Name )
#define JACOBI_WRAPPER_TEST_F( Suite, Name ) TEST_F( Suite, Name )

std::vector< order_t > orders{
    order_t{ order_id_t{ 0 }, order_qty_t{ 10 }, order_price_t{ 333 } },
    order_t{ order_id_t{ 1 }, order_qty_t{ 50 }, order_price_t{ 222 } },
    order_t{ order_id_t{ 2 }, order_qty_t{ 100 }, order_price_t{ 222 } },
    order_t{ order_id_t{ 3 }, order_qty_t{ 42 }, order_price_t{ 300 } },
};

// NOLINTNEXTLINE
JACOBI_WRAPPER_TEST( JACOBI_BOOK_TYPE_ORDERS_TABLE, AddDeleteOrders )
{
    book_impl_data_t book_impl_data{};

    JACOBI_BOOK_ORDERS_TABLE_TYPE orders_table{ book_impl_data };

    // ===============================================================
    // Add orders
    ASSERT_TRUE( orders_table.empty() );
    ASSERT_FALSE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_FALSE( static_cast< bool >( orders_table.top_price_qty() ) );

    ASSERT_NE( orders_table.add_order( orders[ 0 ] ),
               book_impl_data.order_refs_index.end() );

    ASSERT_FALSE( orders_table.empty() );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 333 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 10 } );

    ASSERT_NE( orders_table.add_order( orders[ 1 ] ),
               book_impl_data.order_refs_index.end() );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 50 } );

    ASSERT_NE( orders_table.add_order( orders[ 2 ] ),
               book_impl_data.order_refs_index.end() );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 150 } );

    ASSERT_NE( orders_table.add_order( orders[ 3 ] ),
               book_impl_data.order_refs_index.end() );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 150 } );

    // ==============================================================

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [50]   [100]

    // ===============================================================

    // ===============================================================
    // Delete some
    orders_table.delete_order( orders[ 2 ].id );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [50]
    //               ******
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 50 } );

    orders_table.delete_order( orders[ 3 ].id );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300
    //   ...  ******
    //   $222  [50]
    //
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 50 } );

    orders_table.delete_order( orders[ 1 ].id );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300
    //   ...
    //   $222
    //         ******
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 333 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 10 } );

    orders_table.delete_order( orders[ 0 ].id );
    // Trade Side (sell):
    //   $333
    //   ...  ******
    //   ...
    //   $300
    //   ...
    //   $222
    //   ...
    ASSERT_TRUE( orders_table.empty() );
    ASSERT_FALSE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_FALSE( static_cast< bool >( orders_table.top_price_qty() ) );
}

// NOLINTNEXTLINE
JACOBI_WRAPPER_TEST( JACOBI_BOOK_TYPE_ORDERS_TABLE, AddAndDeleteSingle )
{
    book_impl_data_t book_impl_data{};
    JACOBI_BOOK_ORDERS_TABLE_TYPE orders_table{ book_impl_data };

    ASSERT_NE( orders_table.add_order( orders[ 0 ] ),
               book_impl_data.order_refs_index.end() );
    orders_table.delete_order( orders[ 0 ].id );
}

//
// JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE
//

class JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE : public ::testing::Test
{
protected:
    book_impl_data_t book_impl_data{};
    JACOBI_BOOK_ORDERS_TABLE_TYPE orders_table{ book_impl_data };

    void SetUp() override
    {
        for( const auto order : orders )
        {
            orders_table.add_order( order );
        }
        // Trade Side (sell):
        //   $333  [10]
        //   ...
        //   $300  [42]
        //   ...
        //   $222  [50]   [100]
    }
};

JACOBI_WRAPPER_TEST_F( JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE, ExecuteSome )
{
    // ===============================================================
    // START

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [50]   [100]

    // ===============================================================
    orders_table.execute_order( orders[ 1 ].id, order_qty_t{ 25 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [25]   [100]
    //        ******
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 125 } );

    // ===============================================================
    orders_table.execute_order( orders[ 1 ].id, order_qty_t{ 25 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222        [100]
    //        ******
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 100 } );
    // ===============================================================

    orders_table.execute_order( orders[ 2 ].id, order_qty_t{ 1 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222        [99]
    //              ******
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 99 } );
    // ===============================================================

    orders_table.execute_order( orders[ 2 ].id, order_qty_t{ 49 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222        [50]
    //              ******
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 50 } );
    // ===============================================================

    orders_table.execute_order( orders[ 2 ].id, order_qty_t{ 50 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222
    //              ******
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 300 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 42 } );
    // ===============================================================

    orders_table.execute_order( orders[ 3 ].id, order_qty_t{ 30 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [12]
    //   ...  ******
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 300 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 12 } );
    // ===============================================================

    orders_table.execute_order( orders[ 3 ].id, order_qty_t{ 12 } );
    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300
    //   ...  ******
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_TRUE( static_cast< bool >( orders_table.top_price_qty() ) );
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 333 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 10 } );
    // ===============================================================

    orders_table.execute_order( orders[ 0 ].id, order_qty_t{ 10 } );
    // Trade Side (sell):
    //   $333
    //   ...  ******
    ASSERT_TRUE( orders_table.empty() );
    ASSERT_FALSE( static_cast< bool >( orders_table.top_price() ) );
    ASSERT_FALSE( static_cast< bool >( orders_table.top_price_qty() ) );
    // ===============================================================
}

JACOBI_WRAPPER_TEST_F( JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE, ReduceSome )
{
    // ===============================================================
    // START

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [50]   [100]

    orders_table.reduce_order( orders[ 1 ].id, order_qty_t{ 49 } );
    orders_table.reduce_order( orders[ 2 ].id, order_qty_t{ 49 } );

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [ 1]   [51]
    //        ****** ******

    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 52 } );

    // ===============================================================
    orders_table.reduce_order( orders[ 0 ].id, order_qty_t{ 7 } );
    orders_table.reduce_order( orders[ 3 ].id, order_qty_t{ 2 } );

    // Trade Side (sell):
    //   $333  [ 3]
    //   ...  ******
    //   $300  [40]
    //   ...  ******
    //   $222  [ 1]   [51]

    orders_table.delete_order( orders[ 1 ].id );
    orders_table.delete_order( orders[ 2 ].id );

    // ===============================================================
    // Trade Side (sell):
    //   $333  [ 3]
    //   ...
    //   $300  [40]
    //   ...
    //   $222
    //        ****** ******
    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 300 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 40 } );

    // ===============================================================
    orders_table.delete_order( orders[ 3 ].id );
    // Trade Side (sell):
    //   $333  [ 3]
    //   ...
    //   $300
    //   ...  ******
    //   ...
    //   $222

    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 333 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 3 } );
}

JACOBI_WRAPPER_TEST_F( JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE, ModifySome )
{
    // ===============================================================
    // START

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [50]   [100]

    // ===============================================================
    {
        // Modify only qty.
        auto order = orders[ 1 ];
        order.qty  = order_qty_t{ 10 };
        orders_table.modify_order( order );
    }

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222       [100]   [10]
    //        ******       ******

    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 110 } );

    // ===============================================================
    {
        // Modify only price.
        auto order  = orders[ 0 ];
        order.price = order_price_t{ 220 };
        orders_table.modify_order( order );
    }

    // Trade Side (sell):
    //   $333
    //   ...  ******
    //   $300  [42]
    //   ...
    //   $222  [100]   [10]
    //   $221
    //   $220  [10]
    //        ******

    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 220 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 10 } );

    // ===============================================================
    {
        // Modify price and qty.
        auto order  = orders[ 0 ];
        order.price = order_price_t{ 222 };
        order.qty   = order_qty_t{ 30 };
        orders_table.modify_order( order );
    }

    // Trade Side (sell):
    //   $333
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [100]   [10]   [30]
    //   $221                ******
    //   $220
    //        ******
    //

    ASSERT_EQ( orders_table.top_price().value(), order_price_t{ 222 } );
    ASSERT_EQ( orders_table.top_price_qty().value(), order_qty_t{ 140 } );
}

JACOBI_WRAPPER_TEST_F( JACOBI_BOOK_TYPE_ORDERS_TABLE_FIXTURE, LevelsRange )
{
    // ===============================================================
    // START

    // Trade Side (sell):
    //   $333  [10]
    //   ...
    //   $300  [42]
    //   ...
    //   $222  [50]   [100]

    auto rng = orders_table.levels_range();

    const auto n = std::distance( rng.begin(), rng.end() );

    ASSERT_EQ( n, 3 );

    auto it = rng.begin();
    EXPECT_EQ( it->price(), order_price_t{ 222 } );

    ++it;
    EXPECT_EQ( it->price(), order_price_t{ 300 } );

    ++it;
    EXPECT_EQ( it->price(), order_price_t{ 333 } );
}
