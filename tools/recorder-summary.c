#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#include "reader.h"
#include "reader-private.h"
#include "recorder-logger.h"



void print_cst(RecorderReader* reader, CST* cst) {
    printf("\nBelow are the unique call signatures: \n");

    for(int i = 0; i < cst->entries; i++) {
        Record* record = reader_cs_to_record(&cst->cs_list[i]);

        const char* func_name = recorder_get_func_name(reader, record);
        printf("%s(", func_name);

        bool user_func = (recorder_get_func_type(reader, record) == RECORDER_USER_FUNCTION);
        for(int arg_id = 0; !user_func && arg_id < record->arg_count; arg_id++) {
            char *arg = record->args[arg_id];
            printf(" %s", arg);
        }

        printf(" ), count: %d\n", cst->cs_list[i].count);
        recorder_free_record(record);
    }
}

void print_statistics(RecorderReader* reader, CST* cst) {

    int unique_signature[256] = {0};
    int call_count[256] = {0};
    int mpi_count = 0, mpiio_count = 0, hdf5_count = 0, posix_count = 0;

    for(int i = 0; i < cst->entries; i++) {
        Record* record = reader_cs_to_record(&cst->cs_list[i]);
        const char* func_name = recorder_get_func_name(reader, record);

        int type = recorder_get_func_type(reader, record);
        if(type == RECORDER_MPI)
            mpi_count += cst->cs_list[i].count;
        if(type == RECORDER_MPIIO)
            mpiio_count += cst->cs_list[i].count;
        if(type == RECORDER_HDF5)
            hdf5_count += cst->cs_list[i].count;
        if(type == RECORDER_POSIX)
            posix_count += cst->cs_list[i].count;

        unique_signature[record->func_id]++;
        call_count[record->func_id] += cst->cs_list[i].count;

        recorder_free_record(record);
    }
    long int total = posix_count + mpi_count + mpiio_count + hdf5_count;
    printf("Total: %ld\nPOSIX: %d\nMPI: %d\nMPI-IO: %d\nHDF5: %d\n",
           total, posix_count, mpi_count, mpiio_count, hdf5_count);

    printf("\n%-25s %18s %18s\n", "Func", "Unique Signature", "Total Call Count");
    for(int i = 0; i < 256; i++) {
        if(unique_signature[i] > 0) {
            printf("%-25s %18d %18d\n", func_list[i], unique_signature[i], call_count[i]);
        }
    }
}

void print_metadata(RecorderReader* reader) {
    RecorderMetadata* meta =  &(reader->metadata);

    time_t t = meta->start_ts;
    struct tm *lt = localtime(&t);
    char tmbuf[64];
    strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", lt);

    printf("========Recorder Tracing Parameters========\n");
    printf("Tracing start time: %s\n", tmbuf);
    printf("Total processes: %d\n", meta->total_ranks);
    printf("Timestamp compression: %s\n", meta->ts_compression?"True":"False");
    printf("Interprocess compression: %s\n", meta->interprocess_compression?"True":"False");
    printf("Intraprocess pattern recognition: %s\n", meta->intraprocess_pattern_recognition?"True":"False");
    printf("Interprocess pattern recognition: %s\n", meta->interprocess_pattern_recognition?"True":"False");
    printf("===========================================\n\n");
}


int main(int argc, char **argv) {
    bool show_cst = false;
    int opt;
    while ((opt = getopt(argc, argv, "a")) != -1) {
        switch(opt) {
            case 'a':
                show_cst = true;
                break;
            defaut:
                fprintf(stderr, "Usage: %s [-a] [path to traces]\n", argv[0]);
        }
    }

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);
    CST* cst = reader_get_cst(&reader, 0);
    print_metadata(&reader);
    print_statistics(&reader, cst);

    if (show_cst) {
        print_cst(&reader, cst);
    }

    recorder_free_reader(&reader);

    return 0;
}
