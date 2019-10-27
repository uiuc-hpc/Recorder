#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "recorder.h"

/* Global handle for the local trace log */
FILE *__datafh;
FILE *__metafh;

/* filename to id map */
hashmap_map *__filename2id_map;

/*
 * Map filename to Integer in binary format
 * The filename must be absolute pathname
 */
static inline unsigned char get_filename_id(const char *filename) {
    int id = -1;
    if (!filename || !__filename2id_map || strlen(filename) == 0)
        return id;

    const char *key = filename;

    /* filename already exists */
    if (hashmap_get(__filename2id_map, key, &id) == MAP_OK)
        return id;

    /* insert filename into the map */
    id = hashmap_length(__filename2id_map);
    hashmap_put(__filename2id_map, key, id);
    return id;
}

/*
 * Some of functions are not made by the application
 * And they are operating on wierd files
 * We should not include them in the trace file
 */
static inline int exclude_filename(const char *filename) {
    if (filename == NULL) return 0; // pass

    /* these are paths that we will not trace */
    // TODO put these in configuration file?
    static const char *exclusions[] = {"/dev/", "/proc", "/sys", "/etc", "/usr/tce/packages",
                        "pipe:[", "anon_inode:[", "socket:[", NULL};
    int i = 0;
    // Need to make sure both parameters for strncmp are not NULL, otherwise its gonna crash
    while(exclusions[i] != NULL) {
        int find = strncmp(exclusions[i], filename, strlen(exclusions[i]));
        if (find == 0)      // find it. should ignore this filename
            return 1;
        i++;
    }
    return 0;
}

static inline long get_file_size(char *filename) {
    FILE* f = RECORDER_MPI_CALL(fopen(filename, "r"));
    if(f == NULL) return -1;
    RECORDER_MPI_CALL(fseek(f, 0L, SEEK_END));
    long size = RECORDER_MPI_CALL(ftell(f));
    RECORDER_MPI_CALL(fclose(f));
    return size;
}


static inline void write_in_binary(IoOperation_t *op) {
    RECORDER_MPI_CALL(fwrite) (op, sizeof(IoOperation_t), 1, __datafh);
}

static inline void write_in_text(double tstart, double tend, const char* log_text) {
    fprintf(__datafh, "%.6f %s %.6f\n", tstart, log_text, tend-tstart);
}

void write_data_operation(const char *func, const char *filename, double start, double end,
                          size_t offset, size_t count_or_whence, const char *log_text) {

    if (__datafh == NULL) return;       // Haven't initialized yet or have finialized

    if (exclude_filename(filename)) return;

    IoOperation_t op = {
        .func_id = get_function_id_by_name(func),
        .filename_id = get_filename_id(filename),
        .start_time = start,
        .end_time = end,
        .attr1 = offset,
        .attr2 = count_or_whence
    };

    write_in_text(start, end, log_text);
    //write_in_binary(&op);
}

void write_uncompressed_record(FILE *f, Record record) {
    char status = '0';
    char invalid_str[] = "???";
    /*
    RECORDER_MPI_CALL(fwrite) (&status, sizeof(char), 1, f);
    RECORDER_MPI_CALL(fwrite) (&(record.tstart), sizeof(int), 1, f);
    RECORDER_MPI_CALL(fwrite) (&(record.tdur), sizeof(int), 1, f);
    RECORDER_MPI_CALL(fwrite) (&(record.func_id), sizeof(short), 1, f);
    */
    fprintf(f, "%d %d %s", record.tstart, record.tdur, record.func_id);
    for(size_t i = 0; i < record.arg_count; i++) {
        if(record.args[i]) {
            fprintf(f, " ");
            RECORDER_MPI_CALL(fwrite) (record.args[i], strlen(record.args[i]), 1, f);
        } else {
            RECORDER_MPI_CALL(fwrite) (invalid_str, strlen(invalid_str), 1, f);
        }
    }
    fprintf(f, "\n");
}

void write_record(Record record) {
    if (__datafh == NULL) return;
    write_uncompressed_record(__datafh, record);
}

void logger_init(int rank) {
    // Map the functions we will use later
    MAP_OR_FAIL(fopen)
    MAP_OR_FAIL(fclose)
    MAP_OR_FAIL(fwrite)
    MAP_OR_FAIL(ftell)
    MAP_OR_FAIL(fseek)
    // We did not intercept fprintf

    __filename2id_map = hashmap_new();

    mkdir("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    char logfile_name[100];
    char metafile_name[100];
    sprintf(logfile_name, "logs/%d.itf", rank);
    sprintf(metafile_name, "logs/%d.mt", rank);
    __datafh = RECORDER_MPI_CALL(fopen) (logfile_name, "wb");
    __metafh = RECORDER_MPI_CALL(fopen) (metafile_name, "w");
}

void logger_exit() {

    /* Write out the function and filename mappings */
    struct stat st;     // use to store file status, now we only interested in file sizes
    int i;
    if (hashmap_length(__filename2id_map) > 0 ) {
        for(i = 0; i< __filename2id_map->table_size; i++) {
            if(__filename2id_map->data[i].in_use != 0) {
                char *filename = __filename2id_map->data[i].key;
                int id = __filename2id_map->data[i].data;
                fprintf(__metafh, "%s %d %ld\n", filename, id, get_file_size(filename));

            }
        }
    }

    hashmap_free(__filename2id_map);
    __filename2id_map = NULL;
    if ( __metafh) {
        RECORDER_MPI_CALL(fclose) (__metafh);
        __metafh = NULL;
    }
    if ( __datafh ) {
        RECORDER_MPI_CALL(fclose) (__datafh);
        __datafh = NULL;
    }
}
