#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include "recorder.h"
#include "zlib.h"



/* Global file handler (per rank) for the local trace log file */
FILE *__datafh;
FILE *__metafh;
/* Starting timestamp of each rank */
double START_TIMESTAMP;
double TIME_RESOLUTION = 0.000001;
/* Filename to integer map */
hashmap_map *__filename2id_map;

/* A sliding window for peephole compression */
#define RECORD_WINDOW_SIZE 3
Record __record_window[RECORD_WINDOW_SIZE];

/* For zlib compression */
#define ZLIB_BUF_SIZE 4096
z_stream __zlib_stream;

/* compression mode */
CompressionMode __compression_mode = COMP_ZLIB;


static inline int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}


static inline void write_record_args(FILE* f, int arg_count, char** args) {
    char invalid_str[] = "???";
    for(int i = 0; i < arg_count; i++) {
        fprintf(f, " ");
        if(args[i]) {
            //if (!startsWith("0x", args[i]))
            RECORDER_REAL_CALL(fwrite) (args[i], strlen(args[i]), 1, f);
        } else
            RECORDER_REAL_CALL(fwrite) (invalid_str, strlen(invalid_str), 1, f);
    }
    fprintf(f, "\n");
}

static inline void write_compressed_record(FILE* f, char ref_window_id, Record diff_record) {
    char status = '0';
    int tstart = (diff_record.tstart - START_TIMESTAMP) / TIME_RESOLUTION;
    int tend   = (diff_record.tend - START_TIMESTAMP) / TIME_RESOLUTION;
    RECORDER_REAL_CALL(fwrite) (&status, sizeof(char), 1, f);
    RECORDER_REAL_CALL(fwrite) (&tstart, sizeof(int), 1, f);
    RECORDER_REAL_CALL(fwrite) (&tend, sizeof(int), 1, f);
    RECORDER_REAL_CALL(fwrite) (&ref_window_id, sizeof(char), 1, f);
    write_record_args(f, diff_record.arg_count, diff_record.args);
}

static inline void write_uncompressed_record(FILE *f, Record record) {
    char status = '0';
    int tstart = (record.tstart - START_TIMESTAMP) / TIME_RESOLUTION;
    int tend   = (record.tend - START_TIMESTAMP) / TIME_RESOLUTION;
    RECORDER_REAL_CALL(fwrite) (&status, sizeof(char), 1, f);
    RECORDER_REAL_CALL(fwrite) (&tstart, sizeof(int), 1, f);
    RECORDER_REAL_CALL(fwrite) (&tend, sizeof(int), 1, f);
    RECORDER_REAL_CALL(fwrite) (&(record.func_id), sizeof(unsigned char), 1, f);
    write_record_args(f, record.arg_count, record.args);
}

/* Write record in plan text format */
static inline void write_record_in_text(FILE *f, Record record) {
    fprintf(f, "%f %f %s", record.tstart, record.tend, get_function_name_by_id(record.func_id));
    write_record_args(f, record.arg_count, record.args);
}

/* Compress the text with zlib and write it out */
static inline void write_record_in_zlib(FILE *f, Record record) {
    static char in_buf[ZLIB_BUF_SIZE];
    static char out_buf[ZLIB_BUF_SIZE];
    sprintf(in_buf, "%f %f %s", record.tstart, record.tend, get_function_name_by_id(record.func_id));
    for(int i = 0; i < record.arg_count; i++) {
        strcat(in_buf, " ");
        if(record.args[i])
            strcat(in_buf, record.args[i]);
        else                    // some null argument ?
            strcat(in_buf, "???");
    }
    strcat(in_buf, "\n");

    __zlib_stream.avail_in = strlen(in_buf);
    __zlib_stream.next_in = in_buf;
    do {
        __zlib_stream.avail_out = ZLIB_BUF_SIZE;
        __zlib_stream.next_out = out_buf;
        int ret = deflate(&__zlib_stream, Z_NO_FLUSH);    /* no bad return value */
        unsigned have = ZLIB_BUF_SIZE - __zlib_stream.avail_out;
        RECORDER_REAL_CALL(fwrite) (out_buf, 1, have, f);
    } while (__zlib_stream.avail_out == 0);
}

void zlib_init() {
    /* allocate deflate state */
    __zlib_stream.zalloc = Z_NULL;
    __zlib_stream.zfree = Z_NULL;
    __zlib_stream.opaque = Z_NULL;
    deflateInit(&__zlib_stream, Z_DEFAULT_COMPRESSION);
}
void zlib_exit() {
    // Write out everythin zlib's buffer
    char out_buf[ZLIB_BUF_SIZE];
    do {

        __zlib_stream.avail_out = ZLIB_BUF_SIZE;
        __zlib_stream.next_out = out_buf;
        int ret = deflate(&__zlib_stream, Z_FINISH);    /* no bad return value */
        unsigned have = ZLIB_BUF_SIZE - __zlib_stream.avail_out;
        RECORDER_REAL_CALL(fwrite) (out_buf, 1, have, __datafh);
    } while (__zlib_stream.avail_out == 0);
    // Clean up and end the Zlib
    (void)deflateEnd(&__zlib_stream);
}


static inline Record get_diff_record(Record old_record, Record new_record) {
    Record diff_record;
    diff_record.arg_count = 999;

    // Same function should normally have the same number of arguments
    if (old_record.arg_count != new_record.arg_count)
        return diff_record;

    // Get the number of different arguments
    int count = 0;
    for(int i = 0; i < old_record.arg_count; i++)
        if(strcmp(old_record.args[i], new_record.args[i]) !=0)
            count++;

    // Record.args store only the different arguments
    diff_record.arg_count = count;
    int idx = 0;
    diff_record.args = malloc(sizeof(char *) * count);
    for(int i = 0; i < old_record.arg_count; i++)
        if(strcmp(old_record.args[i], new_record.args[i]) !=0)
            diff_record.args[idx++] = new_record.args[i];
    return diff_record;
}


