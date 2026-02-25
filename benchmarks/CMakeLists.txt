set(logr_benchmark_prj logr_benchmark)

project(${logr_benchmark_prj})

if (WIN32)
    set(jacobi_perf_flags)
else ()
    set(jacobi_perf_flags -fno-omit-frame-pointer -g)
endif ()

set(JACOBI_BENCHMARKS_COMMON_LIB jacobi_benchmarks_common)

add_library(jacobi_bench.profiling STATIC
    benchmark_helpers.hpp

    profiling.hpp
    profiling.cpp
)
target_link_libraries(jacobi_bench.profiling PRIVATE fmt::fmt)

target_compile_options(jacobi_bench.profiling PUBLIC ${jacobi_perf_flags})

set(JACOBI_BENCH_COMMON_LIBS
  benchmark::benchmark
  jacobi::book
  jacobi::snapshots
  jacobi_bench.profiling
)

# Usage:
#   jacobi_add_throughput_bench(_bench.throughput.map_levels_storage
#     SOURCES
#       map_levels_storage_book_types.hpp
#       throughput_map_levels_storage_book.cpp
#   )
macro(jacobi_add_throughput_bench target_name)
    set(_opts)
    set(_one_value_args)
    set(_multi_value_args SOURCES )
    cmake_parse_arguments(_B "${_opts}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    if (NOT _B_SOURCES)
        message(
            FATAL_ERROR
            "jacobi_add_throughput_bench(${target_name} ...): missing SOURCES"
        )
    endif()

    add_executable(${target_name} ${_B_SOURCES})

    target_link_libraries(${target_name} PRIVATE
        benchmark::benchmark
        jacobi::book
        jacobi::snapshots
        jacobi_bench.profiling
    )
endmacro()

# Usage:
#   jacobi_add_latency_bench(_bench.throughput.map_levels_storage
#     SOURCES
#       map_levels_storage_book_types.hpp
#       latency_map_levels_storage_book.cpp
#   )
macro(jacobi_add_latency_bench target_name)
    set(_opts)
    set(_one_value_args)
    set(_multi_value_args SOURCES )
    cmake_parse_arguments(_B "${_opts}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    if (NOT _B_SOURCES)
        message(
            FATAL_ERROR
            "jacobi_add_throughput_bench(${target_name} ...): missing SOURCES"
        )
    endif()

    add_executable(${target_name} ${_B_SOURCES})

    target_link_libraries(${target_name} PRIVATE
        CLI11::CLI11
        jacobi::book
        jacobi::snapshots
    )
endmacro()

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

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.map_levels_storage.multi_book
    SOURCES
    map_levels_storage_book_types.hpp
    throughput/map_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.map_levels_storage.single_book
    SOURCES
    map_levels_storage_book_types.hpp
    throughput/map_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.map_levels_storage
    SOURCES
    map_levels_storage_book_types.hpp
    latency/map_levels_storage_book.cpp
)
# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.absl_map_levels_storage.multi_book
    SOURCES
    absl_map_levels_storage_book_types.hpp
    throughput/absl_map_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.absl_map_levels_storage.single_book
    SOURCES
    absl_map_levels_storage_book_types.hpp
    throughput/absl_map_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.absl_map_levels_storage
    SOURCES
    absl_map_levels_storage_book_types.hpp
    latency/absl_map_levels_storage_book.cpp
)

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.linear_v1_levels_storage.multi_book
    SOURCES
    linear_v1_levels_storage_book_types.hpp
    throughput/linear_v1_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.linear_v1_levels_storage.single_book
    SOURCES
    linear_v1_levels_storage_book_types.hpp
    throughput/linear_v1_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.linear_v1_levels_storage
    SOURCES
    linear_v1_levels_storage_book_types.hpp
    latency/linear_v1_levels_storage_book.cpp
)

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.linear_v2_levels_storage.multi_book
    SOURCES
    linear_v2_levels_storage_book_types.hpp
    throughput/linear_v2_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.linear_v2_levels_storage.single_book
    SOURCES
    linear_v2_levels_storage_book_types.hpp
    throughput/linear_v2_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.linear_v2_levels_storage
    SOURCES
    linear_v2_levels_storage_book_types.hpp
    latency/linear_v2_levels_storage_book.cpp
)

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.linear_v3_levels_storage.multi_book
    SOURCES
    linear_v3_levels_storage_book_types.hpp
    throughput/linear_v3_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.linear_v3_levels_storage.single_book
    SOURCES
    linear_v3_levels_storage_book_types.hpp
    throughput/linear_v3_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.linear_v3_levels_storage
    SOURCES
    linear_v3_levels_storage_book_types.hpp
    latency/linear_v3_levels_storage_book.cpp
)

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.mixed_lru_levels_storage.multi_book
    SOURCES
    mixed_lru_levels_storage_book_types.hpp
    throughput/mixed_lru_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.mixed_lru_levels_storage.single_book
    SOURCES
    mixed_lru_levels_storage_book_types.hpp
    throughput/mixed_lru_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.mixed_lru_levels_storage
    SOURCES
    mixed_lru_levels_storage_book_types.hpp
    latency/mixed_lru_levels_storage_book.cpp
)

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.mixed_lru_v2_levels_storage.multi_book
    SOURCES
    mixed_lru_v2_levels_storage_book_types.hpp
    throughput/mixed_lru_v2_levels_storage_multi_book.cpp
)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.mixed_lru_v2_levels_storage.single_book
    SOURCES
    mixed_lru_v2_levels_storage_book_types.hpp
    throughput/mixed_lru_v2_levels_storage_single_book.cpp
)
# ===============================================
jacobi_add_latency_bench( _bench.latency.mixed_lru_v2_levels_storage
    SOURCES
    mixed_lru_v2_levels_storage_book_types.hpp
    latency/mixed_lru_v2_levels_storage_book.cpp
)

# ===============================================
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.mixed_hot_cold_levels_storage.multi_book
    SOURCES
    mixed_hot_cold_levels_storage_book_types.hpp
    throughput/mixed_hot_cold_levels_storage_multi_book.cpp
)
target_compile_definitions(_bench.throughput.mixed_hot_cold_levels_storage.multi_book PRIVATE JACOBI_MIXED_HOT_COLD)
# ===============================================
jacobi_add_throughput_bench( _bench.throughput.mixed_hot_cold_levels_storage.single_book
    SOURCES
    mixed_hot_cold_levels_storage_book_types.hpp
    throughput/mixed_hot_cold_levels_storage_single_book.cpp
)
target_compile_definitions(_bench.throughput.mixed_hot_cold_levels_storage.single_book PRIVATE JACOBI_MIXED_HOT_COLD)
# ===============================================
jacobi_add_latency_bench( _bench.latency.mixed_hot_cold_levels_storage
    SOURCES
    mixed_hot_cold_levels_storage_book_types.hpp
    latency/mixed_hot_cold_levels_storage_book.cpp
)
target_compile_definitions(_bench.latency.mixed_hot_cold_levels_storage PRIVATE JACOBI_MIXED_HOT_COLD)
