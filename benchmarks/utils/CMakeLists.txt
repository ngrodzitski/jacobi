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
