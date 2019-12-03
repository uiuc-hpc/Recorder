#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unordered_map>
#include <vector>
#include <array>

#include "reader.h"

using namespace std;

/*
 * Read the global metada file and write the information in *global_def
 */
void read_global_metadata(string path, RecorderGlobalDef *global_def) {
    FILE* f = fopen(path.c_str(), "rb");
    fread(global_def, sizeof(RecorderGlobalDef), 1, f);
    fclose(f);
}

/*
 * Read one local metadata file (one rank)
 * And output in *local_def
 */
void read_local_metadata(string path, RecorderLocalDef *local_def) {
    FILE* f = fopen(path.c_str(), "rb");
    fread(local_def, sizeof(RecorderLocalDef), 1, f);
    char **filemap = (char**)malloc(sizeof(char*) * local_def->num_files);
    size_t *file_sizes = (size_t*)malloc(sizeof(size_t) * local_def->num_files);

    int id;
    int filename_len;
    for(int i = 0; i < local_def->num_files; i++) {
        fread(&id, sizeof(id), 1, f);
        fread(&(file_sizes[id]), sizeof(size_t), 1, f);
        fread(&filename_len, sizeof(filename_len), 1, f);
        filemap[id] = (char*)malloc(sizeof(char) * filename_len);
        fread(filemap[id], sizeof(char), filename_len, f);
        filemap[id][filename_len] = 0;
        //printf("%d %zu %d %s\n", i, file_size, filename_len, filemap[id]);
    }
    local_def->filemap = filemap;
    local_def->file_sizes = file_sizes;
    printf("total records: %d\n", local_def->total_records);
    fclose(f);
}


unordered_map<string,int> merge_filemaps(RecorderGlobalDef *global_def, RecorderLocalDef *local_defs) {
    // Merge filemap from all ranks
    unordered_map<string, int > filemap;
    for(int rank = 0; rank < global_def->total_ranks; rank++) {
        for(int i = 0; i < local_defs[rank].num_files; i++) {
            string filename = local_defs[rank].filemap[i];
            if(filemap.find(filename) == filemap.end())
                filemap[filename] = filemap.size();
        }
    }
    return filemap;
}



/*
 * Read one record (one line) from the trace file FILE* f
 * return 0 on success, -1 if read EOF
 * Note that this function does not perform decoding or decompression
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

    record->args = (char**)malloc(sizeof(char*) * record->arg_count);
    int arg_idx = -1, pos = 0;
    for(int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == ' ') {
            if ( arg_idx >= 0 )
                record->args[arg_idx][pos] = 0;
            arg_idx++;
            pos = 0;
            record->args[arg_idx] = (char*)malloc(sizeof(char) * 64);
        } else
            record->args[arg_idx][pos++] = buffer[i];
    }
    record->args[arg_idx][pos] = 0;         // the last argument
    return 0;
}


static inline char** copy_args(char** args, int count) {
    char **new_args = (char**)malloc(sizeof(char*) * count);
    for(int i = 0; i < count; i++) {
        new_args[i] = (char*)malloc(sizeof(char) * (strlen(args[i])+1));
        strcpy(new_args[i], args[i]);
    }
    return new_args;

}

void decode(vector<Record> records, RecorderGlobalDef global_def, RecorderLocalDef local_def) {
    for(int i = 0; i < local_def.total_records; i++) {
        Record *record = &(records[i]);             // Use pointer to directly modify record in array

        // decompress peephole compressed record
        if (record->status & 0b10000000) {
            int ref_id = record->func_id;
            char **diff_args = record->args;                // diff_args only have the different arguments
            int diff_arg_id = 0;
            record->func_id = records[i-1-ref_id].func_id;
            record->arg_count = records[i-1-ref_id].arg_count;
            record->args = copy_args(records[i-1-ref_id].args, record->arg_count);
            for(int arg_pos = 0; arg_pos < 7; arg_pos++) {   // set the different arguments
                char diff_bit = 0b00000001 << arg_pos;
                if (diff_bit & record->status) {
                    free(record->args[arg_pos]);
                    record->args[arg_pos] = diff_args[diff_arg_id++];
                }
            }
        }

        // Decode
        // convert filename id to filename string
        /*
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
        */
    }
}


