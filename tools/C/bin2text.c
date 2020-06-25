#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"


/*
 * Write all original records (encoded and compressed) to text file
 */
void write_to_textfile(const char* path, Record *records, RecorderLocalDef local_def) {
    FILE* out_file = fopen(path, "w");
    for(int i = 0; i < local_def.total_records; i++) {
        Record record = records[i];
        //printf("%d %f %f %s %d", record->status, record->tstart, record->tend, func_list[record->func_id], record->arg_count);
        fprintf(out_file, "%f %f %s", record.tstart, record.tend, func_list[record.func_id]);
        for(int arg_id = 0; arg_id < record.arg_count; arg_id++) {
            char *arg = record.args[arg_id];

            // convert filename id to filename string
            if (arg_id < 8) {
                char pos = 0b00000001 << arg_id;
                if (pos & filename_arg_pos[record.func_id]) {
                    int filename_id = atoi(record.args[arg_id]);
                    arg = local_def.filemap[filename_id];
                }
            }

            //printf(" %s", arg);
            fprintf(out_file, " %s", arg);

        }
        //printf("\n");
        fprintf(out_file, "\n");
    }
    fclose(out_file);
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

        printf("RANK %d\n", i);
        read_local_metadata(local_metadata_path, &local_def);
        Record *records = read_logfile(logfile_path, global_def, local_def);
        decode(records, global_def, local_def);
        write_to_textfile(textfile_path, records, local_def);

        //free(local_def.filemap);
    }

    return 0;
}
