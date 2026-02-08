#pragma once

#include <sys/types.h>

namespace jacobi::bench
{

//
// start_perf_profiling()
//

/**
 * @brief Starts 'perf' in a sidecar process to record the current process.
 */
pid_t start_perf_profiling( int mode );

/**
 * @brief Stops the profiler cleanly.
 */
void stop_perf_profiling( pid_t perf_pid );

}  // namespace jacobi::bench
