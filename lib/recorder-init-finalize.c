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
    RECORDER_LOGDBG("[Recorder] recorder initialized.\n");
}

void update_mpi_info() {

    int mpi_initialized = 0;
    PMPI_Initialized(&mpi_initialized);  // we do not intercept MPI_Initialized() call.

    rank   = 0;
    nprocs = 1;
    if(mpi_initialized) {
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        PMPI_Comm_size(MPI_COMM_WORLD, &nprocs);
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
    RECORDER_LOGDBG("[Recorder] MPI_Init\n");
    int ret = PMPI_Init(argc, argv);
    recorder_init();
    update_mpi_info();
    return ret;
}

void mpi_init_(MPI_Fint* ierr) {
    RECORDER_LOGDBG("[Recorder] mpi_init_\n");
    int argc = 0;
    char** argv = NULL;
    int ret = PMPI_Init(&argc, &argv);
    recorder_init();
    update_mpi_info();
    *ierr = (MPI_Fint)ret;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
    RECORDER_LOGDBG("[Recorder] MPI_Init_thread\n");
    int ret = PMPI_Init_thread(argc, argv, required, provided);
    recorder_init();
    update_mpi_info();
    return ret;
}

void mpi_init_thread_(MPI_Fint* required, MPI_Fint* provided, MPI_Fint* ierr) {
    RECORDER_LOGDBG("[Recorder] mpi_init_thread_\n");
    int argc = 0;
    char** argv = NULL;
    int ret = PMPI_Init_thread(&argc, &argv, *((int*)required), provided);
    recorder_init();
    update_mpi_info();
    *ierr = (MPI_Fint)ret;
}

int MPI_Finalize(void) {
    RECORDER_LOGDBG("[Recorder] MPI_Finalize\n");
    recorder_finalize();
    return PMPI_Finalize();
}

void mpi_finalize_(MPI_Fint* ierr) {
    RECORDER_LOGDBG("[Recorder] mpi_finalize_\n");
    recorder_finalize();
    *ierr = 0;
}



#ifdef __GNUC__

/**
 * Handle non mpi programs
 */
void __attribute__((constructor)) no_mpi_init() {
    char* with_non_mpi = getenv(RECORDER_WITH_NON_MPI);
    if(with_non_mpi && atoi(with_non_mpi) == 1) {
        recorder_init();
    }
}

void __attribute__((destructor))  no_mpi_finalize() {
    char* with_non_mpi = getenv(RECORDER_WITH_NON_MPI);
    if(with_non_mpi && atoi(with_non_mpi) == 1) {
        recorder_finalize();
    }
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
