#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <errno.h>
#include "recorder.h"
#include "recorder-sequitur.h"


#define TIME_RESOLUTION 0.000001
#define VERSION_STR "2.2.3"
#define TS_BUFFER_ELEMENTS 1024


pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

struct RecorderLogger {
    int rank;

    Grammar     cfg;
    RecordHash* cst;

    char traces_dir[512];
    char cst_path[1024];
    char cfg_path[1024];

    double start_ts;
    FILE*  ts_file;
    int*   ts;          // memory buffer for timestamps (tstart, tend)
    int    ts_index;    // current position of ts buffer, spill to file once full.
};

struct RecorderLogger logger;

static int current_terminal_id = 0;

// External global values, initialized here
bool __recording;



/**
 * Key: func_id + res + all arguments in string
 *
 * arguments seperated by space ' '
 */
char* compose_call_key(Record *record, int* key_len) {
    int arg_count = record->arg_count;
    char **args = record->args;

    char invalid_str[] = "???";
    int invalid_str_len = strlen(invalid_str);

    int arg_strlen = arg_count;
    for(int i = 0; i < arg_count; i++) {
        if(args[i]) {
            for(int j = 0; j < strlen(args[i]); j++)
                if(args[i][j] == ' ') args[i][j] = '_';
            arg_strlen += strlen(args[i]);
        } else {
            arg_strlen += strlen(invalid_str);
        }
    }

    *key_len = sizeof(record->func_id) + sizeof(record->res) + sizeof(record->arg_count) + sizeof(int) + arg_strlen;


    char* key = recorder_malloc(*key_len);
    int pos = 0;
    memcpy(key+pos, &record->func_id, sizeof(record->func_id));
    pos += sizeof(record->func_id);
    memcpy(key+pos, &record->res, sizeof(record->res));
    pos += sizeof(record->res);
    memcpy(key+pos, &record->arg_count, sizeof(record->arg_count));
    pos += sizeof(record->arg_count);
    memcpy(key+pos, &arg_strlen, sizeof(int));
    pos += sizeof(int);

    for(int i = 0; i < arg_count; i++) {
        if(args[i]) {
            memcpy(key+pos, args[i], strlen(args[i]));
            pos += strlen(args[i]);
        } else {
            memcpy(key+pos, invalid_str, strlen(invalid_str));
            pos += invalid_str_len;
        }
        key[pos] = ' ';
        pos += 1;
    }

    return key;
}


void free_record(Record *record) {
    if(record == NULL)
        return;

    if(record->args) {
        for(int i = 0; i < record->arg_count; i++)
            free(record->args[i]);  // note here we don't use recorder_fere
                                    // because the memory was potentially
                                    // allocated by realpath() or other system calls.
        recorder_free(record->args, sizeof(char*)*record->arg_count);
    }

    record->args = NULL;
    recorder_free(record, sizeof(Record));
}


void write_record(Record *record) {
    pthread_mutex_lock(&g_mutex);

    int key_len;
    char* key = compose_call_key(record, &key_len);

    RecordHash *entry = NULL;
    HASH_FIND(hh, logger.cst, key, key_len, entry);
    if(entry) {                         // Found
        entry->count++;
        recorder_free(key, key_len);
    } else {                            // Not exist, add to hash table
        entry = (RecordHash*) recorder_malloc(sizeof(RecordHash));
        entry->key = key;
        entry->key_len = key_len;
        entry->rank = logger.rank;
        entry->terminal_id = current_terminal_id++;
        entry->count = 0;
        HASH_ADD_KEYPTR(hh, logger.cst, entry->key, entry->key_len, entry);
    }

    append_terminal(&logger.cfg, entry->terminal_id, 1);
    free_record(record);

    // write timestamps
    int tstart = (record->tstart-logger.start_ts) / TIME_RESOLUTION;
    int tend   = (record->tend-logger.start_ts)   / TIME_RESOLUTION;
    logger.ts[logger.ts_index++] = tstart;
    logger.ts[logger.ts_index++] = tend;
    if(logger.ts_index == TS_BUFFER_ELEMENTS) {
        RECORDER_REAL_CALL(fwrite)(logger.ts, sizeof(int), TS_BUFFER_ELEMENTS, logger.ts_file);
        logger.ts_index = 0;
    }

    pthread_mutex_unlock(&g_mutex);
}

