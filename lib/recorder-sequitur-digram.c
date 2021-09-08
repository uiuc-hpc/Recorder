/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "recorder-sequitur.h"
#include "recorder-utils.h"


void* build_digram_key(int v1, int exp1, int v2, int exp2) {
    void *key = recorder_malloc(DIGRAM_KEY_LEN);
    memcpy(key, &v1, sizeof(int));
    memcpy(key+sizeof(int), &exp1, sizeof(int));
    memcpy(key+sizeof(int)*2, &v2, sizeof(int));
    memcpy(key+sizeof(int)*3, &exp2, sizeof(int));
    return key;
}


/**
 * Look up a digram in the hash table
 *
 * @param v1 The symbol value of the first symbol of the digram
 * @param v2 The symbol value of the second symbol of the digram
 */
Symbol* digram_get(Digram *digram_table, Symbol* sym1, Symbol* sym2) {

    void* key = build_digram_key(sym1->val, sym1->exp, sym2->val, sym2->exp);

    Digram *found;
    HASH_FIND(hh, digram_table, key, DIGRAM_KEY_LEN, found);
    recorder_free(key, DIGRAM_KEY_LEN);

    if(found) {
        return found->symbol;
    }
    return NULL;
}

/**
 * Insert a digram into the hash table
 *
 * @param symbol The first symbol of the digram
 *
 */
int digram_put(Digram **digram_table, Symbol *symbol) {
    if (symbol == NULL || symbol->next == NULL)
        return -1;

    void* key = build_digram_key(symbol->val, symbol->exp, symbol->next->val, symbol->next->exp);

    Digram *found;
    HASH_FIND(hh, *digram_table, key, DIGRAM_KEY_LEN, found);

    // Found the same digram in the table already
    if(found) {
        recorder_free(key, DIGRAM_KEY_LEN);
        return 1;
    } else {
        Digram *digram = recorder_malloc(sizeof(Digram));
        digram->key = key;
        digram->symbol = symbol;
        HASH_ADD_KEYPTR(hh, *digram_table, digram->key, DIGRAM_KEY_LEN, digram);
        return 0;
    }
}


int digram_delete(Digram **digram_table, Symbol *symbol) {
    if(symbol == NULL || symbol->next == NULL)
        return 0;

    void* key = build_digram_key(symbol->val, symbol->exp, symbol->next->val, symbol->next->exp);

    Digram *found;
    HASH_FIND(hh, *digram_table, key, DIGRAM_KEY_LEN, found);
    recorder_free(key, DIGRAM_KEY_LEN);

    // 1 1 1, this sequence only has one digram (1, 1) points to the first 1.
    // if somehow digram_delete is called on the 2nd 1, we should not delete the
    // digram. This can happen for this sequence 1 1 1 2 1 2
    if(found && found->symbol == symbol) {
        HASH_DELETE(hh, *digram_table, found);
        recorder_free(found->key, DIGRAM_KEY_LEN);
        recorder_free(found, sizeof(Digram));
        return 0;
    }

    return -1;
}

