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
 */
char** read_local_metadata(const char* path) {
    int lines = 0;
    char ch;

    FILE* f = fopen(path, "r");
    while(!feof(f)) {
        ch = fgetc(f);
        if(ch == '\n')
            lines++;
    }
    printf("lines: %d\n", lines);
    char **filenames = malloc(sizeof(char*) * lines);
    char filename[256], id[256], size[256];
    int i = 0;
    enum Field {FIELD_FILE, FIELD_ID, FIELD_SIZE};
    enum Field current_field = FIELD_FILE;

    fseek(f, 0, SEEK_SET);
    while(!feof(f)) {
        ch = fgetc(f);
        if (ch == '\n') {
            size[i] = 0;
            i = 0;
            current_field = FIELD_FILE;
            filenames[atoi(id)] = strdup(filename);
            printf("%s, %s, %s\n", filename, id, size);
        } else if (ch == ' ') {
            if(current_field == FIELD_FILE)
                filename[i] = 0;
            if(current_field == FIELD_ID)
                id[i] = 0;
            i = 0;
            current_field = (current_field == FIELD_FILE) ? FIELD_ID : FIELD_SIZE;
        } else {
            if (current_field == FIELD_FILE)
                filename[i++] = ch;
            if (current_field == FIELD_ID)
                id[i++] = ch;
            if (current_field == FIELD_SIZE)
                size[i++] = ch;
        }
    }
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
int read_record(FILE *f, RecorderGlobalDef global_def, Record *record) {
    int tstart, tend;
    fread(&(record->status), sizeof(char), 1, f);
    fread(&tstart, sizeof(int), 1, f);
    fread(&tend, sizeof(int), 1, f);
    fread(&(record->func_id), sizeof(unsigned char), 1, f);
    record->arg_count = 0;
    record->tstart = tstart * global_def.time_resolution + global_def.start_timestamp;
    record->tend = tstart * global_def.time_resolution + global_def.start_timestamp;

    char buffer[1024];
    char* ret = fgets(buffer, 1024, f);     // read a line
    if (!ret) return -1;                    // EOF is read
    buffer[strlen(buffer)-1] = 0;           // remove the trailing '\n'
    if (strlen(buffer) == 0 ) return 0;     // no arguments

    //printf("strlen: %ld, %s\n", strlen(buffer), buffer);
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
    record->args[arg_idx][pos] = 0; // the last argument
    return 0;
}

/*
 * Read one log file (for one  rank)
 */
void read_logfile(const char* path, char** filenames, RecorderGlobalDef global_def) {
    char text_logfile_path[256];
    sprintf(text_logfile_path, "%s.txt", path);
    FILE* out_file = fopen(text_logfile_path, "w");
    FILE* in_file = fopen(path, "rb");

    Record record;
    while( read_record(in_file, global_def, &record) == 0) {

        // convert filename id to filename string
        if (record.func_id < 120) { // for POSIX and MPI only now
            for(int idx = 0; idx < 8; idx++) {
                char pos = 0b00000001 << idx;
                if (pos & filename_arg_pos[record.func_id]) {
                    int filename_id = atoi(record.args[idx]);
                    char* filename = filenames[filename_id];
                    free(record.args[idx]);
                    record.args[idx] = strdup(filename);
                }
            }
        }

        //printf("%d %f %f %s", record.status, record.tstart, record.tend, func_list[record.func_id]);
        fprintf(out_file, "%d %f %f %s", record.status, record.tstart, record.tend, func_list[record.func_id]);
        for(int i = 0; i < record.arg_count; i++) {
            //printf(" %s", record.args[i]);
            fprintf(out_file, " %s", record.args[i]);
            free(record.args[i]);
        }
        //printf("\n");
        fprintf(out_file, "\n");
        free(record.args);
    }

    fclose(out_file);
    fclose(in_file);
}

int main(int argc, char **argv) {
    char* log_dir_path = argv[1];
    char global_metadata_path[256], local_metadata_path[256], logfile_path[256];
    RecorderGlobalDef global_def;


    sprintf(global_metadata_path, "%s/recorder.mt", log_dir_path);
    read_global_metadata(global_metadata_path, &global_def);

    for(int i = 0; i < global_def.total_ranks ; i++) {
        sprintf(local_metadata_path, "%s/%d.mt" , log_dir_path, i);
        sprintf(logfile_path, "%s/%d.itf" , log_dir_path, i);
        char** filenames = read_local_metadata(local_metadata_path);
        read_logfile(logfile_path, filenames, global_def);
        free(filenames);
    }

    return 0;
}
