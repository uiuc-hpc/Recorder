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
static char formatting_record[32];
static char formatting_fname[20];

int digits_count(int n) {
    int digits = 0;
    do {
        n /= 10;
        ++digits;
    } while (n != 0);
    return digits;
}

void write_to_textfile(Record *record, void* arg) {
    FILE* f = (FILE*) arg;

    bool user_func = (record->func_id == RECORDER_USER_FUNCTION);

    const char* func_name = recorder_get_func_name(&reader, record);

    fprintf(f, formatting_record, record->tstart, record->tend, // record->tid
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
    sprintf(formatting_record, "%%.%df %%.%df %%s %%d %%d (", decimal, decimal);
    sprintf(formatting_fname,  "%%s/%%0%dd.txt", digits_count(reader.metadata.total_ranks));

    // Each rank will process n files (n ranks traces)
    int n = max(reader.metadata.total_ranks/mpi_size, 1);
    int start_rank = n * mpi_rank;
    int end_rank   = min(reader.metadata.total_ranks, n*(mpi_rank+1));

    for(int rank = start_rank; rank < end_rank; rank++) {

        sprintf(textfile_path, formatting_fname, textfile_dir, rank);
        FILE* fout = fopen(textfile_path, "w");

        recorder_decode_records(&reader, rank, write_to_textfile, fout);

        fclose(fout);

        printf("\r[Recorder] rank %d finished\n", rank);
    }

    recorder_free_reader(&reader);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
