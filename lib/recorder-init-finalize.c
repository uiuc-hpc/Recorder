#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE /* for RTLD_NEXT */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <dlfcn.h>
#include <signal.h>
#include <execinfo.h>

#include "mpi.h"
#include "recorder.h"
#include "recorder-gotcha.h"


static double local_tstart, local_tend;
static int rank, nprocs;


void signal_handler(int sig);

/**
 * First we will intercept the GNU constructor,
 * where we perform recorder_init().
 *
 * If this is an MPI program, then later we will intercept
 * one of MPI_Init* call, where we update the mpi info
 * using update_mpi_info(). Only in that function, we actually
 * create the log directory.
 *
 * If this is not an MPI program, then we create the log
 * directory at the first flush time in recorder-logger.c
 */
void recorder_init() {

    // avoid double init;
    if (logger_initialized()) return;

    /*
    signal(SIGSEGV, signal_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    */

    gotcha_init();
    logger_init();
    utils_init();

    local_tstart = recorder_wtime();
}

void update_mpi_info() {
    recorder_init();

    GOTCHA_SET_REAL_CALL(MPI_Comm_size, RECORDER_MPI_TRACING);
    GOTCHA_SET_REAL_CALL(MPI_Comm_rank, RECORDER_MPI_TRACING);

    int mpi_initialized = 0;
    MPI_Initialized(&mpi_initialized);  // we do not intercept MPI_Initialized() call.

    if(mpi_initialized) {
        GOTCHA_REAL_CALL(MPI_Comm_rank)(MPI_COMM_WORLD, &rank);
        GOTCHA_REAL_CALL(MPI_Comm_size)(MPI_COMM_WORLD, &nprocs);
    }

    logger_set_mpi_info(rank, nprocs);
}

void recorder_finalize() {

    // check if already finialized
    if (!logger_initialized()) return;

    logger_finalize();
    utils_finalize();

    local_tend = recorder_wtime();

    if (rank == 0) {
        RECORDER_LOGINFO("[Recorder] elapsed time: %.2f\n", local_tend-local_tstart);
    }
}

int MPI_Init(int *argc, char ***argv) {
    int ret = PMPI_Init(argc, argv);
    update_mpi_info();
    return ret;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
    int ret = PMPI_Init_thread(argc, argv, required, provided);
    update_mpi_info();
    return ret;
}

int MPI_Finalize(void) {
    recorder_finalize();
    return PMPI_Finalize();
}



#ifdef __GNUC__

/**
 * Handle non mpi programs
 */
void __attribute__((constructor)) no_mpi_init() {
    char* with_non_mpi = getenv(RECORDER_WITH_NON_MPI);
    if(with_non_mpi)
        recorder_init();
}

void __attribute__((destructor))  no_mpi_finalize() {
    recorder_finalize();
}

#endif


void signal_handler(int sig) {
    /*
     * print backtrace for debug
    void *array[20];
    size_t size;
    size = backtrace(array, 20);
    RECORDER_LOGERR("Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDOUT_FILENO);
    exit(1);
    */

    if(rank == 0)
        printf("[Recorder] signal [%s] captured, finalize now.\n", strsignal(sig));
    recorder_finalize();
}
