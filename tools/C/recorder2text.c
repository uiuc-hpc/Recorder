#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"

/*
 * Write all original records (encoded and compressed) to text file
 */
void write_to_textfile(const char* path, Record *records, int len) {
    int i, arg_id;

    FILE* out_file = fopen(path, "w");
    for(i = 0; i < len; i++) {
        Record record = records[i];
        printf("%d %f %f %s %d\n", record.status, record.tstart, record.tend, func_list[record.func_id], record.arg_count);
        fprintf(out_file, "%f %f %d %s (", record.tstart, record.tend, record.res, func_list[record.func_id]);
        for(arg_id = 0; arg_id < record.arg_count; arg_id++) {
            char *arg = record.args[arg_id];
            fprintf(out_file, " %s", arg);
        }
        fprintf(out_file, " )\n");
    }
    fclose(out_file);
}

int main(int argc, char **argv) {
    char* log_dir_path = argv[1];
    char global_metadata_path[256], local_metadata_path[256], logfile_path[256],  textfile_dir[256], textfile_path[256];

    RecorderGlobalDef RGD;
    RecorderLocalDef RLD;

    sprintf(textfile_dir, "%s/_text", log_dir_path);
    mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    sprintf(global_metadata_path, "%s/recorder.mt", log_dir_path);
    read_global_metadata(global_metadata_path, &RGD);

    int i;
    for(i = 0; i < RGD.total_ranks ; i++) {
        sprintf(local_metadata_path, "%s/%d.mt" , log_dir_path, i);
        sprintf(logfile_path, "%s/%d.itf" , log_dir_path, i);
        sprintf(textfile_path, "%s/%d.txt" , textfile_dir, i);

        read_local_metadata(local_metadata_path, &RLD);
        printf("Rank %d, Records: %d\n", i, RLD.total_records);

        Record *records = read_records(logfile_path, RLD.total_records, &RGD);
        decompress_records(records, RLD.total_records);

        write_to_textfile(textfile_path, records, RLD.total_records);
        free(records);
    }

    return 0;
}
