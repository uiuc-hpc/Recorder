#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "recorder.h"

/* Global handle for the local trace log */
FILE *__datafh;
FILE *__metafh;

/* Global (defined in recorder.h) function name to id map */
hashmap_map *__func2id_map;
/* Global (defined in recorder.h) filename to id map */
hashmap_map *__filename2id_map;


/* Map function name to Integer in binary format */
static inline unsigned char get_func_id(const char *func) {
    int id = -1;
    if (!func || !__func2id_map)
        return id;

    /* filename already exists */
    if (hashmap_get(__func2id_map, func, &id) == MAP_OK)
        return id;

    /* insert filename into the map */
    id = hashmap_length(__func2id_map);
    hashmap_put(__func2id_map, func, id);
    return id;
}


/* Map filename to Integer in binary format */
static inline unsigned char get_filename_id(const char *filename) {
    int id = -1;
    if (!filename || !__filename2id_map)
        return id;

    /* filename already exists */
    if (hashmap_get(__filename2id_map, filename, &id) == MAP_OK)
        return id;

    /* insert filename into the map */
    id = hashmap_length(__filename2id_map);
    hashmap_put(__filename2id_map, filename, id);
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
    static const char *exclusions[] = {"/dev/", "/proc", "/sys", "pipe:[",
                            "anon_inode:[", "socket:[", NULL};
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

static inline void write_in_binary(IoOperation_t *op) {
    MAP_OR_FAIL(fwrite)
    RECORDER_MPI_CALL(fwrite) (op, sizeof(IoOperation_t), 1, __datafh);
}

static inline void write_in_text(double tstart, double tend, const char* log_text) {
    fprintf(__datafh, "%.6f %s %.6f\n", tstart, log_text, tend-tstart);
}

void write_data_operation(const char *func, const char *filename, double start, double end,
                          size_t offset, size_t count_or_whence, const char *log_text) {

    if (__datafh == NULL) return;       // Haven't initialized yet.

    if (exclude_filename(filename)) return;

    IoOperation_t op = {
        .func_id = get_func_id(func),
        .filename_id = get_filename_id(filename),
        .start_time = start,
        .end_time = end,
        .attr1 = offset,
        .attr2 = count_or_whence
    };

    //write_in_text(start, end, log_text);
    write_in_binary(&op);
}

void logger_init(int rank) {
    __func2id_map = hashmap_new();
    __filename2id_map = hashmap_new();

    MAP_OR_FAIL(fopen)
    if (rank == 0) {
        mkdir("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    char logfile_name[100];
    char metafile_name[100];
    sprintf(logfile_name, "logs/%d.itf", rank);
    sprintf(metafile_name, "logs/%d.mt", rank);
    __datafh = RECORDER_MPI_CALL(fopen) (logfile_name, "wb");
    __metafh = RECORDER_MPI_CALL(fopen) (metafile_name, "w+");
}

void logger_exit() {
    /* Write out the function and filename mappings */
    int i;
    if (hashmap_length(__func2id_map) <= 0 ) return;
    for(i = 0; i< __func2id_map->table_size; i++) {
        if(__func2id_map->data[i].in_use != 0) {
            const char *func = __func2id_map->data[i].key;
            int id = __func2id_map->data[i].data;
            fprintf(__metafh, "%s %d\n", func, id);
        }
    }

    if (hashmap_length(__filename2id_map) <= 0 ) return;
    for(i = 0; i< __filename2id_map->table_size; i++) {
        if(__filename2id_map->data[i].in_use != 0) {
            const char *filename = __filename2id_map->data[i].key;
            int id = __filename2id_map->data[i].data;
            fprintf(__metafh, "%s %d\n", filename, id);
        }
    }

    hashmap_free(__func2id_map);
    hashmap_free(__filename2id_map);

    MAP_OR_FAIL(fclose)
    if ( __metafh)  RECORDER_MPI_CALL(fclose) (__metafh);
    if ( __datafh ) RECORDER_MPI_CALL(fclose) (__datafh);
}
