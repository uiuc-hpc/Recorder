#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include "recorder.h"

/* Global file handler (per rank) for the local trace log file */
FILE *__datafh;
FILE *__metafh;
/* Starting timestamp of each rank */
double START_TIMESTAMP;
double TIME_RESOLUTION = 0.000001;
/* Filename to integer map */
hashmap_map *__filename2id_map;
/* Hold several previous records for compression */
#define RECORD_WINDOW_SIZE 3
Record __record_window[RECORD_WINDOW_SIZE];


/*
 * Map filename to Integer in binary format
 * The filename must be absolute pathname
 */
int get_filename_id(const char *filename) {
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
    struct stat sb;
    int res = stat(filename, &sb);
    if (res != 0 ) return -1;               // file not exist or some other error

    int is_regular_file = S_ISREG(sb.st_mode);
    if (!is_regular_file) return -1;        // is directory
    return sb.st_size;
    /*
    FILE* f = RECORDER_MPI_CALL(fopen(filename, "r"));
    if(f == NULL) return -1;
    RECORDER_MPI_CALL(fseek(f, 0L, SEEK_END));
    long size = RECORDER_MPI_CALL(ftell(f));
    RECORDER_MPI_CALL(fclose(f));
    return size;
    */
}

static inline void write_record_args(FILE* f, int arg_count, char** args) {
    char invalid_str[] = "???";
    for(int i = 0; i < arg_count; i++) {
        fprintf(f, " ");
        if(args[i])
            RECORDER_MPI_CALL(fwrite) (args[i], strlen(args[i]), 1, f);
        else
            RECORDER_MPI_CALL(fwrite) (invalid_str, strlen(invalid_str), 1, f);
    }
    fprintf(f, "\n");
}

static inline void write_compressed_record(FILE* f, char ref_window_id, Record diff_record) {
    char status = '0';
    int tstart = (diff_record.tstart - START_TIMESTAMP) / TIME_RESOLUTION;
    int tend   = (diff_record.tend - START_TIMESTAMP) / TIME_RESOLUTION;
    RECORDER_MPI_CALL(fwrite) (&status, sizeof(char), 1, f);
    RECORDER_MPI_CALL(fwrite) (&tstart, sizeof(int), 1, f);
    RECORDER_MPI_CALL(fwrite) (&tend, sizeof(int), 1, f);
    RECORDER_MPI_CALL(fwrite) (&ref_window_id, sizeof(char), 1, f);
    write_record_args(f, diff_record.arg_count, diff_record.args);
}

static inline void write_uncompressed_record(FILE *f, Record record) {
    char status = '0';
    int tstart = (record.tstart - START_TIMESTAMP) / TIME_RESOLUTION;
    int tend   = (record.tend - START_TIMESTAMP) / TIME_RESOLUTION;
    RECORDER_MPI_CALL(fwrite) (&status, sizeof(char), 1, f);
    RECORDER_MPI_CALL(fwrite) (&tstart, sizeof(int), 1, f);
    RECORDER_MPI_CALL(fwrite) (&tend, sizeof(int), 1, f);
    RECORDER_MPI_CALL(fwrite) (&(record.func_id), sizeof(unsigned char), 1, f);
    write_record_args(f, record.arg_count, record.args);
}

static inline void write_record_in_text(FILE *f, Record record) {
    fprintf(f, "%f %f %s", record.tstart, record.tend, get_function_name_by_id(record.func_id));
    write_record_args(f, record.arg_count, record.args);
}

Record get_diff_record(Record old_record, Record new_record) {
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

    //write_record_in_text(__datafh, record);

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

void logger_init(int rank) {
    // Map the functions we will use later
    // We did not intercept fprintf
    MAP_OR_FAIL(fopen)
    MAP_OR_FAIL(fclose)
    MAP_OR_FAIL(fwrite)
    MAP_OR_FAIL(ftell)
    MAP_OR_FAIL(fseek)

    // Initialize the global values
    __filename2id_map = hashmap_new();

    mkdir("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char logfile_name[256];
    char metafile_name[256];
    sprintf(logfile_name, "logs/%d.itf", rank);
    sprintf(metafile_name, "logs/%d.mt", rank);
    __datafh = RECORDER_MPI_CALL(fopen) (logfile_name, "wb");
    __metafh = RECORDER_MPI_CALL(fopen) (metafile_name, "w");

    START_TIMESTAMP = recorder_wtime();
}


void logger_exit() {
    /* Close the log file */
    if ( __datafh ) {
        RECORDER_MPI_CALL(fclose) (__datafh);
        __datafh = NULL;
    }

    /* Write out filename mappings, we call stat() to get file size
     * since __datafh is already closed (null), the stat() function
     * won't be intercepted. */
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
}
