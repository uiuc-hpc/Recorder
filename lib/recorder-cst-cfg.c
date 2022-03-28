#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "recorder.h"
#include "recorder-sequitur.h"


/**
 * Key: func_id + res + all arguments in string
 *
 * arguments seperated by space ' '
 */
char* compose_call_key(Record *record, int* key_len) {
    int arg_count = record->arg_count;
    char **args = record->args;

    char invalid_str[] = "???";
    int invalid_str_len = strlen(invalid_str);

    int arg_strlen = arg_count;
    for(int i = 0; i < arg_count; i++) {
        if(args[i]) {
            for(int j = 0; j < strlen(args[i]); j++)
                if(args[i][j] == ' ') args[i][j] = '_';
            arg_strlen += strlen(args[i]);
        } else {
            arg_strlen += strlen(invalid_str);
        }
    }

    // thread id, func id, level, arg count, arg strlen, arg str
    *key_len = sizeof(pthread_t) + sizeof(record->func_id) + sizeof(record->level) +
               sizeof(record->arg_count) + sizeof(int) + arg_strlen;

    char* key = recorder_malloc(*key_len);
    int pos = 0;
    memcpy(key+pos, &record->tid, sizeof(pthread_t));
    pos += sizeof(pthread_t);
    memcpy(key+pos, &record->func_id, sizeof(record->func_id));
    pos += sizeof(record->func_id);
    memcpy(key+pos, &record->level, sizeof(record->level));
    pos += sizeof(record->level);
    memcpy(key+pos, &record->arg_count, sizeof(record->arg_count));
    pos += sizeof(record->arg_count);
    memcpy(key+pos, &arg_strlen, sizeof(int));
    pos += sizeof(int);

    for(int i = 0; i < arg_count; i++) {
        if(args[i]) {
            memcpy(key+pos, args[i], strlen(args[i]));
            pos += strlen(args[i]);
        } else {
            memcpy(key+pos, invalid_str, strlen(invalid_str));
            pos += invalid_str_len;
        }
        key[pos] = ' ';
        pos += 1;
    }

    return key;
}


void cleanup_cst(RecordHash* cst) {
    RecordHash *entry, *tmp;
    HASH_ITER(hh, cst, entry, tmp) {
        HASH_DEL(cst, entry);
        recorder_free(entry->key, entry->key_len);
        recorder_free(entry, sizeof(RecordHash));
    }
    cst = NULL;
}

void* serialize_cst_local(RecordHash *table, size_t *len) {
    *len = sizeof(int);

    RecordHash *entry, *tmp;
    HASH_ITER(hh, table, entry, tmp) {
        *len = *len + entry->key_len + sizeof(int)*2;
    }

    int count = HASH_COUNT(table);
    void *res = recorder_malloc(*len);
    void *ptr = res;

    memcpy(ptr, &count, sizeof(int));
    ptr += sizeof(int);

    HASH_ITER(hh, table, entry, tmp) {

        memcpy(ptr, &entry->terminal_id, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, &entry->key_len, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, entry->key, entry->key_len);
        ptr = ptr + entry->key_len;
    }

    return res;
}

void* serialize_cst_merged(RecordHash *table, size_t *len) {
    *len = sizeof(int);

    RecordHash *entry, *tmp;
    HASH_ITER(hh, table, entry, tmp) {
        *len = *len + entry->key_len + sizeof(int)*3 + sizeof(unsigned);
    }

    int count = HASH_COUNT(table);
    void *res = recorder_malloc(*len);
    void *ptr = res;

    memcpy(ptr, &count, sizeof(int));
    ptr += sizeof(int);

    HASH_ITER(hh, table, entry, tmp) {

        memcpy(ptr, &entry->terminal_id, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, &entry->rank, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, &entry->key_len, sizeof(int));
        ptr = ptr + sizeof(int);

        memcpy(ptr, &entry->count, sizeof(unsigned));
        ptr = ptr + sizeof(unsigned);

        memcpy(ptr, entry->key, entry->key_len);
        ptr = ptr + entry->key_len;
    }

    return res;
}

