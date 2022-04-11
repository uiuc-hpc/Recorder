#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include "reader.h"


void print_cst(RecorderReader* reader, CST* cst) {

    for(int i = 0; i < cst->entries; i++) {
        Record* record = recorder_cs_to_record(&cst->cs_list[i]);

        const char* func_name = recorder_get_func_name(reader, record);
        printf("%s(", func_name);

        bool user_func = (recorder_get_func_type(reader, record) == RECORDER_USER_FUNCTION);
        for(int arg_id = 0; !user_func && arg_id < record->arg_count; arg_id++) {
            char *arg = record->args[arg_id];
            printf(" %s", arg);
        }

        printf(" ), rank: %d, count: %d\n", cst->cs_list[i].rank, cst->cs_list[i].count);
        recorder_free_record(record);
    }
}

void show_statistics(RecorderReader* reader, CST* cst) {
    int mpiio_count = 0, hdf5_count = 0, posix_count = 0;
    for(int i = 0; i < cst->entries; i++) {
        Record* record = recorder_cs_to_record(&cst->cs_list[i]);
        const char* func_name = recorder_get_func_name(reader, record);

        int type = recorder_get_func_type(reader, record);
        if(type == RECORDER_MPIIO)
            mpiio_count += cst->cs_list[i].count;
        if(type == RECORDER_HDF5)
            hdf5_count += cst->cs_list[i].count;
        if(type == RECORDER_POSIX)
            posix_count += cst->cs_list[i].count;

        recorder_free_record(record);
    }
    printf("HDF5: %d, MPI-IO Count: %d, POSIX: %d\n", hdf5_count, mpiio_count, posix_count);
}


int main(int argc, char **argv) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    CST cst;
    recorder_read_cst_merged(&reader, &cst);

    print_cst(&reader, &cst);
    show_statistics(&reader, &cst);

    recorder_free_cst(&cst);

    recorder_free_reader(&reader);

    return 0;
}
