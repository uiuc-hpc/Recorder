#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "reader.h"

/*
 * Read the global metada file and write the information in *global_def
 */
void read_global_metadata(const char *path, RecorderGlobalDef *global_def) {
    FILE* f = fopen(path, "rb");
    fread(global_def, sizeof(RecorderGlobalDef), 1, f);
    fclose(f);
}

/*
 * Read one local metadata file (one rank)
 * And output in *local_def
 */
void read_local_metadata(const char* path, RecorderLocalDef *local_def) {
    FILE* f = fopen(path, "rb");
    fread(local_def, sizeof(RecorderLocalDef), 1, f);
    char **filemap = malloc(sizeof(char*) * local_def->num_files);

    int id;
    size_t file_size;
    int filename_len;
    for(int i = 0; i < local_def->num_files; i++) {
        fread(&id, sizeof(id), 1, f);
        fread(&file_size, sizeof(file_size), 1, f);
        fread(&filename_len, sizeof(filename_len), 1, f);
        filemap[id] = malloc(sizeof(char) * filename_len);
        fread(filemap[id], sizeof(char), filename_len, f);
        filemap[id][filename_len] = 0;
        //printf("%d %zu %d %s\n", i, file_size, filename_len, filemap[id]);
    }
    local_def->filemap = filemap;
    printf("total records: %d\n", local_def->total_records);
    fclose(f);
}


/*
 * Read one record (one line) from the trace file FILE* f
 * return 0 on success, -1 if read EOF
 * Note, this function does not perform decoding or decompression
 * in: FILE* f
 * in: RecorderGlobalDef global_def
 * out: record
 */
int read_record(FILE *f, RecorderGlobalDef global_def, RecorderLocalDef local_def, Record *record) {
    int tstart, tend;
    fread(&(record->status), sizeof(char), 1, f);
    fread(&tstart, sizeof(int), 1, f);
    fread(&tend, sizeof(int), 1, f);
    fread(&(record->func_id), sizeof(unsigned char), 1, f);
    record->arg_count = 0;
    record->tstart = tstart * global_def.time_resolution + local_def.start_timestamp;
    record->tend = tend * global_def.time_resolution + local_def.start_timestamp;

    char buffer[1024];
    char* ret = fgets(buffer, 1024, f);     // read a line
    if (!ret) return -1;                    // EOF is read
    buffer[strlen(buffer)-1] = 0;           // remove the trailing '\n'
    if (strlen(buffer) == 0 ) return 0;     // no arguments

    for(int i = 0; i < strlen(buffer); i++) {
        if(buffer[i] == ' ')
            record->arg_count++;
    }

    record->args = malloc(sizeof(char*) * record->arg_count);
    int arg_idx = -1, pos = 0;
    for(int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == ' ') {
            if ( arg_idx >= 0 )
                record->args[arg_idx][pos] = 0;
            arg_idx++;
            pos = 0;
            record->args[arg_idx] = malloc(sizeof(char) * 64);
        } else
            record->args[arg_idx][pos++] = buffer[i];
    }
    record->args[arg_idx][pos] = 0;         // the last argument
    return 0;
}


/*
 * Write all original records (encoded and compressed) to text file
 */
void decode(Record *records, RecorderGlobalDef global_def, RecorderLocalDef local_def) {
    for(int i = 0; i < local_def.total_records; i++) {
        Record *record = &(records[i]);             // Use pointer to directly modify record in array

        // decompress peephole compressed record
        if (record->status & 0b10000000) {
            int ref_id = record->func_id;
            char **diff_args = record->args;                // diff_args only have the different arguments
            int diff_arg_id = 0;
            record->func_id = records[i-1-ref_id].func_id;
            record->arg_count = records[i-1-ref_id].arg_count;
            record->args = records[i-1-ref_id].args;
            for(int arg_pos= 0; arg_pos < 7; arg_pos++) {   // set the different arguments
                char diff_bit = 0b00000001 << arg_pos;
                if (diff_bit & record->status) {
                    record->args[arg_pos] = diff_args[diff_arg_id++];
                }
            }
        }

        // Decode
        // convert filename id to filename string
        for(int idx = 0; idx < 8; idx++) {
            char pos = 0b00000001 << idx;
            if (pos & filename_arg_pos[record->func_id]) {
                // 1. !record->status, then this record is not compressed, need to map the filename
                // 2. pos & record.status = true so that the filename is not the same as the refered record
                //    otherwise the filename has already been set by the referred record
                if ((!record->status) || (pos & record->status)) {
                    int filename_id = atoi(record->args[idx]);
                    char* filename = local_def.filemap[filename_id];
                    free(record->args[idx]);
                    record->args[idx] = strdup(filename);
                }
            }
        }
    }
}


/*
 * Read one rank's log file
 * This function does not perform decoding and decompression
 * return array of records in the file
 */
Record* read_logfile(const char* logfile_path, RecorderGlobalDef global_def, RecorderLocalDef local_def) {
    Record *records = malloc(sizeof(Record) * local_def.total_records);
    FILE* in_file = fopen(logfile_path, "rb");
    for(int i = 0; i < local_def.total_records; i++) {
        read_record(in_file, global_def, local_def, &(records[i]));
        //printf("%d %f %f %s %d", records[i].status, records[i].tstart, records[i].tend, func_list[records[i].func_id], records[i].arg_count);
    }
    fclose(in_file);
    return records;
}

