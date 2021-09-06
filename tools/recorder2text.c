#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"

RecorderReader reader;

void write_to_textfile(Record *record, void* arg) {
    FILE* f = (FILE*) arg;

    fprintf(f, "%f %f %d %s (", record->tstart, record->tend, record->res,
                                        recorder_get_func_name(&reader, record->func_id));
    for(int arg_id = 0; arg_id < record->arg_count; arg_id++) {
        char *arg = record->args[arg_id];
        fprintf(f, " %s", arg);
    }

    fprintf(f, " )\n");
}

int main(int argc, char **argv) {

    char textfile_dir[256];
    char textfile_path[256];
    sprintf(textfile_dir, "%s/_text", argv[1]);
    mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    recorder_init_reader(argv[1], &reader);

    for(int rank = 0; rank < reader.total_ranks; rank++) {
        CST cst;
        CFG cfg;
        recorder_read_cst(&reader, rank, &cst);
        recorder_read_cfg(&reader, rank, &cfg);

        sprintf(textfile_path, "%s/%d.txt", textfile_dir, rank);
        FILE* fout = fopen(textfile_path, "w");

        recorder_decode_records(&reader, &cst, &cfg, write_to_textfile, fout);

        fclose(fout);

        recorder_free_cst(&cst);
        recorder_free_cfg(&cfg);
    }

    recorder_free_reader(&reader);

    return 0;
}
