/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2012 by The HDF Group
 * All rights reserved.
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

#include "mpi.h"
#include "recorder.h"

void recorder_init(int *argc, char ***argv) {
    int rank, nprocs;
    MAP_OR_FAIL(PMPI_Comm_size)
    MAP_OR_FAIL(PMPI_Comm_rank)
    RECORDER_REAL_CALL(PMPI_Comm_size)(MPI_COMM_WORLD, &nprocs);
    RECORDER_REAL_CALL(PMPI_Comm_rank)(MPI_COMM_WORLD, &rank);

    depth = 0;
    logger_init(rank, nprocs);
}

void recorder_exit() {
    logger_exit();
}

int PMPI_Init(int *argc, char ***argv) {
    MAP_OR_FAIL(PMPI_Init)
    int ret = RECORDER_REAL_CALL(PMPI_Init) (argc, argv);
    recorder_init(argc, argv);
    return ret;
}

int MPI_Init(int *argc, char ***argv) {
    MAP_OR_FAIL(PMPI_Init)
    int ret = RECORDER_REAL_CALL(PMPI_Init) (argc, argv);
    recorder_init(argc, argv);
    return ret;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
    MAP_OR_FAIL(PMPI_Init_thread)
    int ret = RECORDER_REAL_CALL(PMPI_Init_thread) (argc, argv, required, provided);
    recorder_init(argc, argv);
    return ret;
}

int PMPI_Finalize(void) {
    recorder_exit();
    MAP_OR_FAIL(PMPI_Finalize)
    int ret = RECORDER_REAL_CALL(PMPI_Finalize) ();
    return ret;
}

int MPI_Finalize(void) {
    recorder_exit();
    MAP_OR_FAIL(PMPI_Finalize)
    int ret = RECORDER_REAL_CALL(PMPI_Finalize) ();
    return ret;
}