int is_read_function(Record record) {
    const char* func_name = func_list[record.func_id];
    if(strstr(func_name, "read") != NULL)
        return 1;
    return 0;
}
int is_write_function(Record record) {
    const char* func_name = func_list[record.func_id];
    if (strstr(func_name, "write") != NULL)
        return 1;
    return 0;
}
size_t get_io_size(Record record) {
    size_t size = 0;
    size_t nmemb;
    switch(record.func_id) {
        case 5:                     // write
        case 6:                     // read
        case 9:                     // pread
        case 10:                    // pread64
        case 11:                    // pwrite
        case 12:                    // pwrite64
            sscanf(record.args[2], "%zu", &size);
            break;
        case 13:                    // readv
        case 14:                    // writev
            sscanf(record.args[1], "%zu", &size);
            break;
        case 20:                    // fwrite
        case 21:                    // fread
            sscanf(record.args[1], "%zu", &size);
            sscanf(record.args[2], "%zu", &nmemb);
            size = nmemb * size;
            break;
    }
    return size;
}

size_t get_io_offset() {
    return 0;
}

int get_open_flag(Record record, char*filename, char* flag_str) {
    // in POSIX standard open() page
    #define FLAG_COUNT 22
    static int all_flags[FLAG_COUNT] = {
        O_RDONLY, O_WRONLY, O_RDWR,
        O_APPEND, O_ASYNC, O_CLOEXEC, O_CREAT, O_DIRECT,
        O_DIRECTORY, O_DSYNC, O_EXCL, O_LARGEFILE, O_NOATIME,
        O_NOCTTY, O_NOFOLLOW, O_NONBLOCK, O_NDELAY, O_PATH,
        O_SYNC, O_RSYNC, O_TMPFILE, O_TRUNC};
    static char *all_flag_str[FLAG_COUNT] = {
        "O_RDONLY", "O_WRONLY", "O_RDWR",
        "O_APPEND", "O_ASYNC", "O_CLOEXEC", "O_CREAT", "O_DIRECT",
        "O_DIRECTORY", "O_DSYNC", "O_EXCL", "O_LARGEFILE", "O_NOATIME",
        "O_NOCTTY", "O_NOFOLLOW", "O_NONBLOCK", "O_NDELAY", "O_PATH",
        "O_SYNC", "O_RSYNC", "O_TMPFILE", "O_TRUNC"};

    int flags;
    char *mode_str;
    switch(record.func_id) {
        case 2:     // open
        case 3:     // open64
            //flag_str = calloc(128, sizeof(char));
            flags = atoi(record.args[1]);
            flag_str[0] = 0;
            for(int i = 0; i < FLAG_COUNT; i++) {
                if(flags & all_flags[i]) {
                    strcat(flag_str, all_flag_str[i]);
                    strcat(flag_str, " | ");
                }
            }
            if(strlen(flag_str) > 3)
                flag_str[strlen(flag_str)-3] = 0;
            filename = strcpy(filename, record.args[0]);
            return 1;
        case 17:    // fopen
        case 18:    // fopen64
        case 60:    // fdopen
            //flag_str = calloc(128, sizeof(char));
            mode_str = record.args[1];
            if(strstr(mode_str, "r") != NULL) {
                strcpy(flag_str, "O_RDONLY");
            } else if(strstr(mode_str, "w") != NULL) {
                strcpy(flag_str, "O_WRONLY | O_CREAT | O_TRUNC");
            } else if(strstr(mode_str, "a") != NULL) {
                strcpy(flag_str, "O_WRONLY | O_CREAT | O_APPEND");
            } else if(strstr(mode_str, "r+") != NULL) {
                strcpy(flag_str, "O_RDWR");
            } else if(strstr(mode_str, "w+") != NULL) {
                strcpy(flag_str, "O_RDWR | O_CREAT | O_TRUNC");
            } else if(strstr(mode_str, "a+") != NULL) {
                strcpy(flag_str, "O_RDWR | O_CREAT | O_APPEND");
            }
            //printf("here: %s\n", flag_str);
            filename = strcpy(filename, record.args[0]);
            return 1;
        default:
            return 0;
    }
}


// The input *records must have been decoded and decompressed
void get_bandwidth_info(Record *records, RecorderLocalDef local_def, RecorderBandwidthInfo *bwinfo) {
    bwinfo->read_size = 0;
    bwinfo->write_size = 0;
    double read_time = 0, write_time = 0;
    for(int i = 0; i < local_def.total_records; i++) {
        Record record = records[i];
        if (is_read_function(record)) {
            bwinfo->read_size += get_io_size(record);
            read_time += (record.tend - record.tstart);
        }
        if (is_write_function(record)) {
            bwinfo->write_size += get_io_size(record);
            write_time += (record.tend - record.tstart);
        }
    }
    bwinfo->read_bandwidth = bwinfo->read_size / read_time;
    bwinfo->write_bandwidth = bwinfo->write_size / write_time;
}
