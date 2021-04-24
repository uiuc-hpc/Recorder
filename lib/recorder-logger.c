#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include "recorder.h"


#define TIME_RESOLUTION 0.000001
#define RECORD_WINDOW_SIZE 3                    // A sliding window for peephole compression
#define MEMBUF_SIZE 6*1024*1024                 // Memory buffer size, default 6MB
#define VERSION_STR "2.2.0"


struct RecorderLogger {
    int rank;
    FILE *trace_file;                           // log file
    FILE *meta_file;                            // metadata file
    double startTimestamp;
    RecorderLocalDef localDef;                  // Local metadata information
    CompressionMode compMode;                   // Compression Mode
    Record* recordWindow[RECORD_WINDOW_SIZE];   // For Recorder Compression Mode
};

/*
 * Memery buffer to hold records. Write out when its full
 */
struct MemBuf {
    void *buffer;
    int size;
    int pos;
};



// Global objects
struct RecorderLogger logger;
struct MemBuf membuf;

// External global values, initialized here
bool __recording;


void membuf_dump(struct MemBuf *membuf) {
    RECORDER_REAL_CALL(fwrite) (membuf->buffer, 1, membuf->pos, logger.trace_file);
    membuf->pos = 0;
}
void membuf_init(struct MemBuf* membuf) {
    membuf->buffer = recorder_malloc(MEMBUF_SIZE);
    membuf->pos = 0;
}
void membuf_destroy(struct MemBuf *membuf) {
    if(membuf->pos > 0)
        membuf_dump(membuf);
    recorder_free(membuf->buffer, MEMBUF_SIZE);
}
void membuf_append(struct MemBuf* membuf, const void *ptr, int length) {
    if (length >= membuf->size) {
        membuf_dump(membuf);
        RECORDER_REAL_CALL(fwrite) (ptr, 1, length, logger.trace_file);
        return;
    }
    if (membuf->pos + length >= membuf->size) {
        membuf_dump(membuf);
    }
    memcpy(membuf->buffer+membuf->pos, ptr, length);
    membuf->pos += length;
}





static inline int startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}


Record* get_diff_record(Record *old_record, Record *new_record) {
    Record *diff_record = recorder_malloc(sizeof(Record)) ;
    diff_record->status = 0b10000000;

    // Get the number of different arguments
    int total_args = old_record->arg_count;
    int diff_args = 0;
    int i;
    for(i = 0; i < total_args; i++)
        if(strcmp(old_record->args[i], new_record->args[i]) !=0)
            diff_args++;

    // record.args store only the different arguments
    // record.status keeps track the position of different arguments
    diff_record->arg_count = diff_args;
    int idx = 0;
    diff_record->args = recorder_malloc(sizeof(char *) * diff_args);
    static char diff_bits[] = {0b10000001, 0b10000010, 0b10000100, 0b10001000,
                                0b10010000, 0b10100000, 0b11000000};
    for(i = 0; i < total_args; i++) {
        if(strcmp(old_record->args[i], new_record->args[i]) !=0) {
            diff_record->args[idx++] = strdup(new_record->args[i]);
            diff_record->status = diff_record->status | diff_bits[i];
        }
    }
    return diff_record;
}

// 0. Helper function, write all function arguments
void write_record_arguments(FILE* f, Record *record) {
    int arg_count = record->arg_count;
    char **args = record->args;

    char invalid_str[] = "???";
    int i, j;
    for(i = 0; i < arg_count; i++) {
        membuf_append(&membuf, " ", 1);
        if(args[i]) {
            for(j = 0; j < strlen(args[i]); j++)
                if(args[i][j] == ' ') args[i][j] = '_';
            membuf_append(&membuf, args[i], strlen(args[i]));
        } else
            membuf_append(&membuf, invalid_str, strlen(invalid_str));
    }
    membuf_append(&membuf, "\n", 1);
}

/* Mode 1. Write record in plan text format */
// tstart tend function args...
void write_in_text(Record *record) {
    FILE *f = logger.trace_file;
    const char* func = get_function_name_by_id(record->func_id);
    char* tstart = ftoa(record->tstart);
    char* tend = ftoa(record->tend);
    char* res = itoa(record->res);
    membuf_append(&membuf, tstart, strlen(tstart));
    membuf_append(&membuf, " ", 1);
    membuf_append(&membuf, tend, strlen(tend));
    membuf_append(&membuf, " ", 1);
    membuf_append(&membuf, res, strlen(res));
    membuf_append(&membuf, " ", 1);
    membuf_append(&membuf, func, strlen(func));
    write_record_arguments(f, record);
}

