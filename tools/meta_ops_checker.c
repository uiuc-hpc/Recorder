#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"

#define max(a,b) (((a)>(b))?(a):(b))

Record callers[4];

// [func_id] = 3 by app
// [func_id] = 2 by hdf5
// [func_id] = 1 by mpi
// [func_id] = 0 not used
int meta_op_caller[256] = {0};


bool ignore_function(int func_id) {
    const char *func = func_list[func_id];
    if(strstr(func, "MPI") || strstr(func, "H5"))
        return true;
    if(strstr(func, "dir") || strstr(func, "link"))
        return false;

    const char* ignored[] = {"read", "write", "seek", "tell", "open", "close", "sync"};
    int i;
    for(i = 0; i < sizeof(ignored)/sizeof(char*); i++) {
        if(strstr(func, ignored[i]))
            return true;
    }

    return false;
}

void test(Record *records, int len, RecorderReader *reader) {
    int i;

    int depth = 0;
    callers[0].tend = 0;

    for(i = 0; i < len; i++) {
        Record record = records[i];

        while(depth > 0) {
            if(record.tstart >= callers[depth].tend)
                depth--;
            else
                break;
        }

        Record caller = callers[depth];
        callers[++depth]  = record;

        if(!ignore_function(record.func_id)) {
            if(depth > 1) {
                const char* caller_func = reader->func_list[caller.func_id];
                if(strstr(caller_func, "MPI"))
                    meta_op_caller[record.func_id] = max(meta_op_caller[record.func_id], 1);
                if(strstr(caller_func, "H5"))
                    meta_op_caller[record.func_id] = max(meta_op_caller[record.func_id], 2);
            } else {
                meta_op_caller[record.func_id] = 3;
            }

        }

    }
}

void print_metadata_ops(RecorderReader *reader) {
    printf("\n\n");
    const char* included_funcs[] = {
        "__xstat",      "__xstat64",    "__lxstat", "__lxstat64", "__fxstat",     "__fxstat64",
        "mmap",         "getcwd",       "mkdir",        "chdir",
        "umask",        "unlink",        "access",
        "ftruncate",    "fileno",       "faccessat"
    };
    int i;
    int func_id;

    for(i = 0; i < sizeof(included_funcs)/sizeof(char*); i++) {
        for(func_id = 0; func_id < 100; func_id++) {
            const char *f = reader->func_list[func_id];
            if(0 == strcmp(f, included_funcs[i])) {
                printf("%d, ", meta_op_caller[func_id]);
                break;
            }
        }
    }
    printf("\n");
}


int main(int argc, char **argv) {

    RecorderReader reader;
    recorder_read_traces(argv[1], &reader);

    int rank;
    for(rank = 0; rank < reader.RGD.total_ranks; rank++) {
        Record* records = reader.records[rank];
        test(records, reader.RLDs[rank].total_records, &reader);
    }

    int func_id;
    for(func_id = 0; func_id < 100; func_id++) {
        if(!ignore_function(func_id) && meta_op_caller[func_id])
            printf("func: %s, caller: %d\n", reader.func_list[func_id], meta_op_caller[func_id]);
    }
    print_metadata_ops(&reader);

    release_resources(&reader);
    return 0;
}
