/*
 * This file is part of the Score-P software (http://www.score-p.org)
 *
 * Copyright (c) 2009-2013,
 * RWTH Aachen University, Germany
 *
 * Copyright (c) 2009-2013,
 * Gesellschaft fuer numerische Simulation mbH Braunschweig, Germany
 *
 * Copyright (c) 2009-2014,
 * Technische Universitaet Dresden, Germany
 *
 * Copyright (c) 2009-2013,
 * University of Oregon, Eugene, USA
 *
 * Copyright (c) 2009-2013,
 * Forschungszentrum Juelich GmbH, Germany
 *
 * Copyright (c) 2009-2013,
 * German Research School for Simulation Sciences GmbH, Juelich/Aachen, Germany
 *
 * Copyright (c) 2009-2013,
 * Technische Universitaet Muenchen, Germany
 *
 * This software may be modified and distributed under the terms of
 * a BSD-style license.  See the COPYING file in the package base
 * directory for details.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <mpi.h>

#include <otf2/otf2.h>

#if MPI_VERSION < 3
#define OTF2_MPI_UINT64_T MPI_UNSIGNED_LONG
#define OTF2_MPI_INT64_T  MPI_LONG
#endif
#include <otf2/OTF2_MPI_Collectives.h>

#include "recorder.h"

static OTF2_TimeStamp get_time(void) {
    double t = MPI_Wtime() * 1e9;
    return ( uint64_t )t;
}

static OTF2_FlushType pre_flush(void* userData, OTF2_FileType fileType, OTF2_LocationRef location, void* callerData, bool final) {
    return OTF2_FLUSH;
}

static OTF2_TimeStamp post_flush(void* userData, OTF2_FileType fileType, OTF2_LocationRef location) {
    return get_time();
}

static OTF2_FlushCallbacks flush_callbacks =
{
    .otf2_pre_flush  = pre_flush,
    .otf2_post_flush = post_flush
};


/* Make them global so we don't pass them around */
static OTF2_Archive *__archive;
static OTF2_EvtWriter *__evt_writer;
static int __rank;

static void write_open_close(const char *func) {
    printf("here %d\n", __rank);
    OTF2_IoHandleRef ioHandle = 12 + __rank;
    OTF2_IoAccessMode fmode = OTF2_IO_ACCESS_MODE_READ_WRITE;
    OTF2_IoCreationFlag fflag = OTF2_IO_CREATION_FLAG_CREATE;
    OTF2_IoStatusFlag fstatus = OTF2_IO_STATUS_FLAG_APPEND;
    if (strstr(func, "open") !=NULL)
        OTF2_EvtWriter_IoCreateHandle(__evt_writer, NULL, get_time(), ioHandle, fmode, fflag, fstatus);
    else
        OTF2_EvtWriter_IoDestroyHandle(__evt_writer, NULL, get_time(), ioHandle);
}
static void write_read_write(const char *func, size_t bytes) {
    OTF2_IoHandleRef ioHandle = 12 + __rank;
    OTF2_IoOperationMode opmode = OTF2_IO_OPERATION_MODE_WRITE;
    OTF2_IoCreationFlag fflag = OTF2_IO_CREATION_FLAG_CREATE;
    OTF2_EvtWriter_IoOperationBegin(__evt_writer, NULL, get_time(), ioHandle, opmode, fflag, bytes, 0);
    OTF2_EvtWriter_IoOperationComplete(__evt_writer, NULL, get_time(), ioHandle, bytes, 0);
}
static void write_lseek() {
}

void otf2_write_trace(const char *func, size_t bytes){

    if (strstr(func, "write") != NULL || strstr(func, "read") != NULL) {
        //write_read_write(func, bytes);
    } else if (strstr(func, "seek") != NULL ) {
        //write_lseek();
    } else if (strstr(func, "open") != NULL || strstr(func, "close") != NULL ) {
        write_open_close(func);
    }

    //OTF2_DefWriter *loc_def_writer = OTF2_Archive_GetDefWriter(__archive, __rank);
    //OTF2_Archive_CloseDefWriter(__archive, loc_def_writer);
}



void writeGlobalDefinitions(int nranks) {
    OTF2_GlobalDefWriter* global_def_writer = OTF2_Archive_GetGlobalDefWriter(__archive);

    OTF2_GlobalDefWriter_WriteSystemTreeNode( global_def_writer, 0, 5, 6, OTF2_UNDEFINED_SYSTEM_TREE_NODE);

    int r;
    for (r = 0; r < nranks ; r++ ) {
        char process_name[ 32 ];
        sprintf( process_name, "MPI Rank %d", r );
        OTF2_GlobalDefWriter_WriteString( global_def_writer, 100 + r, process_name );

        OTF2_GlobalDefWriter_WriteLocationGroup( global_def_writer,
                                                    r /* id */,
                                                    100 + r /* name */,
                                                    OTF2_LOCATION_GROUP_TYPE_PROCESS,
                                                    0 /* system tree */ );

        OTF2_GlobalDefWriter_WriteLocation( global_def_writer,
                                            r /* id */,
                                            100 + r /* name, reference the above defined process_name */,
                                            OTF2_LOCATION_TYPE_CPU_THREAD,
                                            4 /* # events */,
                                            r /* location group */ );
    }

    OTF2_StringRef filenameRef = 10;
    OTF2_GlobalDefWriter_WriteString(global_def_writer, filenameRef, "ThisIsALongFileName");
    OTF2_IoFileRef fileRef = 11;
    OTF2_GlobalDefWriter_WriteIoRegularFile(global_def_writer, fileRef, filenameRef, 0);
    OTF2_IoHandleRef ioHandle = 12;
    OTF2_GlobalDefWriter_WriteIoHandle(global_def_writer, ioHandle, filenameRef, fileRef, 0, 0, 0, 0);

    OTF2_Archive_CloseGlobalDefWriter(__archive, global_def_writer);
}

/*
 * in: nprocs and rank
 * out: archive and event writer
 */
void otf2_init(int nprocs, int rank) {
    __rank = rank;
    __archive = OTF2_Archive_Open("ArchivePath", "ArchiveName",
                                               OTF2_FILEMODE_WRITE,
                                               1024 * 1024 /* event chunk size */,
                                               4 * 1024 * 1024 /* def chunk size */,
                                               OTF2_SUBSTRATE_POSIX,
                                               OTF2_COMPRESSION_NONE);
    // Mandatory
    OTF2_Archive_SetFlushCallbacks(__archive, &flush_callbacks, NULL );
    OTF2_MPI_Archive_SetCollectiveCallbacks(__archive, MPI_COMM_WORLD, MPI_COMM_NULL);
    //OTF2_Archive_SetSerialCollectiveCallbacks(__archive);

    // Write Gobal Definitions
    if ( 0 == rank ) {
        writeGlobalDefinitions(nprocs);
    }

    // Open event file and return Event writer
    OTF2_Archive_OpenEvtFiles(__archive);
    __evt_writer = OTF2_Archive_GetEvtWriter(__archive, rank);
}

void otf2_exit() {
    OTF2_Archive_CloseEvtWriter(__archive, __evt_writer);
    OTF2_Archive_CloseEvtFiles(__archive);
    MPI_Barrier( MPI_COMM_WORLD);
    OTF2_Archive_Close(__archive);
}
