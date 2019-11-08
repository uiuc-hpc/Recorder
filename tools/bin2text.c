#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/recorder-log-format.h"

/*
 * Read the global metada file and write the information in global_def
 */
void read_global_metadata(const char *path, RecorderGlobalDef *global_def) {
    FILE* f = fopen(path, "rb");
    fread(global_def, sizeof(RecorderGlobalDef), 1, f);
    fclose(f);
}


/*
 * Read one local metadata file (one rank)
 * And output in local_def
 * Also return the map of filenames to integersk
 */
char** read_local_metadata(const char* path, RecorderLocalDef *local_def) {
    FILE* f = fopen(path, "rb");
    fread(local_def, sizeof(RecorderLocalDef), 1, f);
    char **filenames = malloc(sizeof(char*) * local_def->num_files);

    int id;
    size_t file_size;
    int filename_len;
    for(int i = 0; i < local_def->num_files; i++) {
        fread(&id, sizeof(id), 1, f);
        fread(&file_size, sizeof(file_size), 1, f);
        fread(&filename_len, sizeof(filename_len), 1, f);
        filenames[id] = malloc(sizeof(char) * (filename_len+1));
        fread(filenames[id], sizeof(char), filename_len, f);
        filenames[id][filename_len] = 0;
        printf("%d %s\n", i, filenames[id]);
    }
    printf("total records: %d\n", local_def->total_records);

    fclose(f);
    return filenames;
}


/*
 * Read one record (one line) from the trace file FILE* f
 * return 0 on success, -1 if read EOF
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
    record->tend = tstart * global_def.time_resolution + local_def.start_timestamp;

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
 * Read one rank's log file
 */
void write_to_textfile(const char* path, Record *records, char** filenames, RecorderGlobalDef global_def, RecorderLocalDef local_def) {
    FILE* out_file = fopen(path, "w");

    for(int i = 0; i < local_def.total_records; i++) {
        Record *record = &(records[i]);             // Use pointer to directly modify record in array

        // decompress peephole compressed record
        if (record->status & 0b10000000) {
            int ref_id = record->func_id;
            char **diff_args = record->args;
            record->func_id = records[i-1-ref_id].func_id;
            record->arg_count = records[i-1-ref_id].arg_count;
            record->args = records[i-1-ref_id].args;
            for(int idx = 0; idx < 7; idx++) {      // set the different arguments
                char diff_bit = 0b00000001 << idx;
                if (diff_bit & record->status)
                    record->args[idx] = diff_args[idx];
            }
        }

        // Decode
        // convert filename id to filename string
        for(int idx = 0; idx < 8; idx++) {
            char pos = 0b00000001 << idx;
            if (pos & filename_arg_pos[record->func_id]) {
                int filename_id = atoi(record->args[idx]);
                char* filename = filenames[filename_id];
                free(record->args[idx]);
                record->args[idx] = strdup(filename);
            }
        }

        printf("%d %f %f %s %d", record->status, record->tstart, record->tend, func_list[record->func_id], record->arg_count);
        fprintf(out_file, "%f %f %s", record->tstart, record->tend, func_list[record->func_id]);
        for(int arg_id = 0; arg_id < record->arg_count; arg_id++) {
            printf(" %s", record->args[arg_id]);
            fprintf(out_file, " %s", record->args[arg_id]);
        }
        printf("\n");
        fprintf(out_file, "\n");
    }
    fclose(out_file);
}


/*
 * Read one rank's log file
 * This function does not perform decoding and decompression
 * return array of records in the file
 */
Record* read_logfile(const char* logfile_path, RecorderGlobalDef global_def, RecorderLocalDef local_def) {
    Record *records = malloc(sizeof(Record) * local_def.total_records);
    FILE* in_file = fopen(logfile_path, "rb");
    for(int i = 0; i < local_def.total_records; i++)
        read_record(in_file, global_def, local_def, &(records[i]));
    fclose(in_file);
    return records;
}


int main(int argc, char **argv) {
    char* log_dir_path = argv[1];
    char global_metadata_path[256], local_metadata_path[256], logfile_path[256],  textfile_path[256];
    RecorderGlobalDef global_def;
    RecorderLocalDef local_def;

    sprintf(global_metadata_path, "%s/recorder.mt", log_dir_path);
    read_global_metadata(global_metadata_path, &global_def);

    for(int i = 0; i < global_def.total_ranks ; i++) {
        sprintf(local_metadata_path, "%s/%d.mt" , log_dir_path, i);
        sprintf(logfile_path, "%s/%d.itf" , log_dir_path, i);
        sprintf(textfile_path, "%s/%d.itf.txt" , log_dir_path, i);
        char** filenames = read_local_metadata(local_metadata_path, &local_def);
        Record *records = read_logfile(logfile_path, global_def, local_def);
        write_to_textfile(textfile_path, records, filenames, global_def, local_def);
        //free(filenames);
    }

    return 0;
}