// Mode 2. Write in binary format, no compression
void write_in_binary(Record *record) {
    FILE *f = logger.trace_file;
    int tstart = (record->tstart - logger.startTimestamp) / TIME_RESOLUTION;
    int tend   = (record->tend - logger.startTimestamp) / TIME_RESOLUTION;
    membuf_append(&membuf, &(record->status), sizeof(record->status));
    membuf_append(&membuf, &tstart, sizeof(tstart));
    membuf_append(&membuf, &tend, sizeof(tend));
    membuf_append(&membuf, &(record->res), sizeof(record->res));
    membuf_append(&membuf, &(record->func_id), sizeof(record->func_id));
    write_record_arguments(f, record);
}

void free_record(Record *record) {
    if(record == NULL)
        return;

    if(record->args) {
        int i;
        for(i = 0; i < record->arg_count; i++)
            free(record->args[i]);
        recorder_free(record->args, sizeof(char*)*record->arg_count);
    }

    record->args = NULL;
    recorder_free(record, sizeof(Record));
}

// Mode 3. Write in Recorder format (binary + peephole compression)
void write_in_recorder(Record *new_record) {

    bool compress = false;
    Record *diff_record = NULL;
    int min_diff_count = new_record->arg_count;
    char ref_window_id;
    int i;
    for(i = 0; i < RECORD_WINDOW_SIZE; i++) {
        Record *old_record = logger.recordWindow[i];
        if(old_record == NULL) break;

        // Only meets the following conditions that we consider to compress it:
        // 1. same function as the one in sliding window
        // 2. have same number of arguments; have only 1 ~ 7 arguments
        // 3. the number of different arguments is less the number of total arguments
        if ((old_record->func_id == new_record->func_id) &&
            (new_record->arg_count == old_record->arg_count) &&
            (new_record->arg_count < 8) && (new_record->arg_count > 0)) {

            Record *tmp_record = get_diff_record(old_record, new_record);

            // Currently has the minimum number of different arguments
            if(tmp_record->arg_count < min_diff_count) {
                free_record(diff_record);

                min_diff_count = tmp_record->arg_count;
                diff_record = tmp_record;
                ref_window_id = i;
                compress = true;
                break;
            } else {
                free_record(tmp_record);
            }
        }
    }

    if (compress) {
        diff_record->tstart = new_record->tstart;
        diff_record->tend = new_record->tend;
        diff_record->func_id = ref_window_id;
        diff_record->res = new_record->res;
        write_in_binary(diff_record);
    } else {
        new_record->status = 0b00000000;
        write_in_binary(new_record);
    }

    // Free the oldest record in the window
    free_record(diff_record);
    free_record(logger.recordWindow[RECORD_WINDOW_SIZE-1]);

    // Move the sliding window
    for(i = RECORD_WINDOW_SIZE-1; i > 0; i--)
        logger.recordWindow[i] = logger.recordWindow[i-1];
    logger.recordWindow[0] = new_record;
}


void write_record(Record *record) {
    logger.localDef.total_records++;
    logger.localDef.function_count[record->func_id]++;

    switch(logger.compMode) {
        case COMP_TEXT:     // 0
            write_in_text(record);
            free_record(record);
            break;
        case COMP_BINARY:   // 1
            write_in_binary(record);
            free_record(record);
            break;
        default:            // 2, default if compMode not set
            write_in_recorder(record);
            break;
    }
}

