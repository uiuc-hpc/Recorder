#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include "recorder.h"
#include "zlib.h"


#define TIME_RESOLUTION 0.000001
#define RECORD_WINDOW_SIZE 3        // A sliding window for peephole compression
#define ZLIB_BUF_SIZE 4096          // For zlib compression //

/**
 * External global values, initialized here
 */
bool __recording;
FilenameHashTable* __filename_hashtable;


struct Logger {
    FILE *dataFile;                     // log file
    FILE *metaFile;                     // metadata file

    double startTimestamp;

    RecorderLocalDef localDef;          // Local metadata information

    CompressionMode compMode;           // Compression Mode

    // For Recorder Compression Mode
    Record recordWindow[RECORD_WINDOW_SIZE];

    // For Zlib
    z_stream zlibStream;
};

// Global object to access the Logger fileds
struct Logger __logger;


/**
 * ------------------------------------------------------------- //
 *
 * Memery buffer to hold records. Write out when its full
 *
 */
struct MemBuf {
    void *buffer;
    int size;
    int pos;
    void (*release) (struct MemBuf*);
    void (*append)(struct MemBuf*, const void* ptr, int length);
    void (*dump) (struct MemBuf*);
};

// global object to access the MemBuf methods
struct MemBuf __membuf;

void membufRelease(struct MemBuf *membuf) {
    free(membuf->buffer);
    membuf->pos = 0;
}
void membufAppend(struct MemBuf* membuf, const void *ptr, int length) {
    if (length >= membuf->size) {
        membuf->dump(membuf);
        RECORDER_REAL_CALL(fwrite) (ptr, 1, length, __logger.dataFile);
        return;
    }
    if (membuf->pos + length >= membuf->size) {
        membuf->dump(membuf);
    }
    memcpy(membuf->buffer+membuf->pos, ptr, length);
    membuf->pos += length;
}
void membufDump(struct MemBuf *membuf) {
    RECORDER_REAL_CALL(fwrite) (membuf->buffer, 1, membuf->pos, __logger.dataFile);
    membuf->pos = 0;
}
void membufInit(struct MemBuf* membuf) {
    membuf->size = 12*1024*1024;            // 12M
    membuf->buffer = malloc(membuf->size);
    membuf->pos = 0;
    membuf->release = membufRelease;
    membuf->append = membufAppend;
    membuf->dump = membufDump;
}
// --------------- End of Memory Buffer ------------------------ //





static inline int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}


static inline Record get_diff_record(Record old_record, Record new_record) {
    Record diff_record;
    diff_record.status = 0b10000000;
    diff_record.arg_count = 999;    // initialize an impossible large value at first

    // Same function should normally have the same number of arguments
    if (old_record.arg_count != new_record.arg_count)
        return diff_record;

    // Get the number of different arguments
    int count = 0;
    int i;
    for(i = 0; i < old_record.arg_count; i++)
        if(strcmp(old_record.args[i], new_record.args[i]) !=0)
            count++;

    // record.args store only the different arguments
    // record.status keeps track the position of different arguments
    diff_record.arg_count = count;
    int idx = 0;
    diff_record.args = malloc(sizeof(char *) * count);
    static char diff_bits[] = {0b10000001, 0b10000010, 0b10000100, 0b10001000,
                                0b10010000, 0b10100000, 0b11000000};
    for(i = 0; i < old_record.arg_count; i++) {
        if(strcmp(old_record.args[i], new_record.args[i]) !=0) {
            diff_record.args[idx++] = new_record.args[i];
            if(i < 7) {
                diff_record.status = diff_record.status | diff_bits[i];
            }
        }
    }
    return diff_record;
}

// 0. Helper function, write all function arguments
static inline void writeArguments(FILE* f, Record record) {
    int arg_count = record.arg_count;
    char **args = record.args;

    char invalid_str[] = "???";
    int i, j;
    for(i = 0; i < arg_count; i++) {
        __membuf.append(&__membuf, " ", 1);
        if(args[i]) {
            for(j = 0; j < strlen(args[i]); j++)
                if(args[i][j] == ' ') args[i][j] = '_';
            __membuf.append(&__membuf, args[i], strlen(args[i]));
        } else
            __membuf.append(&__membuf, invalid_str, strlen(invalid_str));
    }
    __membuf.append(&__membuf, "\n", 1);
}

/* Mode 1. Write record in plan text format */
// tstart tend function args...
static inline void writeInText(FILE *f, Record record) {
    const char* func = get_function_name_by_id(record.func_id);
    char* tstart = ftoa(record.tstart);
    char* tend = ftoa(record.tend);
    char* res = itoa(record.res);
    __membuf.append(&__membuf, tstart, strlen(tstart));
    __membuf.append(&__membuf, " ", 1);
    __membuf.append(&__membuf, tend, strlen(tend));
    __membuf.append(&__membuf, " ", 1);
    __membuf.append(&__membuf, res, strlen(res));
    __membuf.append(&__membuf, " ", 1);
    __membuf.append(&__membuf, func, strlen(func));
    writeArguments(f, record);
}

