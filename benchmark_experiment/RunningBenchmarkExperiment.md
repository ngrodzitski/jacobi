# Running Benchmark Experiment

  * [Dataset Preparation](#dataset-preparation)
  * [Build the Benchmark Apps](#build-the-benchmark-apps)
  * [Experiment Settings (JSON)](#experiment-settings-json)
  * [Running the Experiment](#running-the-experiment)
  * [Post-Processing](#post-processing)
  * [Results Analysis](#results-analysis)
     * [Best results for each Orders Table approach for each file](#best-results-for-each-orders-table-approach-for-each-file)
     * [Rank Order Book implementations against a given dataset](#rank-order-book-implementations-against-a-given-dataset)
  * [Visualizing the Data (HTML Reports)](#visualizing-the-data-html-reports)

<!-- Created by https://github.com/ekalinin/github-markdown-toc -->

This document details how to execute a comprehensive benchmark suite
against the various **JACOBI** Order Book implementations.

A complete benchmark experiment consists of five distinct phases:

  * Dataset Preparation: Gather binary market data files.
  * Compilation: Build the JACOBI benchmark suite with optimal flags.
  * Configuration: Define the experiment parameters via JSON.
  * Execution: Run the automated benchmark script.
  * Analysis: Process the results into a SQLite database and
    generate HTML visualizations.

Before proceeding, ensure you understand the architecture of the
[Benchmarking Suite](../README.md#benchmarking-suite)

## Dataset Preparation

Each benchmark application requires binary input files
containing a stream of book events (Add, Reduce, Modify, Delete, Execute).
The format is strictly defined [here](../README.md#events-data-file).

For a statistically significant experiment,
you must provide a large dataset representing
the specific load profiles you want to measure
(e.g., highly liquid futures vs. sparse crypto pairs).

Requirements:
  * Place all event files into a single directory.
  * Ensure every file has the .jacobi_data extension.
  * Segregation: Ensure you separate files containing single-book events
    from multi-book events. Running a single_book benchmark binary
    against a multi-book data file will yield invalid results or fail.

## Build the Benchmark Apps

Follow the standard [Build Instructions](../README.md#build-instructions).

Critical Requirements for Benchmarking:

  * You must build in Release mode.
  * Ensure the CMake option JACOBI_BUILD_BENCHMARKS=ON is set (this is the default).
  * Hardware Matching: Ensure your Conan profile (or compiler flags)
    matches the target architecture (e.g., -march=native) to enable SIMD and
    most suitable optimizations.

## Experiment Settings (JSON)

The benchmark runner is driven by a JSON configuration file.
This file dictates which binaries to run, which data to feed them,
and which combinations of internal components to test.

Below is the annotated schema for the configuration file:

```jsonc
{
    // The directory containing the compiled JACOBI benchmark binaries.
    "bin_dir": "<BUILD_FOLDER_BIN_DIR>",

    // Path to folder containing your `.jacobi_data` files.
    "data_dir": "<DATA_FILES_PATH>",

    // OPTIONAL: A wrapper command for the execution environment.
    // Highly recommended: Pin the process to an isolated CPU core.
    "wrapper": "taskset -c 3",

    // Define the specific Orders Table binaries to benchmark.
    "binaries": [

        // FORMAT:
        {
            // The executable path (relative to bin_dir)
            "file": "<EXECUTABLE_FILE_NAME>",

            // The label used in the output reports
            "alias": "<ORDERS_TABLE_APPROACH_ALIAS>",

            // Event measurement scope:
            // "full"  : Measures every event in the file.
            // "range" : Skips the first 3% (warm-up) and last 2% (tail) of events.
            "type": "full|range" // OPTIONAL default "full"

            // OPTIONAL: Inject specific environment variables for this run.
            // Useful for tuning strategy-specific parameters.
            "env": { // OPTIONAL
                "ENV_VAR_NAME": "ENV_VAR_VALUE"
                // For example:
                // "JACOBI_BENCHMARK_HOT_STORAGE_SIZE": "256"
            }
        },
    ],

    // The permutation matrix.
    // The runner will test the Cartesian product of these components.
    // "--benchmark_filter={bsn}_{plvl}_{refIX}"
    "matrix" : {
        "bsn": [
            "bsn1",
            "bsn2"
        ],
        "plvl": [
            "plvl11","plvl12",
            "plvl21","plvl22",
            "plvl30","plvl31",
            "plvl41","plvl42",
            "plvl51","plvl52"
        ],

        "refIX": [
            "refIX1",
            "refIX2",
            "refIX3",
            "refIX4"
        ],
    },

    // OPTIONAL: Passthrough arguments directly to Google Benchmark.
    "extra_args": [
    "--benchmark_min_time=1s",
    ]
}
```

See `settings*.json` files [here](./) for examples of settings files.

## Running the Experiment

The actual execution is orchestrated by [run_experiment.py](./run_experiment.py).
This script handles the nested loops of iterating over data files,
binaries, and the configuration matrix.

Usage:

```txt
$ python ./benchmark_experiment/run_experiment.py --help

usage: run_experiment.py [-h] [-s SETTINGS] [-o OUTPUT]

Run Google Benchmark suite using custom data files.

options:
  -h, --help            show this help message and exit
  -s, --settings SETTINGS
                        Path to the configuration JSON file (default: settings.json)
  -o, --output OUTPUT   Path where the result JSON will be saved (default: benchmark_results.json)
```

When running the orchestrator, you may also pin the Python process
to a separate core from your benchmark worker
(if you defined a taskset wrapper in your JSON settings)
to prevent context-switching interference.

For example:
```bash
taskset -c 10 python3 ./benchmark_experiment/run_experiment.py \
    -s benchmark_experiment/feb2026_znver4/clang20.settings.single.range.json \
    -o benchmark_experiment/feb2026_znver4/results/benchmark_clang20.single.range.json
```

Execution Model & Resilience

You need to understand how the orchestrator runs your code:

  * Process Isolation: Each measurement
    (a specific binary running a specific matrix combination against a specific data file)
    is executed as a completely separate OS process.
    This guarantees zero cache pollution or memory fragmentation
    bleed-over between test runs.

  * Iterative Flushing: After benchmarking all implementations
    against a single data file, the script flushes the intermediate
    results to the output JSON.
    If your machine crashes or OOMs halfway through a 10-hour run,
    you do not lose all your data.

## Post-Processing

When the orchestrator finishes, it dumps the raw metrics into a JSON file.
The schema groups the results by
`Data File -> Storage Strategy -> Component Matrix`:

```jsonc
{
    // ...
    "results": {
        "<EVENTS_DATA_FILE>": {
            "<ORDERS_TABLE_APPROACH_ALIAS>": {
                "measurements": {
                    // "bsn1_plvl11_refIX1" is the component matrix
                    // "value_Mps" = Million events Per Second
                    "bsn1_plvl11_refIX1": { "value_Mps": 100, /*...*/ },
                    "bsn1_plvl11_refIX2": { "value_Mps": 100, /*...*/ },
                    // ...
                    "bsn1_plvl52_refIX3": { "value_Mps": 100, /*...*/ },
                    "bsn1_plvl52_refIX4": { "value_Mps": 100, /*...*/ },
                }
            }
            // ...
        }
        // ...
    }
}
```

**Converting to a Database (SQLite / CSV)**

grep-ing a 30MB JSON files to compare performance regressions
across compiler versions is quite a challenge.
We need to normalize this data so we can write SQL queries against it.

Use the [results2db.py](./results2db.py) script to flatten the JSON
into a SQLite database or a CSV file.

```text
$ python ./benchmark_experiment/results2db.py --help

usage: results2db.py [-h] --commit COMMIT --compiler COMPILER --json_path JSON_PATH --output OUTPUT [--table TABLE]

options:
  -h, --help            show this help message and exit
  --commit COMMIT
  --compiler COMPILER
  --json_path JSON_PATH
  --output OUTPUT       Filename ending in .csv or .db/.sqlite
  --table TABLE         Table name for SQLite output (default: measurements)
```

**The Database Schema**

The script flattens the nested JSON into a clean,
queryable table with the following columns:

  * _git_commit_ Auxiliary tag linking the row to a specific repo commit.
  * _compiler_ Auxiliary tag identifying the compiler and version used.
  * _jacobi_data_ The name of the specific market events file used for the load.
  * _orders_table_storage_ The high-level storage strategy strategy
    (e.g., `linear_v1`, `mixed_hot_cold_h64`).
  * _components_ The specific internal permutation tested (e.g., `bsn1_plvl51_refIX4`).
  * _mps_ Million events Per Second (throughput).

## Results Analysis

Having a sqlite db you can run various queries to get the clues
on how well implementations handle a given dataset.

Here are few examples.

### Best results for each Orders Table approach for each file

```sql
SELECT
    jacobi_data,
    orders_table_storage,
    (SELECT components
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage
                  AND mea2.jacobi_data = main.jacobi_data
                ORDER BY mea2.mps DESC
     LIMIT 1) as components,
    max(mps) as max_mps
from measurements as main
GROUP BY jacobi_data, orders_table_storage
ORDER BY jacobi_data, orders_table_storage
```

### Rank Order Book implementations against a given dataset

Here is a sample SQL that shows each implementation characteristics.

**Note**: For calculating of `PXX` values the offset (a position in sorted set)
is hard-coded for case of 302 events data files.

```sql
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
            WHERE mea2.orders_table_storage = main.orders_table_storage and mea2.components = main.components
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 2 )
    as p99,

    (SELECT mps
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage and mea2.components = main.components
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 29 )
    as p90,

    (SELECT mps
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage and mea2.components = main.components
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 150 )
    as p50,
    -- ==========================================

    -- ==========================================
    -- Which files yield worst results for this implementation,
    -- use it for further research:
    (SELECT jacobi_data
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage and mea2.components = main.components
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 0 ) as worst_0,

    (SELECT jacobi_data
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage and mea2.components = main.components
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 1 ) as worst_1,

    (SELECT jacobi_data
        FROM measurements as mea2
            WHERE mea2.orders_table_storage = main.orders_table_storage and mea2.components = main.components
                ORDER BY mea2.mps
        LIMIT 1 OFFSET 2 ) as worst_2
    -- ==========================================

FROM measurements as main
GROUP BY orders_table_storage, components
ORDER BY
    -- Weighted importance of characteristics:
    -- avg_mps + p99 + p90 + p50
    -- Or alternatively:
    avg_mps
    -- p99
    -- p90
    -- p50
desc
```

## Visualizing the Data (HTML Reports)

Raw numbers in a SQLite database are great for automated queries,
but to actually spot performance cliffs and latency spikes, you need graphs.

**JACOBI** provides two scripts to generate static HTML reports for manual analysis.

  * The "Data-Centric" View ([visualize_results_html_by_datafile.py](./visualize_results_html_by_datafile.py))

    Use this when you want to see how every implementation handled
    a specific market condition (a single data file).

  * The "Implementation-Centric" View ([visualize_results_html_by_implementation.py](./visualize_results_html_by_implementation.py))

    Use this when you want to see how a single implementation
    performs across all datasets.
