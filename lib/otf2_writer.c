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

#include <mpi.h>

#include <otf2/otf2.h>

#if MPI_VERSION < 3
#define OTF2_MPI_UINT64_T MPI_UNSIGNED_LONG
#define OTF2_MPI_INT64_T  MPI_LONG
#endif
#include <otf2/OTF2_MPI_Collectives.h>

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

void writeIoEvent(OTF2_Archive *archive, OTF2_EvtWriter *evt_writer, int rank) {

    OTF2_DefWriter *loc_def_writer = OTF2_Archive_GetDefWriter(archive, rank);
    OTF2_Archive_CloseDefWriter(archive, loc_def_writer);

    OTF2_IoHandleRef ioHandle = 12;

    /* Some IO events */
    OTF2_IoAccessMode fmode = OTF2_IO_ACCESS_MODE_READ_WRITE;
    OTF2_IoCreationFlag fflag = OTF2_IO_CREATION_FLAG_CREATE;
    OTF2_IoStatusFlag fstatus = OTF2_IO_STATUS_FLAG_APPEND;

    OTF2_EvtWriter_IoCreateHandle(evt_writer, NULL, get_time(), ioHandle, fmode, fflag, fstatus);

    int i;
    for(i = 0; i< 1473145; i++) {
        OTF2_EvtWriter_IoOperationBegin(evt_writer, NULL, get_time(), ioHandle, fmode, fflag, 100, 0);
        OTF2_EvtWriter_IoOperationComplete(evt_writer, NULL, get_time(), ioHandle, 100, 0);
    }

    OTF2_EvtWriter_IoDestroyHandle(evt_writer, NULL, get_time(), ioHandle);
}


void writeGlobalDefinitions(OTF2_Archive *archive, int nranks) {
    OTF2_GlobalDefWriter* global_def_writer = OTF2_Archive_GetGlobalDefWriter( archive );

    OTF2_GlobalDefWriter_WriteRegion( global_def_writer, 0, 2, 3, 4, OTF2_REGION_ROLE_BARRIER,
                                        OTF2_PARADIGM_MPI, OTF2_REGION_FLAG_NONE, 7, 0, 0);

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

    OTF2_Archive_CloseGlobalDefWriter( archive, global_def_writer );
}

int main(int argc, char** argv )
{
    MPI_Init( &argc, &argv );
    int size;
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    OTF2_Archive* archive = OTF2_Archive_Open("ArchivePath", "ArchiveName",
                                               OTF2_FILEMODE_WRITE,
                                               1024 * 1024 /* event chunk size */,
                                               4 * 1024 * 1024 /* def chunk size */,
                                               OTF2_SUBSTRATE_POSIX,
                                               OTF2_COMPRESSION_ZLIB);

    // mandatory
    OTF2_Archive_SetFlushCallbacks( archive, &flush_callbacks, NULL );
    OTF2_MPI_Archive_SetCollectiveCallbacks(archive, MPI_COMM_WORLD, MPI_COMM_NULL);

    OTF2_Archive_OpenEvtFiles( archive );
    OTF2_EvtWriter* evt_writer = OTF2_Archive_GetEvtWriter(archive, rank );


    // Write Gobal Definitions
    if ( 0 == rank ) {
        writeGlobalDefinitions(archive, size);
    }

    // Write IO Event
    //writeIoEvent(archive, evt_writer, rank);

    OTF2_Archive_CloseEvtWriter( archive, evt_writer );
    OTF2_Archive_CloseEvtFiles( archive );


    MPI_Barrier( MPI_COMM_WORLD );
    OTF2_Archive_Close( archive );

    MPI_Finalize();

    return EXIT_SUCCESS;
}
