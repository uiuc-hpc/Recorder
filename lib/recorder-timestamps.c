#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"
#include "mpio.h"
#include "zlib.h"
#include "recorder.h"

#define CHUNK 16384

void ts_get_filename(RecorderLogger *logger, char* ts_filename) {
    sprintf(ts_filename, "%s/%d.ts", logger->traces_dir, logger->rank);
}

void ts_write_out_zlib(RecorderLogger* logger) {
    int ret;
    unsigned have;
    z_stream strm;

    const size_t buf_size = sizeof(uint32_t) * logger->ts_index;
    unsigned char out[buf_size];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    // ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        RECORDER_LOGERR("[Recorder] fatal error: can't initialize zlib.");
        return;
    }

    strm.avail_in = buf_size;
    strm.next_in  = (unsigned char*) logger->ts;
    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
        strm.avail_out = buf_size;
        strm.next_out = out;
        ret = deflate(&strm, Z_FINISH);    /* no bad return value */
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        have = buf_size - strm.avail_out;
        if (GOTCHA_REAL_CALL(fwrite)(out, 1, have, logger->ts_file) != have) {
            RECORDER_LOGERR("[Recorder] fatal error: zlib write out error.");
            (void)deflateEnd(&strm);
            return;
        }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);         /* all input will be used */

    /* clean up and return */
    (void)deflateEnd(&strm);
}

void ts_write_out(RecorderLogger* logger) {
    if (logger->ts_compression) {
        ts_write_out_zlib(logger);
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

    int offset_type_size;
    MPI_Type_size(MPI_OFFSET, &offset_type_size);

    MPI_Offset file_size = 0, offset = 0;
    void* in;
    GOTCHA_REAL_CALL(fseek)(logger->ts_file, 0, SEEK_END);
    file_size = (MPI_Offset) GOTCHA_REAL_CALL(ftell)(logger->ts_file);
    in = malloc(file_size);
    GOTCHA_REAL_CALL(fread)(in, 1, file_size, logger->ts_file);


    char merged_ts_filename[1024];
    sprintf(merged_ts_filename, "%s/recorder.ts", logger->traces_dir);

    MPI_File fh;
    GOTCHA_REAL_CALL(MPI_File_open)(MPI_COMM_WORLD, merged_ts_filename, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
    // first write out the compressed file size of each rank
    GOTCHA_REAL_CALL(MPI_File_write_at_all)(fh, logger->rank*offset_type_size, &file_size, 1, MPI_OFFSET, MPI_STATUS_IGNORE);
    // then write the acutal content of each rank
    // we don't intercept MPI_Exscan
    MPI_Exscan(&file_size, &offset, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);
    offset += (logger->nprocs * offset_type_size);
    GOTCHA_REAL_CALL(MPI_File_write_at_all)(fh, offset, in, file_size, MPI_BYTE, MPI_STATUS_IGNORE);
    GOTCHA_REAL_CALL(MPI_File_sync)(fh);
    GOTCHA_REAL_CALL(MPI_File_close)(&fh);
    free(in);
}