void write_record(Record new_record) {
    if (__datafh == NULL) return;   // have not initialized yet

    if (__compression_mode == COMP_TEXT) {
        write_record_in_text(__datafh, new_record);
        return;
    }

    if (__compression_mode == COMP_ZLIB) {
        write_record_in_zlib(__datafh, new_record);
        return;
    }

    if (__compression_mode == COMP_BINARY) {        // baseline encoding, uncompressed binary foramt
        write_uncompressed_record(__datafh, new_record);
        return;
    }

    // The rest code is for peephole compression mode
    int compress = 0;
    Record diff_record;
    int min_diff_count = 999;
    char ref_window_id = -1;
    for(int i = 0; i < RECORD_WINDOW_SIZE; i++) {
        Record record = __record_window[i];
        if ((record.func_id == new_record.func_id) && (new_record.arg_count < 8) && (record.arg_count > 0)) {
            Record tmp_record = get_diff_record(record, new_record);
            if(tmp_record.arg_count < 8 && tmp_record.arg_count < min_diff_count) {
                min_diff_count = tmp_record.arg_count;
                ref_window_id = i;
                compress = 1;
                diff_record = tmp_record;
            }
        }
    }

    if (compress) {
        diff_record.tstart = new_record.tstart;
        diff_record.tend = new_record.tend;
        diff_record.func_id = new_record.func_id;
        write_compressed_record(__datafh, ref_window_id, diff_record);
    } else {
        write_uncompressed_record(__datafh, new_record);
    }

    __record_window[2] = __record_window[1];
    __record_window[1] = __record_window[0];
    __record_window[0] = new_record;
}

void logger_init(int rank, int nprocs) {

    // Map the functions we will use later
    // We did not intercept fprintf
    MAP_OR_FAIL(fopen)
    MAP_OR_FAIL(fclose)
    MAP_OR_FAIL(fwrite)
    MAP_OR_FAIL(ftell)
    MAP_OR_FAIL(fseek)
    MAP_OR_FAIL(mkdir)

    // Initialize the global values
    __filename2id_map = hashmap_new();

    START_TIMESTAMP = recorder_wtime();

    RECORDER_REAL_CALL(mkdir) ("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char logfile_name[256];
    char metafile_name[256];
    sprintf(logfile_name, "logs/%d.itf", rank);
    sprintf(metafile_name, "logs/%d.mt", rank);
    __datafh = RECORDER_REAL_CALL(fopen) (logfile_name, "wb");
    __metafh = RECORDER_REAL_CALL(fopen) (metafile_name, "wb");

    // Global metadata, include compression mode, time resolution
    const char* comp_mode = getenv("RECORDER_COMPRESSION_MODE");
    if (comp_mode) __compression_mode = atoi(comp_mode);
    if (__compression_mode == COMP_ZLIB)        // Initialize zlib if compression mode is COMP_ZLIB
        zlib_init();
    if (rank == 0) {
        FILE* global_metafh = RECORDER_REAL_CALL(fopen) ("logs/recorder.mt", "wb");
        RecorderGlobalDef global_def = {
            .time_resolution = TIME_RESOLUTION,
            .total_ranks = nprocs,
            .compression_mode = __compression_mode
        };
        RECORDER_REAL_CALL(fwrite)(&global_def, sizeof(RecorderGlobalDef), 1, global_metafh);
        RECORDER_REAL_CALL(fclose)(global_metafh);
    }
}


void logger_exit() {
    /* Call this before close file since we still could have data in zlib's buffer waiting to write out*/
    if (__compression_mode == COMP_ZLIB)
        zlib_exit();

    /* Close the log file */
    if ( __datafh ) {
        RECORDER_REAL_CALL(fclose) (__datafh);
        __datafh = NULL;
    }

    /* Write out local metadata information */
    RecorderLocalDef local_def = {
        .start_timestamp = START_TIMESTAMP,
        .end_timestamp = recorder_wtime(),
        .num_files = hashmap_length(__filename2id_map)
    };
    RECORDER_REAL_CALL(fwrite) (&local_def, sizeof(local_def), 1, __metafh);

    /* Write out filename mappings, we call stat() to get file size
     * since __datafh is already closed (null), the stat() function
     * won't be intercepted. */
    if (hashmap_length(__filename2id_map) > 0 ) {
        for(int i = 0; i< __filename2id_map->table_size; i++) {
            if(__filename2id_map->data[i].in_use != 0) {
                char *filename = __filename2id_map->data[i].key;
                int id = __filename2id_map->data[i].data;
                size_t file_size = get_file_size(filename);
                int filename_len = strlen(filename);
                RECORDER_REAL_CALL(fwrite) (&id, sizeof(id), 1, __metafh);
                RECORDER_REAL_CALL(fwrite) (&file_size, sizeof(file_size), 1, __metafh);
                RECORDER_REAL_CALL(fwrite) (&filename_len, sizeof(filename_len), 1, __metafh);
                RECORDER_REAL_CALL(fwrite) (filename, sizeof(char), filename_len, __metafh);
            }
        }
    }
    hashmap_free(__filename2id_map);
    __filename2id_map = NULL;
    if ( __metafh) {
        RECORDER_REAL_CALL(fclose) (__metafh);
        __metafh = NULL;
    }
}
