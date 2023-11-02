/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "recorder-sequitur.h"
#include "recorder-utils.h"
#include "mpi.h"
#include "uthash.h"

typedef struct UniqueGrammar_t {
    int ugi;                // unique grammar id
    void *key;              // serialized grammar stream as key
    int count;
    UT_hash_handle hh;
} UniqueGrammar;

static UniqueGrammar *unique_grammars;
static int current_ugi = 0;

/**
 * Store the Grammer in an integer array
 *
 * | #rules |
 * | rule 1 head | #symbols of rule 1 | symbol 1, ..., symbol N |
 * | rule 2 head | #symbols of rule 2 | symbol 1, ..., symbol N |
 * ...
 *
 * @len: [out] the length of the array: 1 + 2 * number of rules + number of symbols
 * @return: return the array, need to be freed by the caller
 *
 */
int* serialize_grammar(Grammar *grammar, int* serialized_integers) {

    int total_integers = 1; // 0: number of rules
    int symbols_count  = 0, rules_count = 0;

    Symbol *rule, *sym;
    DL_COUNT(grammar->rules, rule, rules_count);

    total_integers += 2 * rules_count;

    DL_FOREACH(grammar->rules, rule) {
        DL_COUNT(rule->rule_body, sym, symbols_count);
        total_integers += symbols_count*2;  // val and exp
    }

    int i = 0;
    int *data = recorder_malloc(sizeof(int) * total_integers);
    data[i++]  = rules_count;
    DL_FOREACH(grammar->rules, rule) {
        DL_COUNT(rule->rule_body, sym, symbols_count);
        data[i++] = rule->val;
        data[i++] = symbols_count;

        DL_FOREACH(rule->rule_body, sym) {
            data[i++] = sym->val;       // rule id does not change
            data[i++] = sym->exp;
        }
    }

    *serialized_integers = total_integers;
    return data;
}

void sequitur_save_unique_grammars(const char* path, Grammar* lg, int mpi_rank, int mpi_size) {
    int grammar_ids[mpi_size];
    int integers;
    int *local_grammar = serialize_grammar(lg, &integers);

    int recvcounts[mpi_size], displs[mpi_size];
    PMPI_Gather(&integers, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    displs[0] = 0;
    size_t gathered_integers = recvcounts[0];
    for(int i = 1; i < mpi_size;i++) {
        gathered_integers += recvcounts[i];
        displs[i] = displs[i-1] + recvcounts[i-1];
    }

    int *gathered_grammars = NULL;
    if(mpi_rank == 0)
        gathered_grammars = recorder_malloc(sizeof(int) * gathered_integers);

    PMPI_Gatherv(local_grammar, integers, MPI_INT, gathered_grammars, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    recorder_free(local_grammar, sizeof(int)*integers);

    if(mpi_rank !=0) return;

    char ug_filename[1096] = {0};
    sprintf(ug_filename, "%s/ug.cfg", path);
    FILE* ug_file = fopen(ug_filename, "wb");

    // Go through each rank's grammar
    for(int rank = 0; rank < mpi_size; rank++) {

        // Serialized grammar
        int* g = gathered_grammars + displs[rank];
        int g_len = recvcounts[rank] * sizeof(int);
        //printf("rank: %d, grammar lengh: %d\n", rank, g_len);

        UniqueGrammar *ug_entry = NULL;
        HASH_FIND(hh, unique_grammars, g, g_len, ug_entry);

        if(ug_entry) {
            // A duplicated grammar, only need to store its id
            ug_entry->count++;
            grammar_ids[rank] = ug_entry->ugi;
        } else {
            ug_entry = recorder_malloc(sizeof(UniqueGrammar));
            ug_entry->ugi = current_ugi++;
            ug_entry->key = g;   // use the existing memory, do not copy it
            HASH_ADD_KEYPTR(hh, unique_grammars, ug_entry->key, g_len, ug_entry);
            grammar_ids[rank] = ug_entry->ugi;
            recorder_write_zlib((unsigned char*)g, g_len, ug_file);
        }
    }
    fclose(ug_file);

    // Clean up the hash table, and gathered grammars
    int num_unique_grammars = HASH_COUNT(unique_grammars);

    UniqueGrammar *ug, *tmp;
    HASH_ITER(hh, unique_grammars, ug, tmp) {
        HASH_DEL(unique_grammars, ug);
        recorder_free(ug, sizeof(UniqueGrammar));
    }

    char ug_metadata_fname[1096] = {0};
    sprintf(ug_metadata_fname, "%s/ug.mt", path);
    FILE* f = fopen(ug_metadata_fname, "wb");
    fwrite(grammar_ids, sizeof(int), mpi_size, f);
    fwrite(&num_unique_grammars, sizeof(int), 1, f);
    fflush(f);
    fclose(f);

    RECORDER_LOGINFO("[Recorder] unique grammars: %d\n", num_unique_grammars);
}
