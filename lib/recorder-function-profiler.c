#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include "recorder.h"
#include "utlist.h"
#include "uthash.h"


typedef struct tstart_node {
    double tstart;
    struct tstart_node *next;
} tstart_node_t;

typedef struct func_hash {
    int key_len;
    void *key;          // thread id + func addr as key
    tstart_node_t *tstart_head;
    UT_hash_handle hh;
} func_hash_t;


func_hash_t* func_table;


void* compose_func_hash_key(void* func, int *key_len) {
    pthread_t tid = pthread_self();
    *key_len = sizeof(pthread_t) + sizeof(void*);
    void* key = recorder_malloc(*key_len);
    memcpy(key, &tid, sizeof(pthread_t));
    memcpy(key+sizeof(pthread_t), &func, sizeof(void*));
    return key;
}


void __cyg_profile_func_enter(void *func, void *caller)
                              __attribute__((no_instrument_function));

void __cyg_profile_func_enter (void *func,  void *caller)
{
    if(!logger_initialized()) return;

    Dl_info info;
    if(!dladdr(func, &info)) return;
    if(!info.dli_sname && !info.dli_fname) return;

    //printf("enter %s %s\n", info.dli_fname, info.dli_sname);

    func_hash_t *entry = NULL;
    int key_len;
    void* key = compose_func_hash_key(func, &key_len);
    HASH_FIND(hh, func_table, key, key_len, entry);
    if(entry) {
        recorder_free(key, key_len);
    } else {
        entry = recorder_malloc(sizeof(func_hash_t));
        entry->key = key;
        entry->key_len = key_len;
        entry->tstart_head = NULL;
        HASH_ADD_KEYPTR(hh, func_table, entry->key, entry->key_len, entry);
    }

    tstart_node_t *node = recorder_malloc(sizeof(tstart_node_t));
    node->tstart = recorder_wtime();
    LL_PREPEND(entry->tstart_head, node);
}


void __cyg_profile_func_exit(void *func, void *caller)
                             __attribute__((no_instrument_function));


void __cyg_profile_func_exit (void *func,  void *caller)
{
    if(!logger_initialized()) return;

    Dl_info info;
    if(!dladdr(func, &info)) return;
    if(!info.dli_sname && !info.dli_fname) return;

    func_hash_t *entry = NULL;
    int key_len;
    void* key = compose_func_hash_key(func, &key_len);
    HASH_FIND(hh, func_table, key, key_len, entry);
    recorder_free(key, key_len);

    if(entry) {
        Record *record = recorder_malloc(sizeof(Record));
        record->func_id = RECORDER_USER_FUNCTION;
        record->level = 0;
        record->tid = pthread_self();
        record->tstart = entry->tstart_head->tstart;
        record->tend = recorder_wtime();
        record->arg_count = 2;
        record->args = recorder_malloc(record->arg_count*sizeof(char*));
        record->args[0] = strdup(info.dli_fname?info.dli_fname:"???");
        record->args[1] = strdup(info.dli_sname?info.dli_sname:"???");

        LL_DELETE(entry->tstart_head, entry->tstart_head);
        write_record(record);

        if(entry->tstart_head == NULL) {
            HASH_DEL(func_table, entry);
            recorder_free(entry->key, entry->key_len);
            recorder_free(entry, sizeof(func_hash_t));
        }

        free_record(record);
    } else {
        // Shouldn't be possible
        printf("Not possible!\n");
    }
}

