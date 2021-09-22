#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <mpi.h>
#include "reader.h"

#define DECIMAL 6

RecorderReader reader;
static char formating_string[32];

void write_to_textfile(Record *record, void* arg) {
    FILE* f = (FILE*) arg;

    bool user_func = (record->func_id == RECORDER_USER_FUNCTION);

    const char* func_name = recorder_get_func_name(&reader, record);

    fprintf(f, formating_string, record->tstart, record->tend, // record->tid
                             func_name, record->level, recorder_get_func_type(&reader, record));

    for(int arg_id = 0; !user_func && arg_id < record->arg_count; arg_id++) {
        char *arg = record->args[arg_id];
        fprintf(f, " %s", arg);
    }

    fprintf(f, " )\n");
}


int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

int main(int argc, char **argv) {

    char textfile_dir[256];
    char textfile_path[256];
    sprintf(textfile_dir, "%s/_text", argv[1]);

    int mpi_size, mpi_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if(mpi_rank == 0)
        mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    MPI_Barrier(MPI_COMM_WORLD);

    recorder_init_reader(argv[1], &reader);

    int decimal =  log10(1 / reader.metadata.time_resolution);
    sprintf(formating_string, "%%.%df %%.%df %%s %%d %%d (", decimal, decimal);

    // Each rank will process n files (n ranks traces)
    int n = max(reader.metadata.total_ranks/mpi_size, 1);
    int start_rank = n * mpi_rank;
    int end_rank   = min(reader.metadata.total_ranks, n*(mpi_rank+1));

    for(int rank = start_rank; rank < end_rank; rank++) {

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

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
