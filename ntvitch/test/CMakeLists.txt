set(test_prj _utest.jacobi.ntvitch)

project(${test_prj})

list(APPEND  unittests_srcfiles
    ntvitch.cpp
)

add_executable(${test_prj} ${unittests_srcfiles})


if (WIN32)
    target_compile_definitions(${test_prj} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif ()

list(APPEND jacobi_book_test_deps
    GTest::gtest_main
    jacobi::ntvitch
    jacobi::ntvitch_fmt
)

target_link_libraries(${test_prj}  PRIVATE  ${jacobi_book_test_deps})

include(GoogleTest)

# Distinguishing tests for CI ([Debug, Release, ...] x [devtoolset, other gcc, clang],,,)
# If the variable is defined test report would also include this tag
# as a part of the test name.
gtest_discover_tests(${test_prj} TEST_PREFIX "${JACOBI_TEST_PREFIX}")