void logger_init(int rank, int nprocs) {
    // Map the functions we will use later
    // We did not intercept fprintf
    MAP_OR_FAIL(fopen);
    MAP_OR_FAIL(fclose);
    MAP_OR_FAIL(fwrite);
    MAP_OR_FAIL(rmdir);
    MAP_OR_FAIL(access);
    MAP_OR_FAIL(mkdir);
    MAP_OR_FAIL(PMPI_Barrier);

    // Initialize the global values
    logger.rank = rank;
    logger.start_ts = recorder_wtime();
    logger.cst = NULL;
    sequitur_init(&logger.cfg);
    logger.ts = recorder_malloc(sizeof(int)*TS_BUFFER_ELEMENTS);
    logger.ts_index = 0;

    const char* base_dir = getenv("RECORDER_TRACES_DIR");
    if(base_dir)
        sprintf(logger.traces_dir, "%s/recorder-logs", base_dir);
    else
        sprintf(logger.traces_dir, "recorder-logs"); // current directory


    sprintf(logger.cst_path, "%s/%d.cst", logger.traces_dir, rank);
    sprintf(logger.cfg_path, "%s/%d.cfg", logger.traces_dir, rank);

    if(rank == 0) {
        if(RECORDER_REAL_CALL(access) (logger.traces_dir, F_OK) != -1)
            RECORDER_REAL_CALL(rmdir) (logger.traces_dir);
        RECORDER_REAL_CALL(mkdir) (logger.traces_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    RECORDER_REAL_CALL(PMPI_Barrier) (MPI_COMM_WORLD);


    char ts_filename[1024];
    sprintf(ts_filename, "%s/%d.ts", logger.traces_dir, rank);
    logger.ts_file = RECORDER_REAL_CALL(fopen) (ts_filename, "wb");

    if (rank == 0) {
        char metadata_filename[1024] = {0};
        sprintf(metadata_filename, "%s/recorder.mt", logger.traces_dir);
        FILE* metafh = RECORDER_REAL_CALL(fopen) (metadata_filename, "wb");
        RecorderMetadata metadata = {
            .time_resolution = TIME_RESOLUTION,
            .total_ranks = nprocs,
            .start_ts  = logger.start_ts,
        };
        RECORDER_REAL_CALL(fwrite)(&metadata, sizeof(RecorderMetadata), 1, metafh);

        for(int i = 0; i < sizeof(func_list)/sizeof(char*); i++) {
            const char *funcname = get_function_name_by_id(i);
            if(strstr(funcname, "PMPI_"))       // replace PMPI with MPI
                RECORDER_REAL_CALL(fwrite)(funcname+1, strlen(funcname)-1, 1, metafh);
            else
                RECORDER_REAL_CALL(fwrite)(funcname, strlen(funcname), 1, metafh);
            RECORDER_REAL_CALL(fwrite)("\n", sizeof(char), 1, metafh);
        }
        RECORDER_REAL_CALL(fclose)(metafh);

        char version_filename[1024];
        sprintf(version_filename, "%s/VERSION", logger.traces_dir);
        FILE* version_file = RECORDER_REAL_CALL(fopen) (version_filename, "w");
        RECORDER_REAL_CALL(fwrite) (VERSION_STR, 5, 1, version_file);
        RECORDER_REAL_CALL(fclose)(version_file);
    }

    __recording = true;     // set the extern globals
}

void* serialize_cst(RecordHash *table, size_t *len) {
    *len = sizeof(int);

    RecordHash *entry, *tmp;
    HASH_ITER(hh, table, entry, tmp) {
        *len = *len + entry->key_len + sizeof(int)*2;
    }

    int count = HASH_COUNT(table);
    void *res = recorder_malloc(*len);
    void *ptr = res;

    memcpy(ptr, &count, sizeof(int));
    ptr += sizeof(int);

    HASH_ITER(hh, table, entry, tmp) {

        memcpy(ptr, &entry->terminal_id, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, &entry->key_len, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, entry->key, entry->key_len);
        ptr = ptr + entry->key_len;
    }

    return res;
}

void dump_cst_local() {
    FILE* f = RECORDER_REAL_CALL(fopen) (logger.cst_path, "wb");
    size_t len;
    void* data = serialize_cst(logger.cst, &len);
    RECORDER_REAL_CALL(fwrite)(data, 1, len, f);
    RECORDER_REAL_CALL(fclose)(f);
}

void cleanup_cst(RecordHash* cst) {
    RecordHash *entry, *tmp;
    HASH_ITER(hh, cst, entry, tmp) {
        HASH_DEL(cst, entry);
        recorder_free(entry->key, entry->key_len);
        recorder_free(entry, sizeof(RecordHash));
    }
    cst = NULL;
}

void dump_cfg_local() {
    FILE* f = RECORDER_REAL_CALL(fopen) (logger.cfg_path, "wb");
    int count;
    int* data = serialize_grammar(&logger.cfg, &count);
    RECORDER_REAL_CALL(fwrite)(data, sizeof(int), count, f);
    RECORDER_REAL_CALL(fclose)(f);
}

void logger_finalize() {

    __recording = false;    // set the extern global

    if(logger.ts_index > 0)
        RECORDER_REAL_CALL(fwrite)(logger.ts, sizeof(int), logger.ts_index, logger.ts_file);
    RECORDER_REAL_CALL(fclose)(logger.ts_file);
    recorder_free(logger.ts, sizeof(int)*TS_BUFFER_ELEMENTS);

    dump_cst_local();
    cleanup_cst(logger.cst);
    dump_cfg_local();
    sequitur_cleanup(&logger.cfg);
}