/*
 * Read one rank's log file
 * This function does not perform decoding and decompression
 * return array of records in the file
 */
vector<Record> read_logfile(string logfile_path, RecorderGlobalDef global_def, RecorderLocalDef local_def) {
    vector<Record> records(local_def.total_records);
    FILE* in_file = fopen(logfile_path.c_str(), "rb");
    for(int i = 0; i < local_def.total_records; i++) {
        read_record(in_file, global_def, local_def, &(records[i]));
        //printf("%d %f %f %s %d", records[i].status, records[i].tstart, records[i].tend, func_list[records[i].func_id], records[i].arg_count);
    }
    fclose(in_file);
    return records;
}

int is_posix_read_function(Record *record) {
    if(record->func_id >= 66)   // the first 66 functions are POSIX I/O
        return 0;
    const char* func_name = func_list[record->func_id];
    if(strstr(func_name, "read") != NULL)
        return 1;
    return 0;
}
int is_posix_write_function(Record *record) {
    if(record->func_id >= 66)   // the first 66 functions are POSIX I/O
        return 0;
    const char* func_name = func_list[record->func_id];
    if (strstr(func_name, "write") != NULL)
        return 1;
    return 0;
}

char* get_io_filename(Record *record, RecorderLocalDef *local_def) {
    int file_id = atoi(record->args[0]);
    if(file_id < local_def->num_files)
        return local_def->filemap[file_id];
    return NULL;
}

/**
 * Get the io size of one decoded/decompressed record
 */
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



typedef struct Interval_t {
    size_t offset;
    size_t size;
} Interval;

Interval new_interval(size_t offset, size_t size) {
    Interval interval;
    interval.offset = offset;
    interval.size = size;
    return interval;
}

int interval_comparator(const void *p, const void *q) {
    size_t l = ((Interval *)p)->offset;
    size_t r = ((Interval *)p)->offset;
    return l - r;
}
/**
 * all_records is a pointer to every rank's records, e.g. all_records[0] are processor 0's records
 */
vector<vector<Interval>> allocate_intervals(Record **all_records, RecorderGlobalDef *global_def, RecorderLocalDef *local_defs) {

    unordered_map<string, int> filemap = merge_filemaps(global_def, local_defs);

    int id, local_fileid;
    string filename;
    int *count_per_file = (int*)calloc(sizeof(int), filemap.size());

    for(int rank = 0; rank < global_def->total_ranks; rank++) {
        for(int i = 0; i < local_defs[rank].total_records; i++) {
            switch(all_records[rank][i].func_id) {
                case 5:
                case 6:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 20:
                case 21:
                    local_fileid = atoi(all_records[rank][i].args[0]);
                    filename = local_defs[rank].filemap[local_fileid];
                    id = filemap[filename];
                    count_per_file[id] += 1;
                    break;
            }
        }
    }

    vector<vector<Interval>> all_intervals(filemap.size());
    for(int i = 0; i < filemap.size(); i++) {
        vector<Interval> intervals(count_per_file[id]);
        all_intervals.push_back(intervals);
    }

    return all_intervals;
}



