#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"

RecorderReader reader;

void write_to_textfile(Record *record, void* arg) {
    FILE* f = (FILE*) arg;

    bool user_func = (record->func_id == RECORDER_USER_FUNCTION);

    const char* func_name = recorder_get_func_name(&reader, record->func_id);
    if(user_func)
        func_name = record->args[0];

    fprintf(f, "%.6f %.6f %s (", record->tstart, record->tend, // record->tid
                             func_name);

    for(int arg_id = 0; !user_func && arg_id < record->arg_count; arg_id++) {
        char *arg = record->args[arg_id];
        fprintf(f, " %s", arg);
    }

    fprintf(f, " )\n");
}

int main(int argc, char **argv) {

    char textfile_dir[256];
    char textfile_path[256];
    sprintf(textfile_dir, "%s/_text", argv[1]);
    //sprintf(textfile_dir, "./_text");
    mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    recorder_init_reader(argv[1], &reader);

    for(int rank = 0; rank < reader.metadata.total_ranks; rank++) {
        CST cst;
        CFG cfg;
        recorder_read_cst(&reader, rank, &cst);
        recorder_read_cfg(&reader, rank, &cfg);

        sprintf(textfile_path, "%s/%d.txt", textfile_dir, rank);
        FILE* fout = fopen(textfile_path, "w");

        recorder_decode_records(&reader, &cst, &cfg, write_to_textfile, fout);

        fclose(fout);

        printf("\r[Recorder] rank %d, unique call signatures: %d\n", rank, cst.entries);
        recorder_free_cst(&cst);
        recorder_free_cfg(&cfg);
    }

    recorder_free_reader(&reader);

    return 0;
}
