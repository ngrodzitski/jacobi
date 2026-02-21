# JACOBI Project Benchmark Report (Feb 2026)

  * [Build Instructions](#build-instructions)
  * [Benchmark Dataset](#benchmark-dataset)
  * [Benchmark Settings](#benchmark-settings)
  * [Benchmark Raw Results](#benchmark-raw-results)
  * [Benchmark Processed Results](#benchmark-processed-results)
  * [Results Analysis](#results-analysis)
    * [Rank of Order Book implementations](#rank-of-order-book-implementations)
    * [Visual Reports](#visual-reports)
  * [Reflection on the Results](#reflection-on-the-results)
    * [Edge Cases](#edge-cases)
    * [Gather an Analise Perf-stats Telemetry](#gather-an-analise-perf-stats-telemetry)
    * [Linear Pathological Case](#linear-pathological-case)
    * [The Mixed Hot/Cold Pathological Case](#the-mixed-hotcold-pathological-case)

<!-- Created by https://github.com/ekalinin/github-markdown-toc -->

[JACOBI](https://github.com/ngrodzitski/jacobi)
explores various design approaches for implementing high-performance,
low-latency Order Books (L3) in C++.

This document details a comprehensive case study running
the **JACOBI** benchmark suite against actual market data
(streams of adds, modifies, and executes) to compare implementations strategies.

The experiment was executed following the
[Running a Benchmark Experiment](https://github.com/ngrodzitski/jacobi/blob/main/benchmark_experiment/RunningBenchmarkExperiment.md)
guidelines using tag [benchmark-feb2026](https://github.com/ngrodzitski/jacobi/tree/benchmark-feb2026).

## Hardware and Environment

**System details**:

```txt
$ uname -a
Linux jacobi.benchmark2 6.8.0-100-generic #100-Ubuntu SMP PREEMPT_DYNAMIC Tue Jan 13 16:40:06 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux

$ cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
performance

$ cat /proc/cpuinfo
//.. core 2 is used for running benchmark apps.
processor       : 2
vendor_id       : AuthenticAMD
cpu family      : 25
model           : 97
model name      : AMD Ryzen 9 7950X3D 16-Core Processor
stepping        : 2
microcode       : 0xa601209
cpu MHz         : 4989.492
cache size      : 1024 KB

$ cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq
5759000

$ lscpu | egrep 'Model name|L1d cache|L1i cache|L2 cache|L3 cache'
Model name:                              AMD Ryzen 9 7950X3D 16-Core Processor
L1d cache:                               512 KiB (16 instances)
L1i cache:                               512 KiB (16 instances)
L2 cache:                                16 MiB (16 instances)
L3 cache:                                128 MiB (2 instances)

# 512 KiB (16 instances) means 512/16=32 KiB per Core
# 16 MiB (16 instances) means 16/16=1 MiB per Core

$ cat /proc/meminfo | fgrep MemTotal
MemTotal:       97949508 kB     # 96GB
```

The benchmark suite was compiled using two industry-standard
**compilers**:

```txt
$ g++-14 --version
g++-14 (Ubuntu 14.2.0-4ubuntu2~24.04) 14.2.0

$ clang++-20 --version
Ubuntu clang version 20.1.2 (0ubuntu1~24.04.2)

# Using `-stdlib=libc++` for clang.
```

## Build Instructions

Both builds were executed in Release mode using
Conan for dependency management.
Crucially, the `-march=znver4` flag was passed via the Conan profile
to enable Zen 4 specific instruction sets:

```bash
# gcc build:
conan install -pr:a ./conan_profiles/ubu-gcc-14-znver4 -s:a build_type=Release --build missing -of _gcc14_release .
( source ./_gcc14_release/conanbuild.sh && cmake -B_gcc14_release . -DCMAKE_TOOLCHAIN_FILE=_gcc14_release/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release)
cmake --build _gcc14_release

# clang build:
conan install -pr:a ./conan_profiles/ubu-clang-20-znver4 -s:a build_type=Release --build missing -of _clang20_release .
( source ./_clang20_release/conanbuild.sh && cmake -B_clang20_release . -DCMAKE_TOOLCHAIN_FILE=_clang20_release/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release)
cmake --build _clang20_release

# It also makes sense to run tests:
ctest -T test --timeout 60 --output-on-failure --test-dir _gcc14_release
ctest -T test --timeout 60 --output-on-failure --test-dir _clang20_release
```

## Benchmark Dataset

Binary input files containing dense streams of book events
were prepared from historical market data.
To ensure comprehensive coverage, the datasets are split into two categories:

  * `_snapshots` contains single book events files;
  * `_snapshots_multi` contains events for multiple books.

## Benchmark Settings

The following settings files were used:

* [gcc14.settings.single.range.json](./settings/gcc14.settings.single.range.json)
* [gcc14.settings.multi.range.json](./settings/gcc14.settings.multi.range.json)
* [clang20.settings.single.range.json](./settings/clang20.settings.single.range.json)
* [clang20.settings.multi.range.json](./settings/clang20.settings.multi.range.json)

## Benchmark Raw Results

Note: The original results were post-processed to hide filenames
containing private tags.

* [gcc14 single range based benchmark](./raw_results/benchmark_gcc14.single.range.tar.bz2)
* [gcc14 multi range based benchmark](./raw_results/benchmark_gcc14.multi.range.tar.bz2)
* [clang20 single range based benchmark](./raw_results/benchmark_clang20.single.range.tar.bz2)
* [clang20 multi range based benchmark](./raw_results/benchmark_clang20.multi.range.tar.bz2)

<!--
BZIP2=-9 tar -cjf benchmark_gcc14.single.range.tar.bz2 benchmark_gcc14.single.range.json
BZIP2=-9 tar -cjf benchmark_gcc14.multi.range.tar.bz2 benchmark_gcc14.multi.range.json
BZIP2=-9 tar -cjf benchmark_clang20.single.range.tar.bz2 benchmark_clang20.single.range.json
BZIP2=-9 tar -cjf benchmark_clang20.multi.range.tar.bz2 benchmark_clang20.multi.range.json
 -->
## Benchmark Processed Results

Benchmark results were converted to csv and sqlite db files.

**CSV**:

```bash
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler gcc14 --json_path ./docs/feb2026/raw_results/benchmark_gcc14.single.range.json --output ./docs/feb2026/results/benchmark_gcc14.single.range.csv
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler gcc14 --json_path ./docs/feb2026/raw_results/benchmark_gcc14.multi.range.json --output ./docs/feb2026/results/benchmark_gcc14.multi.range.csv
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler clang20 --json_path ./docs/feb2026/raw_results/benchmark_clang20.single.range.json --output ./docs/feb2026/results/benchmark_clang20.single.range.csv
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler clang20 --json_path ./docs/feb2026/raw_results/benchmark_clang20.multi.range.json --output ./docs/feb2026/results/benchmark_clang20.multi.range.csv
```

[All csv tables](./results/feb2026_all_csv.tar.bz2)

<!-- BZIP2=-9 tar -cjf feb2026_all_csv.tar.bz2 *.csv -->

**SQLite**:

```bash
# Separate tables:
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler gcc14 --json_path ./docs/feb2026/raw_results/benchmark_gcc14.single.range.json --table gcc14_single --output ./docs/feb2026/results/benchmarks_all.db
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler gcc14 --json_path ./docs/feb2026/raw_results/benchmark_gcc14.multi.range.json --table gcc14_multi --output ./docs/feb2026/results/benchmarks_all.db
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler clang20 --json_path ./docs/feb2026/raw_results/benchmark_clang20.single.range.json --table clang20_single --output ./docs/feb2026/results/benchmarks_all.db
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler clang20 --json_path ./docs/feb2026/raw_results/benchmark_clang20.multi.range.json --table clang20_multi --output ./docs/feb2026/results/benchmarks_all.db

# Merged results for both compilers (multi/single are still separate):
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler gcc14 --json_path ./docs/feb2026/raw_results/benchmark_gcc14.single.range.json --table all_single --output ./docs/feb2026/results/benchmarks_all.db
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler gcc14 --json_path ./docs/feb2026/raw_results/benchmark_gcc14.multi.range.json --table all_multi --output ./docs/feb2026/results/benchmarks_all.db
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler clang20 --json_path ./docs/feb2026/raw_results/benchmark_clang20.single.range.json --table all_single --output ./docs/feb2026/results/benchmarks_all.db
python ./benchmark_experiment/results2db.py --commit 437dacf --compiler clang20 --json_path ./docs/feb2026/raw_results/benchmark_clang20.multi.range.json --table all_multi --output ./docs/feb2026/results/benchmarks_all.db
```

[SQLite db](./results/benchmarks_all_db.tar.bz2)
<!-- BZIP2=-9 tar -cjf benchmarks_all_db.tar.bz2 benchmarks_all.db -->

Contains the following tables:

* `gcc14_single` Measurements for gcc-14 single book benchmarks.
* `gcc14_multi` Measurements for gcc-14 multi book benchmarks.
* `clang20_single` Measurements for clang-20 single book benchmarks.
* `clang20_multi` Measurements for clang-20 multi book benchmarks.
* `all_single` Measurements for all compilers single book benchmarks.
* `all_multi` Measurements for all compilers multi book benchmarks.

## Results Analysis

To build a rank of implementations we use the following query:

```sql
-- Use one of the table-names defined above:
WITH measurements AS ( SELECT * FROM gcc14_single ),
-- Single books data set pXX params:
     pxx_params AS (SELECT 2 AS p99_offset, 29 AS p90_offset, 150 AS p50_offset)
-- Multi books data set pXX params:
     -- pxx_params AS (SELECT 0 AS p99_offset, 3 AS p90_offset, 16 AS p50_offset)

SELECT
    -- ==========================================
    -- Implementation:
    compiler,
    orders_table_storage,
    components,
    -- ==========================================

    avg(mps) as avg_mps,
    min(mps) as min_mps,
    max(mps) as max_mps,

    -- ==========================================
    -- PXX results:
    (SELECT mps
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  and mea2.components = main.components
                  and mea2.compiler = main.compiler
                ORDER BY mea2.mps
        LIMIT 1 OFFSET (SELECT p99_offset FROM pxx_params ) )
    as p99,

    (SELECT mps
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  and mea2.components = main.components
                  and mea2.compiler = main.compiler
                ORDER BY mea2.mps
        LIMIT 1 OFFSET (SELECT p90_offset FROM pxx_params ) )
    as p90,

    (SELECT mps
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  and mea2.components = main.components
                  and mea2.compiler = main.compiler
                ORDER BY mea2.mps
        LIMIT 1 OFFSET (SELECT p50_offset FROM pxx_params ) )
    as p50,
    -- ==========================================

    -- ==========================================
    -- Which files yield worst results for this implementation,
    -- use it for further research:
    (SELECT jacobi_data
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  and mea2.components = main.components
                  and mea2.compiler = main.compiler
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 0 ) as worst_0,

    (SELECT jacobi_data
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  and mea2.components = main.components
                  and mea2.compiler = main.compiler
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 1 ) as worst_1,

    (SELECT jacobi_data
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  and mea2.components = main.components
                  and mea2.compiler = main.compiler
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 2 ) as worst_2
    -- ==========================================

FROM measurements as main
GROUP BY orders_table_storage, components, compiler
ORDER BY
    -- Weighted importance of characteristics:
    avg_mps + p99 + p90 + p50
    -- Or alternatively:
    -- avg_mps
    -- p99
    -- p90
    -- p50
desc
```

### Rank of Order Book implementations

<!--
Tables were created with https://www.tablesgenerator.com/markdown_tables#
-->

**Single book handling gcc-14 implementations (TOP 20)**:

| **orders_table_storage** |    **components**    |  **avg_mps**  | **min_mps** | **max_mps** | **p99** | **p90** | **p50** |      **worst_0**     |      **worst_1**     |      **worst_2**      |
|--------------------------|:--------------------:|:-------------:|:-----------:|:-----------:|:-------:|:-------:|:-------:|:--------------------:|:--------------------:|:---------------------:|
| mixed_hot_cold_h1024     | `bsn1_plvl30_refIX3` | 44.2884       | 1.88874     | 55.1051     | 9.5846  | 37.9558 | 45.5046 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl30_refIX3` | 43.8449       | 2.11236     | 55.3187     | 3.03278 | 37.5838 | 45.2059 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl31_refIX3` | 43.8          | 1.84927     | 54.1279     | 9.38197 | 37.5309 | 44.8927 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| linear_v2                | `bsn1_plvl31_refIX3` | 43.7199       | 0.301706    | 59.5851     | 1.91619 | 38.0224 | 45.5224 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl31_refIX3` | 43.3018       | 2.04677     | 55.5127     | 2.88552 | 37.3653 | 44.5804 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| linear_v2                | `bsn1_plvl30_refIX3` | 43.1368       | 0.885178    | 56.1959     | 4.67282 | 36.774  | 44.554  | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl30_refIX3` | 42.7437       | 2.78716     | 55.0785     | 4.35123 | 35.3639 | 44.3506 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl31_refIX3` | 42.3088       | 2.6317      | 54.94       | 4.27141 | 35.6389 | 43.7411 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| mixed_hot_cold_h64       | `bsn1_plvl30_refIX3` | 41.4206       | 4.40493     | 55.6178     | 6.63583 | 32.8821 | 43.0492 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| linear_v1                | `bsn1_plvl31_refIX3` | 41.377        | 0.304808    | 53.4187     | 1.73483 | 33.8081 | 43.9138 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h64       | `bsn1_plvl31_refIX3` | 41.3375       | 4.24288     | 54.1262     | 6.62813 | 33.4906 | 42.7965 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| linear_v2                | `bsn1_plvl51_refIX3` | 41.0562       | 2.34565     | 50.4295     | 17.2688 | 31.8796 | 42.3675 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| linear_v2                | `bsn1_plvl12_refIX3` | 40.6218       | 0.875325    | 55.4466     | 5.03132 | 34.5131 | 41.1963 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl12_refIX3` | 40.5528       | 1.83727     | 52.8096     | 9.55078 | 35.376  | 41.168  | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| linear_v2                | `bsn1_plvl41_refIX3` | 40.3976       | 1.16372     | 53.2686     | 9.30627 | 27.692  | 42.5029 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| linear_v1                | `bsn1_plvl51_refIX3` | 40.3066       | 2.35328     | 50.2622     | 10.3583 | 30.0912 | 42.1778 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h256      | `bsn1_plvl12_refIX3` | 40.0871       | 2.06089     | 53.3618     | 2.97222 | 34.9382 | 40.7926 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| linear_v2                | `bsn1_plvl11_refIX3` | 39.9833       | 2.4581      | 51.4612     | 14.9472 | 35.6174 | 40.4675 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| linear_v2                | `bsn1_plvl42_refIX3` | 39.9209       | 0.611602    | 50.8122     | 7.74623 | 29.4512 | 41.537  | 273_148k.jacobi_data | 083_694k.jacobi_data | 065_125k.jacobi_data  |
| mixed_hot_cold_h32       | `bsn1_plvl31_refIX3` | 39.9022       | 6.92707     | 51.0068     | 10.2256 | 31.2759 | 41.2008 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |


**Single book handling clang-20 implementations (TOP 20)**:

| **orders_table_storage** |    **components**    |  **avg_mps**  | **min_mps** | **max_mps** | **p99** | **p90** | **p50** |      **worst_0**     |      **worst_1**     |      **worst_2**      |
|--------------------------|:--------------------:|:-------------:|:-----------:|:-----------:|:-------:|:-------:|:-------:|:--------------------:|:--------------------:|:---------------------:|
| linear_v2                | `bsn1_plvl11_refIX3` | 44.3905       | 3.08987     | 58.457      | 22.3295 | 38.4883 | 44.9381 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl11_refIX3` | 43.9256       | 5.56357     | 54.0438     | 21.6776 | 38.3558 | 44.6825 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| linear_v2                | `bsn1_plvl51_refIX3` | 45.1507       | 2.55143     | 54.4846     | 19.4928 | 36.7451 | 46.3802 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl51_refIX3` | 44.8569       | 5.05421     | 54.9282     | 18.6164 | 36.1017 | 46.0817 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl30_refIX3` | 46.8915       | 1.9287      | 58.5714     | 10.617  | 39.6212 | 47.8927 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl31_refIX3` | 46.1905       | 1.66626     | 55.1726     | 8.9965  | 41.2374 | 47.4413 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| linear_v2                | `bsn1_plvl30_refIX3` | 47.6105       | 0.883425    | 58.2893     | 4.83673 | 41.1537 | 49.1682 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| linear_v1                | `bsn1_plvl11_refIX3` | 43.9377       | 3.12486     | 56.701      | 13.8634 | 38.2099 | 44.6046 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| linear_v1                | `bsn1_plvl51_refIX3` | 45.3074       | 2.55738     | 55.4899     | 11.6867 | 35.9982 | 46.8858 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl11_refIX2` | 40.1429       | 5.55784     | 51.3512     | 21.8283 | 37.2365 | 40.3389 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| linear_v2                | `bsn1_plvl11_refIX2` | 40.2409       | 3.11203     | 53.1457     | 21.3126 | 37.3002 | 40.3616 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl12_refIX3` | 43.4391       | 2.69618     | 57.6641     | 13.1231 | 37.3391 | 44.0014 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h32       | `bsn1_plvl11_refIX3` | 40.1832       | 17.2861     | 51.1925     | 22.5541 | 33.3143 | 41.088  | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl21_refIX3` | 42.0505       | 3.40538     | 52.0521     | 16.6528 | 35.6143 | 42.7077 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl31_refIX2` | 43.7144       | 1.6942      | 61.0134     | 9.06797 | 39.5274 | 44.2204 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| linear_v2                | `bsn1_plvl51_refIX2` | 40.905        | 2.57378     | 50.596      | 17.8659 | 36.0765 | 41.5529 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h32       | `bsn1_plvl51_refIX3` | 41.4478       | 15.5725     | 51.5882     | 20.1293 | 31.965  | 42.7152 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl30_refIX3` | 46.1283       | 2.21391     | 57.852      | 3.17679 | 39.2035 | 47.5668 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| linear_v2                | `bsn1_plvl31_refIX3` | 45.818        | 0.310656    | 58.7952     | 2.08586 | 39.8115 | 48.1466 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl31_refIX3` | 45.5994       | 1.86344     | 54.5414     | 2.68721 | 39.8011 | 47.1447 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |

**Multiple book handling gcc-14 implementations (TOP 20)**:

| **orders_table_storage** |    **components**    |  **avg_mps**  | **min_mps** | **max_mps** | **p90** | **p50** |         **worst_0**        |         **worst_1**        |         **worst_2**        |
|--------------------------|:--------------------:|:-------------:|:-----------:|:-----------:|:-------:|:-------:|:--------------------------:|:--------------------------:|:--------------------------:|
| mixed_hot_cold_h256      | `bsn1_plvl30_refIX3` | 34.8305       | 17.4648     | 39.1389     | 32.5785 | 35.2175 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h128      | `bsn1_plvl30_refIX3` | 34.2601       | 19.2388     | 38.7183     | 31.4695 | 34.7119 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 020_5859k_b27.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl51_refIX3` | 33.4319       | 22.5239     | 37.8343     | 30.4062 | 33.3048 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl11_refIX3` | 32.3246       | 24.4027     | 35.2288     | 30.3729 | 32.5025 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h128      | `bsn1_plvl51_refIX3` | 33.1125       | 23.1016     | 37.6336     | 29.8193 | 33.0869 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl51_refIX3` | 33.3706       | 21.3732     | 37.6756     | 30.8579 | 33.4887 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl11_refIX3` | 31.9546       | 25.0079     | 34.9747     | 29.8042 | 32.0254 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 009_9582k_b34.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl31_refIX3` | 34.2458       | 17.6727     | 38.7484     | 31.9541 | 34.7198 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl30_refIX3` | 34.6825       | 15.8842     | 38.7608     | 32.5656 | 35.4017 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h64       | `bsn1_plvl30_refIX3` | 33.6092       | 20.6763     | 38.123      | 30.5151 | 33.6862 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl31_refIX3` | 33.7268       | 19.7801     | 38.0169     | 30.9798 | 33.9858 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 005_9443k_b38.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl11_refIX3` | 32.3343       | 23.1586     | 35.1593     | 30.5123 | 32.4591 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h64       | `bsn1_plvl51_refIX3` | 32.674        | 23.2747     | 37.1787     | 29.2151 | 32.5949 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  |
| mixed_hot_cold_h64       | `bsn1_plvl31_refIX3` | 33.1048       | 21.2774     | 37.4406     | 30.098  | 33.2704 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl31_refIX3` | 34.1146       | 15.9556     | 38.216      | 32.0533 | 34.7296 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h64       | `bsn1_plvl11_refIX3` | 31.3839       | 24.7721     | 34.4668     | 29.1565 | 31.4075 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h128      | `bsn1_plvl12_refIX3` | 32.8112       | 20.214      | 36.4114     | 30.3246 | 33.226  | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 020_5859k_b27.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl12_refIX3` | 33.2798       | 18.3924     | 36.8444     | 31.2093 | 33.6557 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |
| mixed_hot_cold_h64       | `bsn1_plvl12_refIX3` | 32.2106       | 21.4599     | 35.9702     | 29.252  | 32.3979 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl12_refIX3` | 33.2073       | 16.6109     | 36.8834     | 31.3483 | 34.0828 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data |

**Multiple book handling clang-20 implementations (TOP 20)**:

| **orders_table_storage** |    **components**    |  **avg_mps**  | **min_mps** | **max_mps** | **p90** | **p50** |         **worst_0**        |         **worst_1**        |         **worst_2**        |      **worst_2**      |
|--------------------------|:--------------------:|:-------------:|:-----------:|:-----------:|:-------:|:-------:|:--------------------------:|:--------------------------:|:--------------------------:|:---------------------:|
| mixed_hot_cold_h256      | `bsn1_plvl51_refIX3` | 37.5125       | 25.2986     | 42.0943     | 34.6049 | 37.42   | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl51_refIX3` | 37.5455       | 24.4102     | 41.8989     | 34.942  | 37.6129 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl11_refIX3` | 36.4122       | 27.1037     | 40.014      | 34.4293 | 36.216  | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl11_refIX3` | 36.5114       | 26.2557     | 39.9033     | 34.478  | 36.6814 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl51_refIX3` | 36.9219       | 25.947      | 41.8309     | 33.2855 | 36.8274 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 020_5859k_b27.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl11_refIX3` | 35.8646       | 27.6775     | 39.4842     | 33.6569 | 35.7045 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 020_5859k_b27.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h64       | `bsn1_plvl51_refIX3` | 36.187        | 25.6996     | 40.877      | 32.6276 | 35.9767 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 241_100k.jacobi_data  |
| mixed_hot_cold_h256      | `bsn1_plvl30_refIX3` | 37.6951       | 19.0124     | 42.5144     | 35.3103 | 37.9512 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data | 186_1944k.jacobi_data |
| mixed_hot_cold_h64       | `bsn1_plvl11_refIX3` | 35.0682       | 27.1062     | 38.7909     | 32.5907 | 34.8756 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 186_1944k.jacobi_data |
| mixed_hot_cold_h1024     | `bsn1_plvl30_refIX3` | 37.6146       | 17.6002     | 42.1854     | 35.3356 | 38.1704 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data | 296_353k.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl30_refIX3` | 36.8688       | 20.654      | 41.8316     | 33.8895 | 36.9676 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 020_5859k_b27.jacobi_data  | 186_1944k.jacobi_data |
| mixed_hot_cold_h256      | `bsn1_plvl52_refIX3` | 36.9209       | 20.0766     | 41.7779     | 33.732  | 37.2017 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl52_refIX3` | 36.9146       | 18.7805     | 41.5692     | 34.5369 | 37.2413 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h128      | `bsn1_plvl52_refIX3` | 36.1402       | 21.4752     | 41.2002     | 32.415  | 36.106  | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 020_5859k_b27.jacobi_data  | 285_166k.jacobi_data  |
| mixed_hot_cold_h64       | `bsn1_plvl30_refIX3` | 35.8968       | 21.3775     | 41.0549     | 32.3458 | 36.0247 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h64       | `bsn1_plvl52_refIX3` | 35.2731       | 22.0682     | 40.4705     | 31.4195 | 35.2188 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 296_353k.jacobi_data  |
| mixed_hot_cold_h32       | `bsn1_plvl51_refIX3` | 34.9294       | 23.7986     | 39.5401     | 30.4365 | 34.7335 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 186_1944k.jacobi_data |
| mixed_hot_cold_h256      | `bsn1_plvl12_refIX3` | 34.8503       | 20.9191     | 38.7959     | 32.4915 | 35.2412 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data | 285_166k.jacobi_data  |
| mixed_hot_cold_h32       | `bsn1_plvl11_refIX3` | 33.8054       | 24.8292     | 37.3119     | 30.9172 | 33.7307 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 019_8074k_b28.jacobi_data  | 285_166k.jacobi_data  |
| mixed_hot_cold_h1024     | `bsn1_plvl12_refIX3` | 34.8447       | 19.7437     | 38.6258     | 32.5715 | 35.4429 | 016_18100k_b33.jacobi_data | 017_10014k_b31.jacobi_data | 006_10198k_b38.jacobi_data | 241_100k.jacobi_data  |

**Single book handling overall (TOP 20)**:

| **compiler** | **orders_table_storage** |    **components**    |  **avg_mps**  | **min_mps** | **max_mps** | **p99** | **p90** | **p50** |      **worst_0**     |      **worst_1**     |      **worst_2**      |
|--------------|--------------------------|:--------------------:|:-------------:|:-----------:|:-----------:|:-------:|:-------:|:-------:|:--------------------:|:--------------------:|:---------------------:|
| clang20      | linear_v2                | `bsn1_plvl11_refIX3` | 44.3905       | 3.08987     | 58.457      | 22.3295 | 38.4883 | 44.9381 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl11_refIX3` | 43.9256       | 5.56357     | 54.0438     | 21.6776 | 38.3558 | 44.6825 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | linear_v2                | `bsn1_plvl51_refIX3` | 45.1507       | 2.55143     | 54.4846     | 19.4928 | 36.7451 | 46.3802 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl51_refIX3` | 44.8569       | 5.05421     | 54.9282     | 18.6164 | 36.1017 | 46.0817 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl30_refIX3` | 46.8915       | 1.9287      | 58.5714     | 10.617  | 39.6212 | 47.8927 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl31_refIX3` | 46.1905       | 1.66626     | 55.1726     | 8.9965  | 41.2374 | 47.4413 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | linear_v2                | `bsn1_plvl30_refIX3` | 47.6105       | 0.883425    | 58.2893     | 4.83673 | 41.1537 | 49.1682 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |
| clang20      | linear_v1                | `bsn1_plvl11_refIX3` | 43.9377       | 3.12486     | 56.701      | 13.8634 | 38.2099 | 44.6046 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| clang20      | linear_v1                | `bsn1_plvl51_refIX3` | 45.3074       | 2.55738     | 55.4899     | 11.6867 | 35.9982 | 46.8858 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl11_refIX2` | 40.1429       | 5.55784     | 51.3512     | 21.8283 | 37.2365 | 40.3389 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | linear_v2                | `bsn1_plvl11_refIX2` | 40.2409       | 3.11203     | 53.1457     | 21.3126 | 37.3002 | 40.3616 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl12_refIX3` | 43.4391       | 2.69618     | 57.6641     | 13.1231 | 37.3391 | 44.0014 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| gcc14        | mixed_hot_cold_h1024     | `bsn1_plvl30_refIX3` | 44.2884       | 1.88874     | 55.1051     | 9.5846  | 37.9558 | 45.5046 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | mixed_hot_cold_h32       | `bsn1_plvl11_refIX3` | 40.1832       | 17.2861     | 51.1925     | 22.5541 | 33.3143 | 41.088  | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl21_refIX3` | 42.0505       | 3.40538     | 52.0521     | 16.6528 | 35.6143 | 42.7077 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | mixed_hot_cold_h1024     | `bsn1_plvl31_refIX2` | 43.7144       | 1.6942      | 61.0134     | 9.06797 | 39.5274 | 44.2204 | 285_166k.jacobi_data | 174_880k.jacobi_data | 296_353k.jacobi_data  |
| clang20      | linear_v2                | `bsn1_plvl51_refIX2` | 40.905        | 2.57378     | 50.596      | 17.8659 | 36.0765 | 41.5529 | 273_148k.jacobi_data | 083_694k.jacobi_data | 186_1944k.jacobi_data |
| clang20      | mixed_hot_cold_h32       | `bsn1_plvl51_refIX3` | 41.4478       | 15.5725     | 51.5882     | 20.1293 | 31.965  | 42.7152 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| clang20      | mixed_hot_cold_h256      | `bsn1_plvl30_refIX3` | 46.1283       | 2.21391     | 57.852      | 3.17679 | 39.2035 | 47.5668 | 284_238k.jacobi_data | 259_300k.jacobi_data | 285_166k.jacobi_data  |
| clang20      | linear_v2                | `bsn1_plvl31_refIX3` | 45.818        | 0.310656    | 58.7952     | 2.08586 | 39.8115 | 48.1466 | 273_148k.jacobi_data | 083_694k.jacobi_data | 241_100k.jacobi_data  |

**Multiple book handling overall (TOP 20)**:

Repeats clang TOP 20, because none of the gcc implementations made it to the top.

### Visual Reports

**Gcc-14**:

  * [Single by datafile](./results/benchmark_gcc14.single_by_datafile.html)
  * [Single by implementation](./results/benchmark_gcc14.single_by_implementation.html)
  * [Multi by datafile](./results/benchmark_gcc14.multi_by_datafile.html)
  * [Multi by implementation](./results/benchmark_gcc14.multi_by_implementation.html)


**Cland-20**:

  * [Single by datafile](./results/benchmark_clang20.single_by_datafile.html)
  * [Single by implementation](./results/benchmark_clang20.single_by_implementation.html)
  * [Multi by datafile](./results/benchmark_clang20.multi_by_datafile.html)
  * [Multi by implementation](./results/benchmark_clang20.multi_by_implementation.html)


<!--
python ./benchmark_experiment/visualize_results_html_by_datafile.py -i ./docs/feb2026/raw_results/benchmark_gcc14.single.range.json -o ./docs/feb2026/results/benchmark_gcc14.single_by_datafile.html
python ./benchmark_experiment/visualize_results_html_by_implementation.py -i ./docs/feb2026/raw_results/benchmark_gcc14.single.range.json -o ./docs/feb2026/results/benchmark_gcc14.single_by_implementation.html

python ./benchmark_experiment/visualize_results_html_by_datafile.py -i ./docs/feb2026/raw_results/benchmark_clang20.single.range.json -o ./docs/feb2026/results/benchmark_clang20.single_by_datafile.html
python ./benchmark_experiment/visualize_results_html_by_implementation.py -i ./docs/feb2026/raw_results/benchmark_clang20.single.range.json -o ./docs/feb2026/results/benchmark_clang20.single_by_implementation.html

python ./benchmark_experiment/visualize_results_html_by_datafile.py -i ./docs/feb2026/raw_results/benchmark_gcc14.multi.range.json -o ./docs/feb2026/results/benchmark_gcc14.multi_by_datafile.html
python ./benchmark_experiment/visualize_results_html_by_implementation.py -i ./docs/feb2026/raw_results/benchmark_gcc14.multi.range.json -o ./docs/feb2026/results/benchmark_gcc14.multi_by_implementation.html

python ./benchmark_experiment/visualize_results_html_by_datafile.py -i ./docs/feb2026/raw_results/benchmark_clang20.multi.range.json -o ./docs/feb2026/results/benchmark_clang20.multi_by_datafile.html
python ./benchmark_experiment/visualize_results_html_by_implementation.py -i ./docs/feb2026/raw_results/benchmark_clang20.multi.range.json -o ./docs/feb2026/results/benchmark_clang20.multi_by_implementation.html
 -->

## Reflection on the Results

After aggregating benchmark results to SQLite database,
several Order Book implementations winners emerged,
along with a few pathological edge cases.

Here are key observations coming out of the benchmark results:

  * **The Index Backend**. `boost::unordered_flat_map` (refIX3)
    is the the best for the throughput.
    It occupied almost every slot in the Top 20 configurations,
    consistently beating both `tsl::robin_map` (`refIX2`) and Google's
    `absl::flat_hash_map` (`refIX4`).
    Even when weighting the results by different criteria,
    the Boost flat map dominates top list.

  * **Price Level Storage**.
    The highest performing variants were `plvl30/31`
    (Embedded index-based linked list with SOA) and plvl51/52 (Chunked SOA).
    That said, simple list implementations (plvl11/12) remained surprisingly competitive
    when compiled with Clang 20.

  * **Orders Table Storage**. Best performers:
    different kinds of `mixed_hot_cold` and `linear_v2/v1`

  * For single book handling all entries in the TOP20 performs very
    poor in some cases. Which is manifested with a `min_mps` and `p99` values
    that are 10-30x less performant than an average values.

### Edge Cases

Usually average numbers are not the goal when building trading systems.
The edge cases are important.

When analyzing the single-book runs, many implementations in TOP 20
exhibited severe performance degradation on specific datasets.
In these isolated cases, the minimum throughput (min_mps) and tail latency (P99)
dropped 10x to 30x below the median.

**Isolating the Pathologies**

To figure out exactly which cases (events data files) break
the "best" Order Book implementations,
I wrote the following query to isolate the runs where the top configurations
choked (dropping below 5 Million events/sec):

```sql
-- Use one of the table-names defined above:
WITH measurements AS (
    SELECT * FROM all_single
    -- Consider best orders table approaches:
    WHERE orders_table_storage in ( 'mixed_hot_cold_h1024', 'mixed_hot_cold_h256', 'mixed_hot_cold_h128',
                                    'mixed_hot_cold_h64', 'mixed_hot_cold_h32', 'linear_v2', 'linear_v1')

    -- Consider best price level approaches:
    AND ( components like '%plvl3%' OR components like '%plvl5%' OR components like '%plvl1%')

    -- Consider best order references index:
    AND components like '%refIX3'
)

-- Aggregate poor measurements by file
-- and calculate avg performance with those files
SELECT poor_runs.jacobi_data, ROUND(sum(mps)/count(1), 2) as avg_mps, count(1) as cnt,
       SUM( CASE WHEN poor_runs.orders_table_storage like 'linear%' THEN 1 ELSE 0 END ) AS linear_affected,
       SUM( CASE WHEN poor_runs.orders_table_storage like '%hot_cold%' THEN 1 ELSE 0 END ) AS hot_cold_affected
FROM
(
    -- Poor measurements for top implementation (having not more than 5 Mps)
    SELECT compiler, orders_table_storage,
           components, jacobi_data, sum(mps) as mps, count(1) as total_cnt
    FROM measurements
    GROUP BY compiler, orders_table_storage, components, jacobi_data
    HAVING mps < 5 ) poor_runs
GROUP BY poor_runs.jacobi_data
HAVING cnt >=5
ORDER BY avg_mps
```

|    **jacobi_data**    | **avg_mps** | **cnt** | **linear_affected** | **hot_cold_affected** |
|:---------------------:|:-----------:|:-------:|:-------------------:|:---------------------:|
| 273_148k.jacobi_data  | 1.37        | 24      | 24                  | 0                     |
| 083_694k.jacobi_data  | 2.59        | 16      | 16                  | 0                     |
| 284_238k.jacobi_data  | 3.03        | 24      | 2                   | 22                    |
| 285_166k.jacobi_data  | 3.05        | 24      | 2                   | 22                    |
| 259_300k.jacobi_data  | 3.28        | 24      | 2                   | 22                    |
| 241_100k.jacobi_data  | 3.37        | 8       | 8                   | 0                     |
| 186_1944k.jacobi_data | 3.83        | 9       | 9                   | 0                     |

Important takeaway:
Look closely at the last two columns.
The failures are almost perfectly mutually exclusive.

The "Linear Killers" books affect `linear_v1/2` approaches while
`mixed_hot_cold` survives unharmed, and vice versa.

<!--
Files were censored with reset_ids_util
../../../_build_release/bin/reset_ids_util -b 123456789 -o 273_148k.jacobi_data -i ORIGINAL_NAME.jacobi_data
 -->

You can verify these pathological datasets yourself. They are available in
[difficult_dataset_files.tar.bz2](./download/difficult_dataset_files.tar.bz2).

Here are a few sample commands to start with:

```bash
# LINEAR PATHOLOGY REPRODUCTION:
JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/273_148k.jacobi_data $bin_dir/_bench.throughput.linear_v1_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/083_694k.jacobi_data $bin_dir/_bench.throughput.linear_v1_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/273_148k.jacobi_data $bin_dir/_bench.throughput.linear_v2_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/083_694k.jacobi_data $bin_dir/_bench.throughput.linear_v2_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"

# HOT_COLD PATHOLOGY REPRODUCTION:
JACOBI_BENCHMARK_HOT_STORAGE_SIZE=32 JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/284_238k.jacobi_data $bin_dir/_bench.throughput.mixed_hot_cold_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
JACOBI_BENCHMARK_HOT_STORAGE_SIZE=128 JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/284_238k.jacobi_data $bin_dir/_bench.throughput.mixed_hot_cold_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
JACOBI_BENCHMARK_HOT_STORAGE_SIZE=256 JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/259_300k.jacobi_data $bin_dir/_bench.throughput.mixed_hot_cold_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
JACOBI_BENCHMARK_HOT_STORAGE_SIZE=1024 JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/285_166k.jacobi_data $bin_dir/_bench.throughput.mixed_hot_cold_levels_storage.single_book --benchmark_min_time=1s "--benchmark_filter=bsn1_plvl[135]._refIX3"
```

### Gather an Analise Perf-stats Telemetry

Let's dive into performance details in worst cases.

To understand why the memory layouts fail on specific files,
we will look at the hardware counters using `JACOBI_BENCHMARK_PROFILE_MODE=4`
(which wraps the execution in `perf stat -d ...`,
for details refer to [Benchmarking Suite](https://github.com/ngrodzitski/jacobi/blob/main/README.md#benchmarking-suite)).
We filter for a single implementation and run it for 8 seconds to gather stable telemetry.

**Note**: performance analysis given below uses a different (less performant) machine
          but results correlate.

### Linear Pathological Case

<!--
export data_dir=/home/ngrodzitski/github/ngrodzitski/jacobi-pub/docs/feb2026/download
export bin_dir=/home/ngrodzitski/github/ngrodzitski/jacobi-pub/_build_release/bin
 -->

Inspecting linear_v1 against a "Linear Killer" dataset (273_148k):

```bash
JACOBI_BENCHMARK_PROFILE_MODE=4 JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/273_148k.jacobi_data taskset -c 3 $bin_dir/_bench.throughput.linear_v1_levels_storage.single_book --benchmark_min_time=8s "--benchmark_filter=bsn1_plvl11_refIX3"
```

Perf output:

```txt
 Performance counter stats for process id '4059105':

         11,508.26 msec task-clock                #    1.000 CPUs utilized
               133      context-switches          #    0.012 K/sec
                 0      cpu-migrations            #    0.000 K/sec
         5,108,042      page-faults               #    0.444 M/sec
    45,457,074,862      cycles                    #    3.950 GHz                      (49.96%)
    51,568,102,608      instructions              #    1.13  insn per cycle           (62.47%)
     7,506,119,227      branches                  #  652.237 M/sec                    (62.47%)
        47,964,114      branch-misses             #    0.64% of all branches          (62.49%)
    12,741,124,932      L1-dcache-loads           # 1107.129 M/sec                    (62.53%)
       961,678,474      L1-dcache-load-misses     #    7.55% of all L1-dcache hits    (62.55%)
       518,958,071      LLC-loads                 #   45.094 M/sec                    (50.02%)
       262,844,348      LLC-load-misses           #   50.65% of all LL-cache hits     (49.98%)

      11.510932764 seconds time elapsed
```

This perf-stats clearly shows that memory access is the root cause of poor performance:

```txt
# Read "normally" as "Checking the metric on other events files and 10s runs"

# 1. Memory Thrashing
# Extreme number of page-faults
         5,108,042      page-faults               #    0.444 M/sec
# Normally this metric is at most 2-digit number. Here it is measured in millions.

# 2. The Cache Wall
# The other issue high LLC-load-misses,
       262,844,348      LLC-load-misses           #   50.65% of all LL-cache hits     (49.98%)
# Normally it is in range 12-25%:

# 2. Instruction Starvation
# And most likely cause by the above:
    51,568,102,608      instructions              #    1.13  insn per cycle           (62.47%)
# Normally it is 2.0+
```

**The Hypothesis**: The market data contains extreme price swings.
This forces the underlying linear storage (`std::vector<PLVL>`)
to constantly reallocate and move memory as the top/bottom bounds
of the order book expand beyond the current vector capacity.

### The Mixed Hot/Cold Pathological Case

Sample command to inspect perf-stats (hot storage size is 128 but other sizes are used too):

```bash
JACOBI_BENCHMARK_PROFILE_MODE=4 JACOBI_BENCHMARK_HOT_STORAGE_SIZE=128 JACOBI_BENCHMARK_EVENTS_FILE=$data_dir/284_238k.jacobi_data taskset -c 3 $bin_dir/_bench.throughput.mixed_hot_cold_levels_storage.single_book --benchmark_min_time=8s "--benchmark_filter=bsn1_plvl11_refIX3"
```

Perf output:
```
 Performance counter stats for process id '4035219':

         11,078.17 msec task-clock                #    1.000 CPUs utilized
               132      context-switches          #    0.012 K/sec
                 0      cpu-migrations            #    0.000 K/sec
                 0      page-faults               #    0.000 K/sec
    44,220,321,575      cycles                    #    3.992 GHz                      (49.99%)
   118,594,302,870      instructions              #    2.68  insn per cycle           (62.53%)
    22,798,601,282      branches                  # 2057.975 M/sec                    (62.53%)
        77,072,117      branch-misses             #    0.34% of all branches          (62.53%)
    41,850,245,460      L1-dcache-loads           # 3777.722 M/sec                    (62.53%)
        39,042,341      L1-dcache-load-misses     #    0.09% of all L1-dcache hits    (62.50%)
         2,097,293      LLC-loads                 #    0.189 M/sec                    (49.96%)
           426,777      LLC-load-misses           #   20.35% of all LL-cache hits     (49.96%)

      11.081143946 seconds time elapsed
```

This perf-stats actually looks solid: zero page faults,
excellent IPC (2.68), and normal cache misses.
The CPU is running with good efficiency,
but the throughput is still low.


**The Hypothesis**: The dataset contains a "flickering" outlier price level(s).
An isolated top-of-book order appears and vanishes rapidly,
far away from the dense cluster of the book.
Because the algorithm enforces that the Top Price must stay within
the Hot Storage window, this flickering order forces the algorithm to constantly
"slide" the hot storage window back and forth across empty price gaps.

  * When hot storage size - `h` is small (32), the slide is comparatively cheap.
  * When h is medium (256), the slide requires heavy bookkeeping, tanking performance.
  * When h is massive (1024 or 2048), the Hot Storage window is finally
    wide enough to encompass both the dense cluster and the outlier simultaneously,
    eliminating the need to slide at all or the ranges of price levels
    covered by window old and new position start overlap.

That is also backed up by measurements from benchmarks:

```sql
SELECT g.jacobi_data, g.orders_table_storage, g.components, g.mps as gcc_mps, c.mps as clang_mps
    FROM gcc14_single g
        INNER JOIN clang20_single c
            ON g.jacobi_data=c.jacobi_data
                AND g.orders_table_storage = c.orders_table_storage
                AND g.components = c.components
WHERE g.orders_table_storage like 'mixed_hot_cold%'
      AND g.components = 'bsn1_plvl11_refIX3'
      AND g.jacobi_data = '284_238k.jacobi_data'
ORDER BY g.id
```

|    **jacobi_data**   | **orders_table_storage** |   **components**   | **gcc_mps** | **clang_mps** |
|:--------------------:|--------------------------|:------------------:|:-----------:|:-------------:|
| 284_238k.jacobi_data | mixed_hot_cold_h32       | bsn1_plvl11_refIX3 | 16.5721     | 17.2861       |
| 284_238k.jacobi_data | mixed_hot_cold_h64       | bsn1_plvl11_refIX3 | 11.7903     | 11.8195       |
| 284_238k.jacobi_data | mixed_hot_cold_h128      | bsn1_plvl11_refIX3 | 7.97784     | 7.84732       |
| 284_238k.jacobi_data | mixed_hot_cold_h256      | bsn1_plvl11_refIX3 | 6.21393     | 6.10035       |
| 284_238k.jacobi_data | mixed_hot_cold_h1024     | bsn1_plvl11_refIX3 | 20.5684     | 38.0535       |

Similar results found for `259_300k.jacobi_data`,
while for `` performance decreases the higher hot storage size is.

|    **jacobi_data**   | **orders_table_storage** |   **components**   | **gcc_mps** | **clang_mps** |
|:--------------------:|--------------------------|:------------------:|:-----------:|:-------------:|
| 285_166k.jacobi_data | mixed_hot_cold_h32       | bsn1_plvl11_refIX3 | 21.0058     | 22.5541       |
| 285_166k.jacobi_data | mixed_hot_cold_h64       | bsn1_plvl11_refIX3 | 15.9255     | 16.3427       |
| 285_166k.jacobi_data | mixed_hot_cold_h128      | bsn1_plvl11_refIX3 | 11.5171     | 11.5117       |
| 285_166k.jacobi_data | mixed_hot_cold_h256      | bsn1_plvl11_refIX3 | 8.59196     | 8.46608       |
| 285_166k.jacobi_data | mixed_hot_cold_h1024     | bsn1_plvl11_refIX3 | 5.73371     | 5.56357       |

But manually checking `h=2048` it once again becomes satisfactory.