void get_access_pattern(Record **all_records, RecorderGlobalDef *global_def, RecorderLocalDef *local_defs) {

    unordered_map<string, int> filemap = merge_filemaps(global_def, local_defs);
    //vector<vector<Interval>> intervals = allocate_intervals(all_records, global_def, local_defs);
    vector<vector<Interval>> intervals(filemap.size());

    size_t offset, size, nmemb;
    int file_id, origin, flag;
    for(int rank = 0; rank < global_def->total_ranks; rank++) {
        Record *records = all_records[rank];
        RecorderLocalDef local_def = local_defs[rank];
        size_t *curr_offsets = (size_t*)malloc(sizeof(size_t) * local_def.num_files);
        for(int i = 0; i < local_def.total_records; i++) {
            Record record = records[i];
            switch(record.func_id) {
                case 5:                     // write
                case 6:                     // read
                    sscanf(record.args[2], "%zu", &size);
                    file_id = atoi(record.args[0]);

                    file_id = filemap[string(local_def.filemap[file_id])];

                    intervals[file_id].push_back(new_interval(curr_offsets[file_id], size));
                    curr_offsets[file_id] += size;
                case 9:                     // pread
                case 10:                    // pread64
                case 11:                    // pwrite
                case 12:                    // pwrite64
                    sscanf(record.args[2], "%zu", &size);
                    sscanf(record.args[3], "%zu", &offset);
                    file_id = atoi(record.args[0]);

                    file_id = filemap[string(local_def.filemap[file_id])];

                    intervals[file_id].push_back(new_interval(curr_offsets[file_id], size));
                    curr_offsets[file_id] = offset + size;
                    break;
                case 13:                    // readv
                case 14:                    // writev
                    sscanf(record.args[1], "%zu", &size);
                    file_id = atoi(record.args[0]);

                    file_id = filemap[string(local_def.filemap[file_id])];
                    intervals[file_id].push_back(new_interval(curr_offsets[file_id], size));

                    curr_offsets[file_id] += size;
                    break;
                case 20:                    // fwrite
                case 21:                    // fread
                    sscanf(record.args[1], "%zu", &size);
                    sscanf(record.args[2], "%zu", &nmemb);
                    size = nmemb * size;
                    file_id = atoi(record.args[0]);

                    file_id = filemap[string(local_def.filemap[file_id])];
                    intervals[file_id].push_back(new_interval(curr_offsets[file_id], size));

                    curr_offsets[file_id] += size;
                    break;
                case 7:                     // lseek
                case 8:                     // lseek64
                case 23:                    // fseek
                    file_id = atoi(record.args[0]);
                    file_id = filemap[string(local_def.filemap[file_id])];
                    offset = atol(record.args[1]);
                    origin = atoi(record.args[2]);
                    if (origin == SEEK_SET)
                        curr_offsets[file_id] = offset;
                    if (origin == SEEK_CUR)
                        curr_offsets[file_id] += offset;
                    break;
                case 2:                     // open
                case 3:                     // open64
                    file_id = atoi(record.args[0]);
                    file_id = filemap[string(local_def.filemap[file_id])];
                    flag = atoi(record.args[1]);
                    if (!(flag & O_APPEND))
                        curr_offsets[file_id] = 0;
                    break;
                case 17:                    // fopen
                case 18:                    // fopen64
                case 60:                    // fdopen
                    file_id = atoi(record.args[0]);
                    file_id = filemap[string(local_def.filemap[file_id])];
                    if(strstr(record.args[1], "a") == NULL)
                        curr_offsets[file_id] = 0;
                    break;
            }
        }
    }

    //qsort((void*)intervals, total, sizeof(Interval), interval_comparator);
}

int main(int argc, char* argv[]) {
    RecorderGlobalDef global_def;
    vector<RecorderLocalDef> local_defs;
    vector<vector<Record>> all_records;

    read_global_metadata(string(argv[1])+"/recorder.mt", &global_def);
    for(int i = 0; i < global_def.total_ranks; i++) {
        RecorderLocalDef local_def;
        read_local_metadata(string(argv[1])+"/"+to_string(i)+".mt", &local_def);
        local_defs.push_back(local_def);

        vector<Record> records = read_logfile(string(argv[1])+"/"+to_string(i)+".itf", global_def, local_def);
        decode(records, global_def, local_def);
        all_records.push_back(records);
    }
    return 0;

}

