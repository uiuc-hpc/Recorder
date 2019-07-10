#include <otf2/otf2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "recorder.h"

static OTF2_FlushType pre_flush(void* userData, OTF2_FileType fileType,
        OTF2_LocationRef location, void* callerData, bool final ) {
    return OTF2_FLUSH;
}

static OTF2_TimeStamp post_flush( void* userData, OTF2_FileType fileType, OTF2_LocationRef location ) {
    double t = MPI_Wtime() * 1e9;
    return ( uint64_t ) t;
}

static OTF2_FlushCallbacks flush_callbacks = {
    .otf2_pre_flush  = pre_flush,
    .otf2_post_flush = post_flush
};


// Record read/write operations
static void record_io_event(OTF2_EvtWriter *evt_writer, OTF2_IoHandleRef ioHandle, const char *func, size_t bytes, double tstart, double tend) {
    static int matchingId;
    OTF2_IoOperationMode opmode = OTF2_IO_OPERATION_MODE_WRITE;
    if ( strstr(func, "read") )
        opmode = OTF2_IO_OPERATION_MODE_READ;
    else if ( strstr(func, "sync") || strstr(func, "flush") )
        opmode = OTF2_IO_OPERATION_MODE_FLUSH;
    OTF2_IoCreationFlag fflag = OTF2_IO_CREATION_FLAG_CREATE;
    OTF2_EvtWriter_IoOperationBegin(evt_writer, NULL, tstart, ioHandle, opmode, fflag, bytes, matchingId);
    OTF2_EvtWriter_IoOperationComplete(evt_writer, NULL, tend, ioHandle, bytes, matchingId++);
}

// Record file open operations
static void record_open_event(OTF2_EvtWriter *evt_writer, int fileId, double tstart){
    OTF2_IoHandleRef ioHandle = fileId;
    OTF2_IoAccessMode fmode = OTF2_IO_ACCESS_MODE_READ_WRITE;
    OTF2_IoCreationFlag fflag = OTF2_IO_CREATION_FLAG_CREATE;
    OTF2_IoStatusFlag fstatus = OTF2_IO_STATUS_FLAG_APPEND;
    OTF2_EvtWriter_IoCreateHandle(evt_writer, NULL, tstart, ioHandle, fmode, fflag, fstatus);
}

// Record file close operations
static void record_close_event(OTF2_EvtWriter *evt_writer, int fileId, double tend) {
    OTF2_IoHandleRef ioHandle = fileId;
    OTF2_EvtWriter_IoDestroyHandle(evt_writer, NULL, tend, ioHandle);
}

// Record file seek operations
static  void record_seek_event(OTF2_EvtWriter *evt_writer, int fileId, int64_t offset, double time) {
    OTF2_IoHandleRef ioHandle = fileId;
    OTF2_IoSeekOption whence = OTF2_IO_SEEK_FROM_START;
    uint64_t offsetResult = offset;
    OTF2_EvtWriter_IoSeek(evt_writer, NULL, time, ioHandle, offset, whence, offset);
}



static void convert_metadata_operations(OTF2_Archive *archive, int rank, const char* base_dir) {
    OTF2_DefWriter *local_def_writer = OTF2_Archive_GetDefWriter(archive, rank);
    char path[256];
    sprintf(path, "%s/%d.mt", base_dir, rank);
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error opening file metadata log file");
        return;
    }
    char buffer[256];
    char *filename, *fileId;
    while(fgets(buffer, 256, fp) != NULL ) {

        filename = strtok(buffer, " ");
        fileId = strtok(NULL, " ");

        // Remove the ending "\n"
        size_t s = strlen(fileId);
        if (s && (fileId[s-1] == '\n')) fileId[--s] = 0;
        printf("META[%d]: %s %s\n", rank, filename, fileId);

        // Write rank.def
        OTF2_DefWriter_WriteString(local_def_writer, atoi(fileId), filename);
    }
    fclose(fp);
    OTF2_Archive_CloseDefWriter(archive, local_def_writer);
}

static void convert_data_operations(OTF2_Archive *archive, int rank, const char* base_dir) {
    OTF2_EvtWriter* evt_writer = OTF2_Archive_GetEvtWriter(archive, rank);


    char path[256];
    sprintf(path, "%s/%d.itf", base_dir, rank);
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        perror("Error opening file data log file");
        return;
    }

    size_t size = sizeof(IoOperation_t);
    IoOperation_t *op = (IoOperation_t *) malloc(size);
    while ( fread(op, size, 1, fp) == 1) { // Read one IoOperation at a time
        const char* func = get_function_name_by_id(op->func_id);
        //printf("DATA[%d]: %s %u %f %f\n", rank, func, op->filename_id, op->start_time, op->end_time);

        if ( op->func_id > 22 )     // Not POSIX IO Calls
            continue;

        if ( strstr(func, "open") ) {
            record_open_event(evt_writer, op->filename_id, op->start_time);
        } else if ( strstr(func, "close") ) {
            record_close_event(evt_writer, op->filename_id, op->end_time);
        } else if ( strstr(func, "seek" ) ) {
            record_seek_event(evt_writer, op->filename_id, op->attr1, op->end_time);
        } else {    // read, write, flush
            record_io_event(evt_writer, op->filename_id, func, 100, op->start_time, op->end_time);
        }
    }
    free(op);
    fclose(fp);

    OTF2_Archive_CloseEvtWriter( archive, evt_writer );
}

int main(int argc, char** argv) {

    OTF2_Archive* archive = OTF2_Archive_Open("logs", "logs", OTF2_FILEMODE_WRITE,
                                               1024 * 1024 /* event chunk size */,
                                               4 * 1024 * 1024 /* def chunk size */,
                                               OTF2_SUBSTRATE_POSIX, OTF2_COMPRESSION_NONE );

    OTF2_Archive_SetFlushCallbacks(archive, &flush_callbacks, NULL);
    OTF2_Archive_SetSerialCollectiveCallbacks( archive );


    OTF2_GlobalDefWriter* global_def_writer = OTF2_Archive_GetGlobalDefWriter( archive );
    OTF2_GlobalDefWriter_WriteString( global_def_writer, 0, "" );
    OTF2_GlobalDefWriter_WriteString( global_def_writer, 1, "Main Thread" );

    /*
     * LocationGroup maps to rank
     * Location maps to thread
     */
    int nprocs = 4;
    for (int r = 0; r < nprocs; r++) {
        OTF2_GlobalDefWriter_WriteString( global_def_writer, 2+r, "Rank " );    // 0 is "", 1 is "Main Thread"
        OTF2_GlobalDefWriter_WriteLocationGroup( global_def_writer,
                                                r /* id */,
                                                2+r /* name */,
                                                OTF2_LOCATION_GROUP_TYPE_PROCESS,
                                                OTF2_UNDEFINED_SYSTEM_TREE_NODE/* system tree */ );
        OTF2_GlobalDefWriter_WriteLocation( global_def_writer,
                                            r /* id */,
                                            1 /* name */,
                                            OTF2_LOCATION_TYPE_CPU_THREAD,
                                            2 /* # events */,
                                            0 /* location group */ );

        convert_metadata_operations(archive, r, argv[1]);
        convert_data_operations(archive, r, argv[1]);
    }

    OTF2_Archive_Close(archive);

    return EXIT_SUCCESS;
}
