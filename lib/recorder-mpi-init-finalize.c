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

#include "mpi.h"
#include "recorder.h"
#include "recorder-dynamic.h"


char *comm2name(MPI_Comm comm) {
    char *tmp = (char *)malloc(1024);
    int len;
    PMPI_Comm_get_name(comm, tmp, &len);
    tmp[len] = 0;
    if(len == 0) strcpy(tmp, "MPI_COMM_UNKNOWN");
    return tmp;
}

char *type2name(MPI_Datatype type) {
    char *tmp = (char *)malloc(1024);
    int len;
    PMPI_Type_get_name(type, tmp, &len);
    tmp[len] = 0;
    return tmp;
}

char *makename(MPI_Datatype *type) {
    char *tmp = (char *)malloc(1024);
    sprintf(tmp, "RECORDER_TYPE_%p", type);
    return tmp;
}

/*
void overload_mpi_symbols(void) {
     * These function are not intercepted but are used
     * by recorder itself.
    MAP_OR_FAIL(PMPI_Wtime);
    MAP_OR_FAIL(PMPI_Allreduce);
    MAP_OR_FAIL(PMPI_Bcast);
    MAP_OR_FAIL(PMPI_Comm_rank);
    MAP_OR_FAIL(PMPI_Comm_size);
    MAP_OR_FAIL(PMPI_Scan);
    MAP_OR_FAIL(PMPI_Type_commit);
    MAP_OR_FAIL(PMPI_Type_contiguous);
    MAP_OR_FAIL(PMPI_Type_extent);
    MAP_OR_FAIL(PMPI_Type_free);
    MAP_OR_FAIL(PMPI_Type_size);
    MAP_OR_FAIL(PMPI_Type_hindexed);
    MAP_OR_FAIL(PMPI_Op_create);
    MAP_OR_FAIL(PMPI_Op_free);
    MAP_OR_FAIL(PMPI_Reduce);
    MAP_OR_FAIL(PMPI_Type_get_envelope);

    return;
}
*/

extern char *__progname;
void recorder_mpi_initialize(int *argc, char ***argv) {
    int nprocs;
    int rank;

    RECORDER_MPI_CALL(PMPI_Comm_size)(MPI_COMM_WORLD, &nprocs);
    RECORDER_MPI_CALL(PMPI_Comm_rank)(MPI_COMM_WORLD, &rank);

    char *logfile_name;
    char *metafile_name;
    char *logdir_name;
    logfile_name = malloc(PATH_MAX);
    logdir_name = malloc(PATH_MAX);
    metafile_name = malloc(PATH_MAX);
    char cuser[L_cuserid] = {0};
    cuserid(cuser);

    sprintf(logdir_name, "%s_%s", cuser, __progname);
    int status;
    status = mkdir(logdir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // write enabled i/o layers to meta file
    if(rank == 0){
        FILE *__recordermeta;
        sprintf(metafile_name, "%s/recorder.meta", logdir_name);
        __recordermeta = fopen(metafile_name, "w");
        printf(" metafile_name %s\n", metafile_name);

        fprintf(__recordermeta, "enabled_layers:");
        #ifndef DISABLE_HDF5_TRACE
        fprintf(__recordermeta, " HDF5");
        #endif
        #ifndef DISABLE_MPIO_TRACE
        fprintf(__recordermeta, " MPI");
        #endif
        #ifndef DISABLE_POSIX_TRACE
        fprintf(__recordermeta, " POSIX");
        #endif
        fprintf(__recordermeta, "\n");
        fprintf(__recordermeta, "workload_start_time:");
        fprintf(__recordermeta, " %.5f\n",recorder_wtime());
        fclose(__recordermeta);
    }

    sprintf(logfile_name, "%s/log.%d", logdir_name, rank);
    __recorderfh = fopen(logfile_name, "w");
    depth = 0;

    printf(" logfile_name %s\n", logfile_name);

    free(logfile_name);
    free(logdir_name);

    fn2id_map = hashmap_new();

    return;
}

void recorder_shutdown(int timing_flag) {
    // Any of the following will cause a crash, so wierd
    //if (__recorderfh != NULL)
    //    fclose(__recorderfh);
    //hashmap_free(fn2id_map);
    return;
}

int PMPI_Init(int *argc, char ***argv) {
    #ifdef RECORDER_PRELOAD
    //overload_mpi_symbols();
    #endif

    int ret = RECORDER_MPI_CALL(PMPI_Init)(argc, argv);
    if (ret != MPI_SUCCESS)
        return (ret);

    recorder_mpi_initialize(argc, argv);
    return (ret);
}

int MPI_Init(int *argc, char ***argv) {
    #ifdef RECORDER_PRELOAD
    //overload_mpi_symbols();
    #endif

    int ret = RECORDER_MPI_CALL(PMPI_Init)(argc, argv);
    if (ret != MPI_SUCCESS)
        return (ret);

    recorder_mpi_initialize(argc, argv);
    return (ret);
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
    #ifdef RECORDER_PRELOAD
    //overload_mpi_symbols();
    #endif

    int ret = RECORDER_MPI_CALL(PMPI_Init_thread)(argc, argv, required, provided);
    if (ret != MPI_SUCCESS)
        return (ret);

    recorder_mpi_initialize(argc, argv);
    return (ret);
}

int MPI_Finalize(void) {
    /*
    if(getenv("RECORDER_INTERNAL_TIMING"))
       recorder_shutdown(1);
    else
       recorder_shutdown(0);
    */
    recorder_shutdown(0);
    int ret = RECORDER_MPI_CALL(PMPI_Finalize)();
    return (ret);
}

