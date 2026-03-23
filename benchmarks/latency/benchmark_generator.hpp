#pragma once

// Standard instantiation of throughput benchmarks.

#define JACOBI_BENCHMARK_LATENCY_TEMPLATE(                                      \
    func, traits_type, benchmark_name, events, rng, measurement_count, filter ) \
    if( std::regex_search( benchmark_name, filter ) )                           \
    {                                                                           \
        func< traits_type >( benchmark_name, events, rng, measurement_count );  \
    }

// clang-format off
#define JACOBI_GENERATE_BENCHMARKS_XXX_STORAGE(              \
    func, events, rng, measurement_count, filter )           \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl11_refIX1_t, "bsn1_plvl11_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl11_refIX2_t, "bsn1_plvl11_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl11_refIX3_t, "bsn1_plvl11_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl11_refIX4_t, "bsn1_plvl11_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl13_refIX1_t, "bsn1_plvl13_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl13_refIX2_t, "bsn1_plvl13_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl13_refIX3_t, "bsn1_plvl13_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl13_refIX4_t, "bsn1_plvl13_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl21_refIX1_t, "bsn1_plvl21_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl21_refIX2_t, "bsn1_plvl21_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl21_refIX3_t, "bsn1_plvl21_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl21_refIX4_t, "bsn1_plvl21_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl30_refIX1_t, "bsn1_plvl30_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl30_refIX2_t, "bsn1_plvl30_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl30_refIX3_t, "bsn1_plvl30_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl30_refIX4_t, "bsn1_plvl30_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl31_refIX1_t, "bsn1_plvl31_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl31_refIX2_t, "bsn1_plvl31_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl31_refIX3_t, "bsn1_plvl31_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl31_refIX4_t, "bsn1_plvl31_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl32_refIX1_t, "bsn1_plvl32_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl32_refIX2_t, "bsn1_plvl32_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl32_refIX3_t, "bsn1_plvl32_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl32_refIX4_t, "bsn1_plvl32_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl51_refIX1_t, "bsn1_plvl51_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl51_refIX2_t, "bsn1_plvl51_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl51_refIX3_t, "bsn1_plvl51_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl51_refIX4_t, "bsn1_plvl51_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl53_refIX1_t, "bsn1_plvl53_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl53_refIX2_t, "bsn1_plvl53_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl53_refIX3_t, "bsn1_plvl53_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn1_plvl53_refIX4_t, "bsn1_plvl53_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl11_refIX1_t, "bsn2_plvl11_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl11_refIX2_t, "bsn2_plvl11_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl11_refIX3_t, "bsn2_plvl11_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl11_refIX4_t, "bsn2_plvl11_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl13_refIX1_t, "bsn2_plvl13_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl13_refIX2_t, "bsn2_plvl13_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl13_refIX3_t, "bsn2_plvl13_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl13_refIX4_t, "bsn2_plvl13_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl21_refIX1_t, "bsn2_plvl21_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl21_refIX2_t, "bsn2_plvl21_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl21_refIX3_t, "bsn2_plvl21_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl21_refIX4_t, "bsn2_plvl21_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl30_refIX1_t, "bsn2_plvl30_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl30_refIX2_t, "bsn2_plvl30_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl30_refIX3_t, "bsn2_plvl30_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl30_refIX4_t, "bsn2_plvl30_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl31_refIX1_t, "bsn2_plvl31_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl31_refIX2_t, "bsn2_plvl31_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl31_refIX3_t, "bsn2_plvl31_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl31_refIX4_t, "bsn2_plvl31_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl32_refIX1_t, "bsn2_plvl32_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl32_refIX2_t, "bsn2_plvl32_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl32_refIX3_t, "bsn2_plvl32_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl32_refIX4_t, "bsn2_plvl32_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl51_refIX1_t, "bsn2_plvl51_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl51_refIX2_t, "bsn2_plvl51_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl51_refIX3_t, "bsn2_plvl51_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl51_refIX4_t, "bsn2_plvl51_refIX4", events, rng, measurement_count, filter ) \
                                                             \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl53_refIX1_t, "bsn2_plvl53_refIX1", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl53_refIX2_t, "bsn2_plvl53_refIX2", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl53_refIX3_t, "bsn2_plvl53_refIX3", events, rng, measurement_count, filter ) \
    JACOBI_BENCHMARK_LATENCY_TEMPLATE( func, bsn2_plvl53_refIX4_t, "bsn2_plvl53_refIX4", events, rng, measurement_count, filter )

// clang-format on
