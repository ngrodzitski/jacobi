set(test_prj _utest.jacobi.book)

project(${test_prj})

list(APPEND  unittests_srcfiles
    vocabulary_types.cpp
    price_level.cpp
    soa_price_level2.cpp
    order_refs_index.cpp
    book.cpp

    utils/intrusive_nodes_pool.cpp

    map/orders_table.cpp
    linear/orders_table.cpp
    mixed/lru/orders_table.cpp
    mixed/hot_cold/orders_table.cpp
)

add_executable(${test_prj} ${unittests_srcfiles})

if (WIN32)
    target_compile_definitions(${test_prj} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif ()

list(APPEND jacobi_book_test_deps
    GTest::gtest_main
    jacobi::book
    jacobi::snapshots
)

target_link_libraries(${test_prj}  PRIVATE  ${jacobi_book_test_deps})

include(GoogleTest)
gtest_discover_tests(${test_prj} TEST_PREFIX "${JACOBI_TEST_PREFIX}")


set(levels_storage_variants "MAP" "LINEAR" "MIXED")
set(price_levels_variants
    "PLVL11" "PLVL13"
    "PLVL21"
    "PLVL30" "PLVL31" "PLVL32"
    "PLVL51" "PLVL53"
)
set(reference_index_variants  "REFIX1" "REFIX2" "REFIX3" "REFIX4" )

foreach (storage IN LISTS levels_storage_variants)
    foreach (plvl IN LISTS price_levels_variants)
        foreach (refix IN LISTS reference_index_variants)
            set(subtest_target "${test_prj}.book_more_tests.${storage}.${plvl}.${refix}")

            add_executable(${subtest_target} book_more_tests.cpp)
            target_compile_definitions(
                ${subtest_target} PRIVATE
                JACOBI_BOOK_TEST_${plvl}
                JACOBI_BOOK_TEST_${refix}
                JACOBI_BOOK_TEST_LEVELS_STORAGE_${storage}
                JACOBI_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
            )

            if (WIN32)
                target_compile_definitions(${subtest_target} PRIVATE _CRT_SECURE_NO_WARNINGS)
            endif ()
            target_link_libraries(${subtest_target} PRIVATE  ${jacobi_book_test_deps})

            gtest_discover_tests(${subtest_target} TEST_PREFIX "${JACOBI_TEST_PREFIX}")
        endforeach ()
    endforeach ()
endforeach ()
