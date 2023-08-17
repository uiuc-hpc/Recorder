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
int* serialize_grammar(Grammar *grammar, int *integers) {

    int total_integers = 1, symbols_count = 0, rules_count = 0;

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

    *integers = total_integers;
    return data;
}


/**
 * Inter-process compression of CFGs
 *
 * Grammar* lg [in]: local grammar
 * return: a compressed grammar.
 */
Grammar* compress_grammars(Grammar *lg, int mpi_rank, int mpi_size, size_t *uncompressed_integers, int* num_unique_grammars, int* grammar_ids) {
    int integers = 0;
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

    if(mpi_rank !=0) return NULL;

    // Run a final Sequitur pass to compress the gathered grammars
    Grammar *grammar = recorder_malloc(sizeof(Grammar));
    grammar->start_rule_id = min_in_array(gathered_grammars, gathered_integers)  -1;
    sequitur_init_rule_id(grammar, grammar->start_rule_id, false);
    int rules, rule_val, symbols, symbol_val, symbol_exp;


    *uncompressed_integers = 0;

    // Go through each rank's grammar
    for(int i = 0; i < mpi_size; i++) {

        // Serialized grammar of rank i
        int* g = gathered_grammars + displs[i];
        int g_len = recvcounts[i] * sizeof(int);

        UniqueGrammar *entry = NULL;
        HASH_FIND(hh, unique_grammars, g, g_len, entry);

        if(entry) {
            // A duplicated grammar, only need to store its id
            entry->count++;
            grammar_ids[i] = entry->ugi;
        } else {
            printf("[Recorder] uniqune grammar, rank: %d\n", i);
            // An unseen grammar, fully store it.
            entry = recorder_malloc(sizeof(UniqueGrammar));
            entry->ugi = current_ugi++;
            entry->key = g;   // use the existing memory, do not copy it
            HASH_ADD_KEYPTR(hh, unique_grammars, entry->key, g_len, entry);
            grammar_ids[i] = entry->ugi;

            int k = 0;
            rules = g[k++];
            append_terminal(grammar, rules, 1);
            *uncompressed_integers += 2;

            for(int rule_idx = 0; rule_idx < rules; rule_idx++) {
                rule_val = g[k++];
                symbols = g[k++];
                append_terminal(grammar, rule_val, 1);
                append_terminal(grammar, symbols, 1);
                *uncompressed_integers += 4;
                printf("%d(%d)->", rule_val, symbols);
                for(int sym_id = 0; sym_id < symbols; sym_id++) {
                    symbol_val = g[k++];
                    symbol_exp = g[k++];
                    append_terminal(grammar, symbol_val, symbol_exp);
                    *uncompressed_integers += 2;
                    printf("%d^%d ", symbol_val, symbol_exp);
                }
                printf("\n");
            }
        }
    } // end of for loop

    // Clean up the hash table, and gathered grammars
    *num_unique_grammars = HASH_COUNT(unique_grammars);

    UniqueGrammar *ug, *tmp;
    HASH_ITER(hh, unique_grammars, ug, tmp) {
        HASH_DEL(unique_grammars, ug);
        recorder_free(ug, sizeof(UniqueGrammar));
    }
    recorder_free(gathered_grammars, gathered_integers*sizeof(int));

    return grammar;
}

void sequitur_save_unique_grammars(const char* path, Grammar* lg, int mpi_rank, int mpi_size) {
    int grammar_ids[mpi_size];
    int integers = 0;
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

            char ug_filename[1096] = {0};
            sprintf(ug_filename, "%s/%d.cfg", path, ug_entry->ugi);

            FILE* ug_file = fopen(ug_filename, "wb");
            fwrite(g, 1, g_len, ug_file);
            fflush(ug_file);
            fclose(ug_file);
        }
    }

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

    printf("[recorder] unique grammars: %d\n", num_unique_grammars);
}

// Return the size of compressed grammar in KB
double sequitur_dump(const char* path, Grammar *local_grammar, int mpi_rank, int mpi_size) {
    int compressed_integers = 0;

    // Compressed grammar is NULL except rank 0
    size_t uncompressed_integers = 0;
    int grammar_ids[mpi_size];
    int num_unique_grammars;
    Grammar *grammar = compress_grammars(local_grammar, mpi_rank, mpi_size, &uncompressed_integers, &num_unique_grammars, grammar_ids);

    // Serialize the compressed grammar and write it to file
    if(mpi_rank == 0) {

        int* compressed_grammar = serialize_grammar(grammar, &compressed_integers);

        errno = 0;
        FILE* f = fopen(path, "wb");
        if(f) {
            fwrite(grammar_ids, sizeof(int), mpi_size, f);
            fwrite(&num_unique_grammars, sizeof(int), 1, f);
            fwrite(&(grammar->start_rule_id), sizeof(int), 1, f);
            fwrite(&uncompressed_integers, sizeof(size_t), 1, f);
            fwrite(compressed_grammar, sizeof(int), compressed_integers, f);
            fclose(f);
        } else {
            printf("[recorder] Open file: %s failed, errno: %d!\n", path, errno);
        }

        sequitur_cleanup(grammar);
        recorder_free(grammar, sizeof(Grammar));
        recorder_free(compressed_grammar, compressed_integers*sizeof(int));

        printf("[recorder] unique grammars: %d, uncompressed integers: %ld, compressed integers: %d\n", num_unique_grammars, uncompressed_integers, compressed_integers);
    }

    return (compressed_integers/1024.0*sizeof(int));
}
