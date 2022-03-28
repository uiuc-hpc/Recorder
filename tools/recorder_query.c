#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include "reader.h"


void print_cst(RecorderReader* reader, CST *cst) {

    for(int i = 0; i < cst->entries; i++) {
        Record record;
        recorder_cs_to_record(&cst->cs_list[i], &record);


        const char* func_name = recorder_get_func_name(reader, &record);
        printf("%s(", func_name);

        bool user_func = (record.func_id == RECORDER_USER_FUNCTION);
        for(int arg_id = 0; !user_func && arg_id < record.arg_count; arg_id++) {
            char *arg = record.args[arg_id];
            printf(" %s", arg);
        }

        printf(" )\n");
        free(record.args);
    }
}


int main(int argc, char **argv) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    CST cst;
    recorder_read_cst_merged(&reader, &cst);

    print_cst(&reader, &cst);

    recorder_free_cst(&cst);

    recorder_free_reader(&reader);

    return 0;
}
