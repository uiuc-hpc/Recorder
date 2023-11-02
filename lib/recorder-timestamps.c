#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"
#include "recorder.h"

#define CHUNK 16384

void ts_get_filename(RecorderLogger *logger, char* ts_filename) {
    sprintf(ts_filename, "%s/%d.ts", logger->traces_dir, logger->rank);
}

void ts_write_out(RecorderLogger* logger) {
    if (logger->ts_compression) {
        size_t buf_size = logger->ts_index * sizeof(uint32_t);
        recorder_write_zlib((unsigned char*)logger->ts, buf_size, logger->ts_file);
    } else {
        GOTCHA_REAL_CALL(fwrite)(logger->ts, logger->ts_index, sizeof(uint32_t), logger->ts_file);
    }
}

void ts_merge_files(RecorderLogger* logger) {
    GOTCHA_SET_REAL_CALL(fread, RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(fseek, RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(ftell, RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(MPI_File_open, RECORDER_MPI_TRACING);
    GOTCHA_SET_REAL_CALL(MPI_File_write_at_all, RECORDER_MPI_TRACING);
    GOTCHA_SET_REAL_CALL(MPI_File_close, RECORDER_MPI_TRACING);
    GOTCHA_SET_REAL_CALL(MPI_File_sync, RECORDER_MPI_TRACING);

    MPI_Offset file_size = 0, offset = 0;
    void* in;
    GOTCHA_REAL_CALL(fseek)(logger->ts_file, 0, SEEK_END);
    file_size = (MPI_Offset) GOTCHA_REAL_CALL(ftell)(logger->ts_file);
    in = malloc((size_t)file_size);
    GOTCHA_REAL_CALL(fseek)(logger->ts_file, 0, SEEK_SET);
    GOTCHA_REAL_CALL(fread)(in, 1, (size_t)file_size, logger->ts_file);
    //size_t* tmp = (size_t*) in;
    //printf("CHEN file_size: %ld %ld %ld\n", (size_t)file_size, tmp[0], tmp[1]);

    char merged_ts_filename[1024];
    sprintf(merged_ts_filename, "%s/recorder.ts", logger->traces_dir);

    MPI_File fh;
    GOTCHA_REAL_CALL(MPI_File_open)(MPI_COMM_WORLD, merged_ts_filename, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
    // first write out the compressed file size of each rank
    size_t file_size_t = (size_t) file_size;
    GOTCHA_REAL_CALL(MPI_File_write_at_all)(fh, logger->rank*sizeof(size_t), &file_size_t, sizeof(size_t), MPI_BYTE, MPI_STATUS_IGNORE);
    // then write the acutal content of each rank
    // we don't intercept MPI_Exscan
    MPI_Exscan(&file_size, &offset, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);
    offset += (logger->nprocs * sizeof(size_t));
    GOTCHA_REAL_CALL(MPI_File_write_at_all)(fh, offset, in, file_size, MPI_BYTE, MPI_STATUS_IGNORE);
    GOTCHA_REAL_CALL(MPI_File_sync)(fh);
    GOTCHA_REAL_CALL(MPI_File_close)(&fh);
    free(in);
}
