#include "profiling.hpp"

#include <unistd.h>
#include <sys/wait.h>

#include <chrono>
#include <csignal>
#include <string>

#include <fmt/format.h>

namespace jacobi::bench
{

//
// start_perf_profiling()
//

pid_t start_perf_profiling( int mode )
{
    if( 0 == mode )
    {
        // Ignore profiling.
        return 0;
    }

    pid_t profiled_app_pid = getpid();

    // Clone the process.
    pid_t pid = fork();

    if( pid == -1 )
    {
        perror( "fork failed" );
        return -1;
    }

    if( pid == 0 )
    {
        std::string pid_str = std::to_string( profiled_app_pid );

        if( 1 == mode )
        {
            // Simply start perf record
            // execlp replaces this process with 'perf'.
            execlp( "perf",
                    "perf",
                    "record",
                    "-g",
                    "-p",
                    pid_str.c_str(),
                    (char *)nullptr );
        }
        else if( 2 == mode )
        {
            const auto duration =
                std::chrono::system_clock::now().time_since_epoch();
            const auto ms =
                std::chrono::duration_cast< std::chrono::milliseconds >( duration )
                    .count();
            const auto output_filename =
                fmt::format( "perf-{}_{:03}.data", ms / 1000, ms % 1000 );

            // Start perf record with specific filename as output
            // execlp replaces this process with 'perf'.
            execlp( "perf",
                    "perf",
                    "record",
                    "-g",
                    "-o",
                    output_filename.c_str(),
                    "-p",
                    pid_str.c_str(),
                    (char *)nullptr );
        }
        else if( 3 == mode )
        {
            // Start perf record with specific filename as output
            // execlp replaces this process with 'perf'.
            execlp( "perf",
                    "perf",
                    "stat",
                    "--append",
                    "-o",
                    "perf-stat.txt",
                    "-p",
                    pid_str.c_str(),
                    (char *)nullptr );
        }
        else if( 4 == mode )
        {
            // Start perf record with specific filename as output
            // execlp replaces this process with 'perf'.
            execlp( "perf",
                    "perf",
                    "stat",
                    "--append",
                    "-d",
                    "-o",
                    "perf-stat.txt",
                    "-p",
                    pid_str.c_str(),
                    (char *)nullptr );
        }
        else if( 5 == mode )
        {
            // Start perf record with specific filename as output
            // execlp replaces this process with 'perf'.
            execlp( "perf",
                    "perf",
                    "stat",
                    "--append",
                    "-d",
                    "-d",
                    "-o",
                    "perf-stat.txt",
                    "-p",
                    pid_str.c_str(),
                    (char *)nullptr );
        }

        // If we get here, execlp failed.
        perror( "execlp failed" );
        _exit( 127 );
    }

    // --- PARENT PROCESS ---
    return pid;
}

//
// stop_perf_profiling()
//

void stop_perf_profiling( pid_t perf_pid )
{
    if( perf_pid > 0 )
    {
        // SIGINT tells perf to close files and write symbols.
        kill( perf_pid, SIGINT );

        // Reap the zombie to prevent process table leaks.
        int status;
        waitpid( perf_pid, &status, 0 );
    }
}

}  // namespace jacobi::bench
