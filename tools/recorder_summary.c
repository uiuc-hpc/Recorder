#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include "reader.h"



void print_cst(RecorderReader* reader, CST* cst) {
    printf("\nBelow are the unique call signatures: \n");

    for(int i = 0; i < cst->entries; i++) {
        Record* record = recorder_cs_to_record(&cst->cs_list[i]);

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

void show_statistics(RecorderReader* reader, CST* cst) {

    int unique_signature[256] = {0};
    int call_count[256] = {0};
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

        unique_signature[record->func_id]++;
        call_count[record->func_id] += cst->cs_list[i].count;

        recorder_free_record(record);
    }
    long int total = hdf5_count + mpiio_count + posix_count;
    printf("Total: %ld\nHDF5: %d\nMPI-IO Count: %d\nPOSIX: %d\n", total, hdf5_count, mpiio_count, posix_count);

    printf("\n%-25s %18s %18s\n", "Func", "Unique Signature", "Total Call Count");
    for(int i = 0; i < 256; i++) {
        if(unique_signature[i] > 0) {
            printf("%-25s %18d %18d\n", func_list[i], unique_signature[i], call_count[i]);
        }
    }

}


int main(int argc, char **argv) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    CST* cst;
    CFG* cfg;
    recorder_get_cst_cfg(&reader, 0, &cst, &cfg);

    show_statistics(&reader, cst);

    print_cst(&reader, cst);

    recorder_free_reader(&reader);

    return 0;
}
