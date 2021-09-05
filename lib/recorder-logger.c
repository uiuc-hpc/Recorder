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


pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;


struct RecorderLogger {
    int rank;
    double startTimestamp;

    Grammar     cfg;
    RecordHash* cst;

    char traces_dir[512];
    char cst_path[1024];
    char cfg_path[1024];
};
struct RecorderLogger logger;

static int current_terminal_id = 0;

// External global values, initialized here
bool __recording;


/**
 * Key: func_id + ret + all arguments in string
 *
 * arguments seperated by space ' '
 */
char* compose_call_key(Record *record, int* key_len) {
    int arg_count = record->arg_count;
    char **args = record->args;

    char invalid_str[] = "???";
    int invalid_str_len = strlen(invalid_str);

    *key_len = sizeof(record->func_id) + sizeof(record->res) + sizeof(record->arg_count) + arg_count;
    for(int i = 0; i < arg_count; i++) {
        if(args[i]) {
            for(int j = 0; j < strlen(args[i]); j++)
                if(args[i][j] == ' ') args[i][j] = '_';
            *key_len += strlen(args[i]);
        } else {
            *key_len += strlen(invalid_str);
        }
    }

    char* key = recorder_malloc(*key_len);
    int pos = 0;
    memcpy(key+pos, &record->func_id, sizeof(record->func_id));
    pos += sizeof(record->func_id);
    memcpy(key+pos, &record->res, sizeof(record->res));
    pos += sizeof(record->res);
    memcpy(key+pos, &record->arg_count, sizeof(record->arg_count));
    pos += sizeof(record->arg_count);

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
    logger.startTimestamp = recorder_wtime();
    logger.cst = NULL;
    sequitur_init(&logger.cfg);

    const char* base_dir = getenv("RECORDER_TRACES_DIR");
    char global_meta_filename[1024] = {0};

    if(base_dir)
        sprintf(logger.traces_dir, "%s/recorder-logs", base_dir);
    else
        sprintf(logger.traces_dir, "recorder-logs"); // current directory

    sprintf(logger.cst_path, "%s/%d.cst", logger.traces_dir, rank);
    sprintf(logger.cfg_path, "%s/%d.cfg", logger.traces_dir, rank);
    sprintf(global_meta_filename, "%s/recorder.mt", logger.traces_dir);

    if(rank == 0) {
        if(RECORDER_REAL_CALL(access) (logger.traces_dir, F_OK) != -1)
            RECORDER_REAL_CALL(rmdir) (logger.traces_dir);
        RECORDER_REAL_CALL(mkdir) (logger.traces_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    RECORDER_REAL_CALL(PMPI_Barrier) (MPI_COMM_WORLD);


    if (rank == 0) {
        FILE* global_metafh = RECORDER_REAL_CALL(fopen) (global_meta_filename, "wb");
        RecorderGlobalDef global_def = {
            .time_resolution = TIME_RESOLUTION,
            .total_ranks = nprocs,
        };
        RECORDER_REAL_CALL(fwrite)(&global_def, sizeof(RecorderGlobalDef), 1, global_metafh);

        for(int i = 0; i < sizeof(func_list)/sizeof(char*); i++) {
            const char *funcname = get_function_name_by_id(i);
            if(strstr(funcname, "PMPI_"))       // replace PMPI with MPI
                RECORDER_REAL_CALL(fwrite)(funcname+1, strlen(funcname)-1, 1, global_metafh);
            else
                RECORDER_REAL_CALL(fwrite)(funcname, strlen(funcname), 1, global_metafh);
            RECORDER_REAL_CALL(fwrite)("\n", sizeof(char), 1, global_metafh);
        }
        RECORDER_REAL_CALL(fclose)(global_metafh);

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

    dump_cst_local();
    cleanup_cst(logger.cst);
    dump_cfg_local();
    sequitur_cleanup(&logger.cfg);
}