RecordHash* deserialize_cst_merged(void *data) {
    int num;
    memcpy(&num, data, sizeof(int));

    void *ptr = data + sizeof(int);

    RecordHash *table = NULL, *entry = NULL;
    for(int i = 0; i < num; i++) {
        entry = recorder_malloc(sizeof(RecordHash));

        memcpy( &(entry->terminal_id), ptr, sizeof(int) );
        ptr += sizeof(int);

        memcpy( &(entry->rank), ptr, sizeof(int) );
        ptr += sizeof(int);

        memcpy( &(entry->key_len), ptr, sizeof(int) );
        ptr += sizeof(int);

        memcpy( &(entry->count), ptr, sizeof(unsigned) );
        ptr += sizeof(unsigned);

        entry->key = recorder_malloc(entry->key_len);
        memcpy( entry->key, ptr, entry->key_len );
        ptr += entry->key_len;

        HASH_ADD_KEYPTR(hh, table, entry->key, entry->key_len, entry);
    }

    return table;
}


void save_cst_local(RecorderLogger* logger) {
    FILE* f = RECORDER_REAL_CALL(fopen) (logger->cst_path, "wb");
    size_t len;
    void* data = serialize_cst_local(logger->cst, &len);
    RECORDER_REAL_CALL(fwrite)(data, 1, len, f);
    RECORDER_REAL_CALL(fflush)(f);
    RECORDER_REAL_CALL(fclose)(f);
}

RecordHash* copy_cst(RecordHash* origin) {
    RecordHash* table = NULL;
    RecordHash *entry, *tmp, *new_entry;
    HASH_ITER(hh, origin, entry, tmp) {
        new_entry = recorder_malloc(sizeof(RecordHash));
        new_entry->terminal_id = entry->terminal_id;
        new_entry->key_len = entry->key_len;
        new_entry->rank = entry->rank;
        new_entry->count = entry->count;
        new_entry->key = recorder_malloc(entry->key_len);
        memcpy(new_entry->key, entry->key, entry->key_len);
        HASH_ADD_KEYPTR(hh, table, new_entry->key, new_entry->key_len, new_entry);
    }
    return table;
}

