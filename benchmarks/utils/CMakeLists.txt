add_executable(reset_ids_util reset_ids_util.cpp)

target_link_libraries(reset_ids_util PRIVATE
    CLI11::CLI11
    jacobi::book
    jacobi::snapshots
)

add_executable(analyze_book analyze_book.cpp)

target_link_libraries(analyze_book PRIVATE
    CLI11::CLI11
    jacobi::book
    jacobi::snapshots
)

if (JACOBI_NTVITCH_CONVERTER)
    add_executable(ntvitch2jacobi ntvitch2jacobi.cpp)

    target_link_libraries(ntvitch2jacobi PRIVATE
        CLI11::CLI11
        boost::boost
        jacobi::book
        jacobi::snapshots
        jacobi::ntvitch
        jacobi::ntvitch_fmt
    )
endif ()
