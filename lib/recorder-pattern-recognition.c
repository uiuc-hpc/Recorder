#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include "recorder.h"

struct offset_cs_entry {
    int offset_key_start;
    int offset_key_end;
    CallSignature* cs;
};

int count_function(RecorderLogger *logger, unsigned char filter_func_id) {
    int func_count = 0;

    CallSignature *entry, *tmp;
    HASH_ITER(hh, logger->cst, entry, tmp) {
        void* ptr = entry->key+sizeof(pthread_t);
        unsigned char func_id;
        memcpy(&func_id, ptr, sizeof(func_id));
        if(func_id == filter_func_id)
            func_count++;
    }

    return func_count;
}

void interprocess_pattern_recognition(RecorderLogger *logger, char* func_name, int offset_arg_idx) {

    unsigned char filter_func_id = get_function_id_by_name(func_name);
    int func_count = count_function(logger, filter_func_id);

    struct offset_cs_entry *offset_cs_entries = malloc(sizeof(struct offset_cs_entry) * func_count);
    long int *offsets = malloc(sizeof(long int) * func_count);

    int args_start = cs_key_args_start();

    int idx = 0;
    CallSignature* entry;
    CallSignature* tmp;
    HASH_ITER(hh, logger->cst, entry, tmp) {
        void* ptr = entry->key+sizeof(pthread_t);

        unsigned char func_id;
        memcpy(&func_id, ptr, sizeof(unsigned char));

        if(func_id == filter_func_id) {

            char* key = (char*) entry->key;

            int arg_idx = 0;
            int start = args_start, end = args_start;

            for(int i = args_start; i < entry->key_len; i++) {
                if(key[i] == ' ') {
                    if(arg_idx == offset_arg_idx) {
                        end = i;
                        break;
                    } else {
                        arg_idx++;
                        start = i;
                    }
                }
            }

            assert(end > start);
            char offset_str[64] = {0};
            memcpy(offset_str, key+start+1, end-start-1);
            long int offset = atol(offset_str);

            offsets[idx] = offset;
            offset_cs_entries[idx].offset_key_start = start;
            offset_cs_entries[idx].offset_key_end   = end;
            offset_cs_entries[idx].cs = entry;

            idx++;
        }
    }

    MAP_OR_FAIL(PMPI_Comm_split);
    MAP_OR_FAIL(PMPI_Comm_size);
    MAP_OR_FAIL(PMPI_Comm_rank);
    MAP_OR_FAIL(PMPI_Comm_free);
    MAP_OR_FAIL(PMPI_Allgather);

    MPI_Comm comm;
    int comm_size, comm_rank;
    RECORDER_REAL_CALL(PMPI_Comm_split)(MPI_COMM_WORLD, func_count, logger->rank, &comm);
    RECORDER_REAL_CALL(PMPI_Comm_size)(comm, &comm_size);
    RECORDER_REAL_CALL(PMPI_Comm_rank)(comm, &comm_rank);

    //if(comm_rank == 0)
    //    printf("%s count: %d, comm size: %d\n", func_name, func_count, comm_size);

    if(comm_size > 2) {
        long int *all_offsets = calloc(comm_size*(func_count), sizeof(long int));
        RECORDER_REAL_CALL(PMPI_Allgather)(offsets, func_count, MPI_LONG,
                                            all_offsets, func_count, MPI_LONG, comm);

        // Fory every offset of the same I/O call
        // check if it is the form of offset = a * rank + b;
        for(int i = 0; i < func_count; i++) {

            long int o1 = all_offsets[i];
            long int o2 = all_offsets[i+func_count];
            long int a = o2 - o1;
            long int b = o1;
            int same_pattern = 1;
            for(int r = 0; r < comm_size; r++) {
                long int o = all_offsets[i+func_count*r];
                if(o != a*r+b) {
                    same_pattern = 0;
                    break;
                }
            }

            // Everyone has the same pattern of offset
            // Then modify the call signature to store
            // the pattern instead of the actuall offset
            // TODO we should store a and b, but now we
            // store a only
            if(same_pattern) {
                HASH_DEL(logger->cst, offset_cs_entries[i].cs);

                int start = offset_cs_entries[i].offset_key_start;
                int end   = offset_cs_entries[i].offset_key_end;

                char* tmp = calloc(64, 1);
                sprintf(tmp, "%ld*r+%ld", a, b);

                //if(comm_rank == 0)
                //    printf("pattern recognized %d: offset = %ld*rank+%ld\n", offset_cs_entries[i].cs->terminal_id, a, b);

                int old_keylen = offset_cs_entries[i].cs->key_len;
                int new_keylen = old_keylen - (end-start-1) + strlen(tmp);
                int new_arg_strlen = new_keylen - args_start;

                void* newkey = malloc(new_keylen);
                void* oldkey = offset_cs_entries[i].cs->key;

                memcpy(newkey, oldkey, start+1);
                memcpy(newkey+args_start-sizeof(int), &new_arg_strlen, sizeof(int));
                memcpy(newkey+start+1, tmp, strlen(tmp));
                memcpy(newkey+start+1+strlen(tmp), oldkey+end, old_keylen-end);

                offset_cs_entries[i].cs->key = newkey;
                offset_cs_entries[i].cs->key_len = new_keylen;
                HASH_ADD_KEYPTR(hh, logger->cst, offset_cs_entries[i].cs->key, offset_cs_entries[i].cs->key_len, offset_cs_entries[i].cs);


                free(oldkey);
                free(tmp);
            }
        }
        free(all_offsets);
    }

    RECORDER_REAL_CALL(PMPI_Comm_free)(&comm);
    free(offsets);
    free(offset_cs_entries);
}
