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

    char textfile_dir[256], textfile_path[256];
    sprintf(textfile_dir, "%s/_text", argv[1]);
    mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    RecorderReader reader;
    recorder_read_traces(argv[1], &reader);

    for(int rank = 0; rank < reader.RGD.total_ranks; rank++) {

        sprintf(textfile_path, "%s/%d.txt" , textfile_dir, rank);

        Record* records = reader.records[rank];
        write_to_textfile(textfile_path, records, reader.RLDs[rank].total_records);
    }

    release_resources(&reader);

    return 0;
}
