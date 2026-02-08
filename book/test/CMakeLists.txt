set(test_prj _utest.jacobi.book)

project(${test_prj})

list(APPEND  unittests_srcfiles
    vocabulary_types.cpp
    price_level.cpp
    order_refs_index.cpp
    book.cpp

    utils/chunk_list.cpp

    map/orders_table.cpp
    linear/orders_table.cpp
    mixed/lru/orders_table.cpp
    mixed/hot_cold/orders_table.cpp
)

add_executable(${test_prj} ${unittests_srcfiles})

add_executable(${test_prj}.book_more_tests.map1           book_more_tests.cpp)
add_executable(${test_prj}.book_more_tests.map2           book_more_tests.cpp)
add_executable(${test_prj}.book_more_tests.linear_v1      book_more_tests.cpp)
add_executable(${test_prj}.book_more_tests.linear_v2      book_more_tests.cpp)
add_executable(${test_prj}.book_more_tests.linear_v3      book_more_tests.cpp)
add_executable(${test_prj}.book_more_tests.mixed_lru      book_more_tests.cpp)
add_executable(${test_prj}.book_more_tests.mixed_hot_cold book_more_tests.cpp)


if (WIN32)
    target_compile_definitions(${test_prj} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.map1           _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.map2           _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.linear_v1      _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.linear_v2      _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.linear_v3      _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.mixed_lru      _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(${test_prj}.book_more_tests.mixed_hot_cold _CRT_SECURE_NO_WARNINGS)
endif ()

list(APPEND jacobi_book_test_deps
    GTest::gtest_main
    jacobi::book
    jacobi::snapshots
)

target_link_libraries(${test_prj}  PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.map1             PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.map2             PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.linear_v1       PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.linear_v2       PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.linear_v3       PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.mixed_lru       PRIVATE  ${jacobi_book_test_deps})
target_link_libraries(${test_prj}.book_more_tests.mixed_hot_cold  PRIVATE  ${jacobi_book_test_deps})


target_compile_definitions( ${test_prj}.book_more_tests.map1            PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_MAP_ONES_1
)
target_compile_definitions( ${test_prj}.book_more_tests.map2            PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_MAP_ONES_2
)
target_compile_definitions( ${test_prj}.book_more_tests.linear_v1      PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_LINEAR_V1_ONES
)
target_compile_definitions( ${test_prj}.book_more_tests.linear_v2      PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_LINEAR_V2_ONES
)
target_compile_definitions( ${test_prj}.book_more_tests.linear_v3      PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_LINEAR_V3_ONES
)
target_compile_definitions( ${test_prj}.book_more_tests.mixed_lru      PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_MIXED_LRU_ONES
)
target_compile_definitions( ${test_prj}.book_more_tests.mixed_hot_cold PRIVATE
    JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
    JACOBI_BOOK_TESTS_SPLIT_MIXED_HOT_COLD_ONES
)
target_compile_options( ${test_prj}.book_more_tests.mixed_hot_cold PRIVATE -fno-omit-frame-pointer -g)

include(GoogleTest)

# Distinguishing tests for CI ([Debug, Release, ...] x [devtoolset, other gcc, clang],,,)
# If the variable is defined test report would also include this tag
# as a part of the test name.
gtest_discover_tests(${test_prj} TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.map1             TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.map2             TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.linear_v1       TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.linear_v2       TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.linear_v3       TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.mixed_lru       TEST_PREFIX "${JACOBI_TEST_PREFIX}")
gtest_discover_tests(${test_prj}.book_more_tests.mixed_hot_cold  TEST_PREFIX "${JACOBI_TEST_PREFIX}")