// Mode 2. Write in binary format, no compression
static inline void writeInBinary(FILE *f, Record record) {
    int tstart = (record.tstart - __logger.startTimestamp) / TIME_RESOLUTION;
    int tend   = (record.tend - __logger.startTimestamp) / TIME_RESOLUTION;
    __membuf.append(&__membuf, &(record.status), sizeof(char));
    __membuf.append(&__membuf, &tstart, sizeof(int));
    __membuf.append(&__membuf, &tend, sizeof(int));
    __membuf.append(&__membuf, &(record.res), sizeof(int));
    __membuf.append(&__membuf, &(record.func_id), sizeof(unsigned char));
    writeArguments(f, record);
}


// Mode 3. Write in Recorder format (binary + peephole compression)
static inline void writeInRecorder(FILE* f, Record new_record) {

    bool compress = false;
    Record diff_record;
    int min_diff_count = 999;
    char ref_window_id;
    int i;
    for(i = 0; i < RECORD_WINDOW_SIZE; i++) {
        Record record = __logger.recordWindow[i];
        // Only meets the following conditions that we consider to compress it:
        // 1. same function as the one in sliding window
        // 2. has at least 1 arguments
        // 3. has less than 8 arguments
        // 4. the number of different arguments is less the number of total arguments
        if ((record.func_id == new_record.func_id) && (new_record.arg_count < 8) &&
             (new_record.arg_count > 0) && (record.arg_count > 0)) {
            Record tmp_record = get_diff_record(record, new_record);

            // Cond.4
            if(tmp_record.arg_count >= new_record.arg_count)
                continue;

            // Currently has the minimum number of different arguments
            if(tmp_record.arg_count < min_diff_count) {
                min_diff_count = tmp_record.arg_count;
                ref_window_id = i;
                compress = true;
                diff_record = tmp_record;
            }
        }
    }

    if (compress) {
        diff_record.tstart = new_record.tstart;
        diff_record.tend = new_record.tend;
        diff_record.func_id = ref_window_id;
        diff_record.res = new_record.res;
        writeInBinary(__logger.dataFile, diff_record);
    } else {
        new_record.status = 0b00000000;
        writeInBinary(__logger.dataFile, new_record);
    }

    __logger.recordWindow[2] = __logger.recordWindow[1];
    __logger.recordWindow[1] = __logger.recordWindow[0];
    __logger.recordWindow[0] = new_record;

}

/* Mode 4. Compress the plain text with zlib and write it out */
static inline void writeInZlib(FILE *f, Record record) {
    static char in_buf[ZLIB_BUF_SIZE];
    static char out_buf[ZLIB_BUF_SIZE];
    sprintf(in_buf, "%f %f %s", record.tstart, record.tend, get_function_name_by_id(record.func_id));
    int i;
    for(i = 0; i < record.arg_count; i++) {
        strcat(in_buf, " ");
        if(record.args[i])
            strcat(in_buf, record.args[i]);
        else                    // some null argument ?
            strcat(in_buf, "???");
    }
    strcat(in_buf, "\n");

    __logger.zlibStream.avail_in = strlen(in_buf);
    __logger.zlibStream.next_in = in_buf;
    do {
        __logger.zlibStream.avail_out = ZLIB_BUF_SIZE;
        __logger.zlibStream.next_out = out_buf;
        int ret = deflate(&__logger.zlibStream, Z_NO_FLUSH);    /* no bad return value */
        unsigned have = ZLIB_BUF_SIZE - __logger.zlibStream.avail_out;
        RECORDER_REAL_CALL(fwrite) (out_buf, 1, have, f);
    } while (__logger.zlibStream.avail_out == 0);
}

void zlib_init() {
    /* allocate deflate state */
    __logger.zlibStream.zalloc = Z_NULL;
    __logger.zlibStream.zfree = Z_NULL;
    __logger.zlibStream.opaque = Z_NULL;
    deflateInit(&__logger.zlibStream, Z_DEFAULT_COMPRESSION);
}
void zlib_exit() {
    // Write out everythin zlib's buffer
    char out_buf[ZLIB_BUF_SIZE];
    do {

        __logger.zlibStream.avail_out = ZLIB_BUF_SIZE;
        __logger.zlibStream.next_out = out_buf;
        int ret = deflate(&__logger.zlibStream, Z_FINISH);    /* no bad return value */
        unsigned have = ZLIB_BUF_SIZE - __logger.zlibStream.avail_out;
        RECORDER_REAL_CALL(fwrite) (out_buf, 1, have, __logger.dataFile);
    } while (__logger.zlibStream.avail_out == 0);
    // Clean up and end the Zlib
    (void)deflateEnd(&__logger.zlibStream);
}


