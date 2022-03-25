/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 * 4. All publications or advertising materials mentioning features or use of
 *    this software are asked, but not required, to acknowledge that it was
 *    developed by The HDF Group and by the National Center for Supercomputing
 *    Applications at the University of Illinois at Urbana-Champaign and
 *    credit the contributors.
 *
 * 5. Neither the name of The HDF Group, the name of the University, nor the
 *    name of any Contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission from
 *    The HDF Group, the University, or the Contributor, respectively.
 *
 * DISCLAIMER:
 * THIS SOFTWARE IS PROVIDED BY THE HDF GROUP AND THE CONTRIBUTORS
 * "AS IS" WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED. In no
 * event shall The HDF Group or the Contributors be liable for any damages
 * suffered by the users arising out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * Portions of Recorder were developed with support from the Lawrence Berkeley
 * National Laboratory (LBNL) and the United States Department of Energy under
 * Prime Contract No. DE-AC02-05CH11231.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _XOPEN_SOURCE 500
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


static double local_tstart, local_tend;
static int rank, nprocs;


void signal_handler(int sig);

/**
 * First we will intercept the GNU constructor,
 * where we perform recorder_init().
 *
 * If this is an MPI program, then later we will intercept
 * one of MPI_Init* call, where we update the mpi info
 * using update_mpi_inf(). Only in that function, we actually
 * create the log directory.
 *
 * If this is not an MPI program, then we create the log
 * directory at the first flush time in recorder-logger.c
 */

void recorder_init() {

    // avoid double init;
    if (logger_initialized()) return;

    signal(SIGSEGV, signal_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    logger_init(rank, nprocs);
    utils_init();

    local_tstart = recorder_wtime();
}

void update_mpi_info() {
    recorder_init();

    MAP_OR_FAIL(PMPI_Comm_size);
    MAP_OR_FAIL(PMPI_Comm_rank);

    int mpi_initialized = 0;
    PMPI_Initialized(&mpi_initialized);  // we do not intercept MPI_Initialized() call.

    if(mpi_initialized) {
        RECORDER_REAL_CALL(PMPI_Comm_rank)(MPI_COMM_WORLD, &rank);
        RECORDER_REAL_CALL(PMPI_Comm_size)(MPI_COMM_WORLD, &nprocs);
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
        fprintf(stderr, "[Recorder] elapsed time on rank 0: %.2f\n", local_tend-local_tstart);
    }
}

int PMPI_Init(int *argc, char ***argv) {
    MAP_OR_FAIL(PMPI_Init);
    int ret = RECORDER_REAL_CALL(PMPI_Init) (argc, argv);
    update_mpi_info();
    return ret;
}

int MPI_Init(int *argc, char ***argv) {
    MAP_OR_FAIL(PMPI_Init);
    int ret = RECORDER_REAL_CALL(PMPI_Init) (argc, argv);
    update_mpi_info();
    return ret;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
    MAP_OR_FAIL(PMPI_Init_thread)
    int ret = RECORDER_REAL_CALL(PMPI_Init_thread) (argc, argv, required, provided);
    update_mpi_info();
    return ret;
}

int PMPI_Finalize(void) {
    recorder_finalize();
    MAP_OR_FAIL(PMPI_Finalize);
    return RECORDER_REAL_CALL(PMPI_Finalize) ();
}

int MPI_Finalize(void) {
    recorder_finalize();
    MAP_OR_FAIL(PMPI_Finalize);
    return RECORDER_REAL_CALL(PMPI_Finalize) ();
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
    fprintf(stdout, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDOUT_FILENO);
    exit(1);
    */

    if(rank == 0)
        printf("[Recorder] signal [%s] captured, finalize now.\n", strsignal(sig));
    recorder_finalize();



}
