#ifdef _MSC_VER
#    pragma pack( push, 1 )
#    define JACOBI_BENCH_PACKED
#else
#    define JACOBI_BENCH_PACKED [[gnu::packed]]
#endif