void logger_init(int rank, int nprocs) {
    // Map the functions we will use later
    // We did not intercept fprintf
    MAP_OR_FAIL(fopen);
    MAP_OR_FAIL(fclose);
    MAP_OR_FAIL(fwrite);
    MAP_OR_FAIL(remove);
    MAP_OR_FAIL(access);
    MAP_OR_FAIL(mkdir);
    MAP_OR_FAIL(PMPI_Barrier);

    // Initialize the global values
    logger.rank = rank;
    logger.startTimestamp = recorder_wtime();
    int i;
    for(i = 0; i < RECORD_WINDOW_SIZE; i++)
        logger.recordWindow[i] = NULL;


    const char* base_dir = getenv("RECORDER_TRACES_DIR");
    char traces_dir[1024] = {0};
    char logfile_name[1024] = {0};
    char metafile_name[1024] = {0};
    char global_meta_filename[1024] = {0};
    if(base_dir)
        sprintf(traces_dir, "%s/recorder-logs", base_dir);
    else
        sprintf(traces_dir, "recorder-logs"); // current directory
    sprintf(logfile_name, "%s/%d.itf", traces_dir, rank);
    sprintf(metafile_name, "%s/%d.mt", traces_dir, rank);
    sprintf(global_meta_filename, "%s/recorder.mt", traces_dir);

    if(rank == 0) {
        if(RECORDER_REAL_CALL(access)  (traces_dir, F_OK) != -1)
            RECORDER_REAL_CALL(remove) (traces_dir);
        RECORDER_REAL_CALL(mkdir) (traces_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    RECORDER_REAL_CALL(PMPI_Barrier) (MPI_COMM_WORLD);


    logger.trace_file = RECORDER_REAL_CALL(fopen) (logfile_name, "wb");
    logger.meta_file = RECORDER_REAL_CALL(fopen) (metafile_name, "wb");

    // Global metadata, include compression mode, time resolution
    logger.compMode = COMP_RECORDER;
    const char* comp_mode = getenv("RECORDER_COMPRESSION_MODE");
    if (comp_mode) logger.compMode = atoi(comp_mode);

    if (rank == 0) {
        FILE* global_metafh = RECORDER_REAL_CALL(fopen) (global_meta_filename, "wb");
        RecorderGlobalDef global_def = {
            .time_resolution = TIME_RESOLUTION,
            .total_ranks = nprocs,
            .compression_mode = logger.compMode,
            .peephole_window_size = RECORD_WINDOW_SIZE
        };
        RECORDER_REAL_CALL(fwrite)(&global_def, sizeof(RecorderGlobalDef), 1, global_metafh);

        unsigned int i;
        for(i = 0; i < sizeof(func_list)/sizeof(char*); i++) {
            const char *funcname = get_function_name_by_id(i);
            if(strstr(funcname, "PMPI_"))       // replace PMPI with MPI
                RECORDER_REAL_CALL(fwrite)(funcname+1, strlen(funcname)-1, 1, global_metafh);
            else
                RECORDER_REAL_CALL(fwrite)(funcname, strlen(funcname), 1, global_metafh);
            RECORDER_REAL_CALL(fwrite)("\n", sizeof(char), 1, global_metafh);
        }
        RECORDER_REAL_CALL(fclose)(global_metafh);

        char version_filename[1024];
        sprintf(version_filename, "%s/VERSION", traces_dir);
        FILE* version_file = RECORDER_REAL_CALL(fopen) (version_filename, "w");
        RECORDER_REAL_CALL(fwrite) (VERSION_STR, 5, 1, version_file);
        RECORDER_REAL_CALL(fclose)(version_file);
    }

    membuf_init(&membuf);
    __recording = true;     // set the extern globals
}


void logger_finalize() {
    __recording = false;    // set the extern global

    int i;
    for(i = 0; i < RECORD_WINDOW_SIZE; i++)
        free_record(logger.recordWindow[i]);

    /* Write out local metadata information */
    FilenameHashTable* filename_table = get_filename_map();
    logger.localDef.num_files = HASH_COUNT(filename_table);
    logger.localDef.start_timestamp = logger.startTimestamp;
    logger.localDef.end_timestamp = recorder_wtime();
    RECORDER_REAL_CALL(fwrite) (&logger.localDef, sizeof(logger.localDef), 1, logger.meta_file);

    /* Write out filename mappings, we call stat() to get file size
     * since is already closed (null), the stat() function
     * won't be intercepted. */
    FilenameHashTable *item, *tmp;
    int id = 0;
    HASH_ITER(hh, filename_table, item, tmp) {
        int filename_len = strlen(item->name);
        size_t file_size = get_file_size(item->name);
        RECORDER_REAL_CALL(fwrite) (&id, sizeof(id), 1, logger.meta_file);
        RECORDER_REAL_CALL(fwrite) (&file_size, sizeof(file_size), 1, logger.meta_file);
        RECORDER_REAL_CALL(fwrite) (&filename_len, sizeof(filename_len), 1, logger.meta_file);
        RECORDER_REAL_CALL(fwrite) (item->name, sizeof(char), filename_len, logger.meta_file);
        id++;
    }

    membuf_destroy(&membuf);

    if ( logger.trace_file) {
        RECORDER_REAL_CALL(fclose) (logger.trace_file);
        logger.trace_file = NULL;
    }
    if ( logger.meta_file) {
        RECORDER_REAL_CALL(fclose) (logger.meta_file);
        logger.meta_file = NULL;
    }
}