RecordHash* compress_csts(RecorderLogger* logger) {
    int my_rank = logger->rank;
    int other_rank;
    int mask = 1;
    bool done = false;

    int phases = recorder_ceil(recorder_log2(logger->nprocs));

    RecordHash* merged_table = copy_cst(logger->cst);

    for(int k = 0; k < phases; k++, mask*=2) {
        if(done) break;

        other_rank = my_rank ^ mask;     // other_rank = my_rank XOR 2^k

        if(other_rank >= logger->nprocs) continue;

        size_t size;
        void* buf;

        // bigger ranks send to smaller ranks
        if(my_rank < other_rank) {
            RECORDER_REAL_CALL(PMPI_Recv)(&size, sizeof(size), MPI_BYTE, other_rank, mask, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            buf = recorder_malloc(size);
            RECORDER_REAL_CALL(PMPI_Recv)(buf, size, MPI_BYTE, other_rank, mask, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int cst_rank, entries, key_len;
            unsigned count;
            void *ptr = buf;
            memcpy(&entries, ptr, sizeof(int));
            ptr = ptr + sizeof(int);
            for(int i = 0; i < entries; i++) {
                // skip 4 bytes terminal id
                ptr = ptr + sizeof(int);

                // 4 bytes rank
                memcpy(&cst_rank, ptr, sizeof(int));
                ptr = ptr + sizeof(int);

                // 4 bytes key length
                memcpy(&key_len, ptr, sizeof(int));
                ptr = ptr + sizeof(int);

                // 4 bytes count
                memcpy(&count, ptr, sizeof(unsigned));
                ptr = ptr + sizeof(unsigned);

                // key length bytes key
                void *key = recorder_malloc(key_len);
                memcpy(key, ptr, key_len);
                ptr = ptr + key_len;

                // Check to see if this function entry is already in the table
                RecordHash *entry = NULL;
                HASH_FIND(hh, merged_table, key, key_len, entry);
                if(entry) {
                    recorder_free(key, key_len);
                    entry->count += count;
                } else {                                // Not exist, add to hash table
                    entry = (RecordHash*) recorder_malloc(sizeof(RecordHash));
                    entry->key = key;
                    entry->key_len = key_len;
                    entry->rank = cst_rank;
                    entry->count = count;
                    HASH_ADD_KEYPTR(hh, merged_table, key, key_len, entry);
                }
            }
            recorder_free(buf, size);

        } else {   // SENDER
            buf = serialize_cst_merged(merged_table, &size);
            RECORDER_REAL_CALL(PMPI_Send)(&size, sizeof(size), MPI_BYTE, other_rank, mask, MPI_COMM_WORLD);
            RECORDER_REAL_CALL(PMPI_Send)(buf, size, MPI_BYTE, other_rank, mask, MPI_COMM_WORLD);
            recorder_free(buf, size);
            done = true;
        }
    }

    // Eventually the root (rank 0) will get the fully merged CST
    // Update (re-assign) terminal id for all unique signatures
    if(my_rank == 0) {
        //linear_regression(merged_table);
        int terminal_id = 0;
        RecordHash *entry, *tmp;
        HASH_ITER(hh, merged_table, entry, tmp) {
            entry->terminal_id = terminal_id++;
        }
    } else {
        cleanup_cst(merged_table);
    }
    return merged_table;
}


void save_cst_merged(RecorderLogger* logger) {
    // 1. Inter-process copmression for CSTs
    // Eventually, rank 0 will have the compressed table.
    RecordHash* compressed_cst = compress_csts(logger);

    // 2. Broadcast the merged CST to all ranks
    size_t cst_stream_size;
    void *cst_stream;

    if(logger->rank == 0) {
        cst_stream = serialize_cst_merged(compressed_cst, &cst_stream_size);

        RECORDER_REAL_CALL(PMPI_Bcast)(&cst_stream_size, sizeof(cst_stream_size), MPI_BYTE, 0, MPI_COMM_WORLD);
        RECORDER_REAL_CALL(PMPI_Bcast)(cst_stream, cst_stream_size, MPI_BYTE, 0, MPI_COMM_WORLD);

        // 3. Rank 0 write out the compressed CST
        errno = 0;
        FILE *trace_file = fopen(logger->cst_path, "wb");
        if(trace_file) {
            fwrite(cst_stream, 1, cst_stream_size, trace_file);
            fclose(trace_file);
        } else {
            printf("[Recorder] Open file: %s failed, errno: %d\n", logger->cst_path, errno);
        }
    } else {
        RECORDER_REAL_CALL(PMPI_Bcast)(&cst_stream_size, sizeof(cst_stream_size), MPI_BYTE, 0, MPI_COMM_WORLD);
        cst_stream = recorder_malloc(cst_stream_size);
        RECORDER_REAL_CALL(PMPI_Bcast)(cst_stream, cst_stream_size, MPI_BYTE, 0, MPI_COMM_WORLD);

        // 3. Other rank get the compressed cst stream from rank 0
        // then convert it to the CST
        compressed_cst = deserialize_cst_merged(cst_stream);
    }

    // 4. Update function entry's terminal id
    int *update_terminal_id = recorder_malloc(sizeof(int) * logger->current_cfg_terminal);
    RecordHash *entry, *tmp, *res;
    HASH_ITER(hh, logger->cst, entry, tmp) {
        HASH_FIND(hh, compressed_cst, entry->key, entry->key_len, res);
        if(res)
            update_terminal_id[entry->terminal_id] = res->terminal_id;
        else
            printf("[pilgrim] %d Not possible! Not exist in merged table?\n", logger->rank);
    }


    cleanup_cst(compressed_cst);
    recorder_free(cst_stream, cst_stream_size);

    sequitur_update(&(logger->cfg), update_terminal_id);
    recorder_free(update_terminal_id, sizeof(int)* logger->current_cfg_terminal);
}


void save_cfg_local(RecorderLogger* logger) {
    FILE* f = RECORDER_REAL_CALL(fopen) (logger->cfg_path, "wb");
    int count;
    int* data = serialize_grammar(&logger->cfg, &count);
    RECORDER_REAL_CALL(fwrite)(data, sizeof(int), count, f);
    RECORDER_REAL_CALL(fflush)(f);
    RECORDER_REAL_CALL(fclose)(f);
}

void save_cfg_merged(RecorderLogger* logger) {
    sequitur_dump(logger->cfg_path, &logger->cfg, logger->rank, logger->nprocs);
}