void write_record(Record record) {
    if (!__recording) return;       // have not initialized yet

    __logger.localDef.total_records++;
    __logger.localDef.function_count[record.func_id]++;

    switch(__logger.compMode) {
        case COMP_TEXT:     // 0
            writeInText(__logger.dataFile, record);
            break;
        case COMP_BINARY:   // 1
            writeInBinary(__logger.dataFile, record);
            break;
        case COMP_ZLIB:     // 3
            writeInZlib(__logger.dataFile, record);
            break;
        default:            // 2, default if compMode not set
            writeInRecorder(__logger.dataFile, record);
            break;
    }
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
    __filename_hashtable = NULL;
    __logger.startTimestamp = recorder_wtime();

    RECORDER_REAL_CALL(mkdir) ("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    char logfile_name[256];
    char metafile_name[256];
    sprintf(logfile_name, "logs/%d.itf", rank);
    sprintf(metafile_name, "logs/%d.mt", rank);
    __logger.dataFile = RECORDER_REAL_CALL(fopen) (logfile_name, "wb");
    __logger.metaFile = RECORDER_REAL_CALL(fopen) (metafile_name, "wb");

    // Global metadata, include compression mode, time resolution
    __logger.compMode = COMP_RECORDER;
    const char* comp_mode = getenv("RECORDER_COMPRESSION_MODE");
    if (comp_mode) __logger.compMode = atoi(comp_mode);

    if (__logger.compMode == COMP_ZLIB)        // Initialize zlib if compression mode is COMP_ZLIB
        zlib_init();
    if (rank == 0) {
        FILE* global_metafh = RECORDER_REAL_CALL(fopen) ("logs/recorder.mt", "wb");
        RecorderGlobalDef global_def = {
            .time_resolution = TIME_RESOLUTION,
            .total_ranks = nprocs,
            .compression_mode = __logger.compMode,
            .peephole_window_size = RECORD_WINDOW_SIZE
        };
        RECORDER_REAL_CALL(fwrite)(&global_def, sizeof(RecorderGlobalDef), 1, global_metafh);

        unsigned int i;
        for(i = 0; i < 256; i++) {
            const char *funcname = get_function_name_by_id(i);
            if(funcname) {
                RECORDER_REAL_CALL(fwrite)(funcname, strlen(funcname), 1, global_metafh);
                RECORDER_REAL_CALL(fwrite)("\n", sizeof(char), 1, global_metafh);
            } else {
                break;
            }
        }
        RECORDER_REAL_CALL(fclose)(global_metafh);


        FILE* version_file = RECORDER_REAL_CALL(fopen) ("logs/VERSION", "w");
        RECORDER_REAL_CALL(fwrite) ("2.1", 3, 1, version_file);
        RECORDER_REAL_CALL(fclose)(version_file);
    }

    membufInit(&__membuf);
    __recording = true;     // set the extern globals
}


void logger_exit() {
    __recording = false;    // set the extern global

    /* Call this before close file since we still could have data in zlib's buffer waiting to write out*/
    if (__logger.compMode == COMP_ZLIB)
        zlib_exit();


    /* Write out local metadata information */
    __logger.localDef.num_files = HASH_COUNT(__filename_hashtable);
    __logger.localDef.start_timestamp = __logger.startTimestamp;
    __logger.localDef.end_timestamp = recorder_wtime();
    RECORDER_REAL_CALL(fwrite) (&__logger.localDef, sizeof(__logger.localDef), 1, __logger.metaFile);

    /* Write out filename mappings, we call stat() to get file size
     * since is already closed (null), the stat() function
     * won't be intercepted. */
    FilenameHashTable *item, *tmp;
    int id = 0;
    HASH_ITER(hh, __filename_hashtable, item, tmp) {
        int filename_len = strlen(item->name);
        size_t file_size = get_file_size(item->name);
        printf("here: %s\n", item->name);
        RECORDER_REAL_CALL(fwrite) (&id, sizeof(id), 1, __logger.metaFile);
        RECORDER_REAL_CALL(fwrite) (&file_size, sizeof(file_size), 1, __logger.metaFile);
        RECORDER_REAL_CALL(fwrite) (&filename_len, sizeof(filename_len), 1, __logger.metaFile);
        RECORDER_REAL_CALL(fwrite) (item->name, sizeof(char), filename_len, __logger.metaFile);
        id++;
    }

    HASH_CLEAR(hh, __filename_hashtable);

    if ( __logger.metaFile) {
        RECORDER_REAL_CALL(fclose) (__logger.metaFile);
        __logger.metaFile = NULL;
    }

    __membuf.dump(&__membuf);
    __membuf.release(&__membuf);

    /* Close the log file */
    if ( __logger.dataFile) {
        RECORDER_REAL_CALL(fclose) (__logger.dataFile);
        __logger.dataFile = NULL;
    }

}
