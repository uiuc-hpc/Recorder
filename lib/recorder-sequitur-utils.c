/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "recorder-sequitur.h"

void sequitur_print_digrams(Grammar *grammar) {
    Digram *digram, *tmp;

    printf("digrams count: %d\n", HASH_COUNT(grammar->digram_table));
    HASH_ITER(hh, grammar->digram_table, digram, tmp) {
        int v1, v2;
        memcpy(&v1, digram->key, sizeof(int));
        memcpy(&v2, digram->key+sizeof(int)*2, sizeof(int));

        if(digram->symbol->rule)
            printf("digram(%d, %d, rule:%d): %d %d\n", v1, v2, digram->symbol->rule->val, digram->symbol->val, digram->symbol->next->val);
        else
            printf("digram(%d, %d, rule:): %d %d\n", v1, v2, digram->symbol->val, digram->symbol->next->val);
    }
}

void sequitur_print_rules(Grammar *grammar) {
    Symbol *rule, *sym;
    int rules_count = 0, symbols_count = 0;
    DL_COUNT(grammar->rules, rule, rules_count);

    DL_FOREACH(grammar->rules, rule) {
        int count;
        DL_COUNT(rule->rule_body, sym, count);
        symbols_count += count;

        //#ifdef DEBUG
        printf("Rule %d :-> ", rule->val);

        DL_FOREACH(rule->rule_body, sym) {
            if(sym->exp > 1)
                printf("%d^%d ", sym->val, sym->exp);
            else
                printf("%d ", sym->val);
        }
        printf("\n");
        //#endif
    }

    /*
    printf("\n=======================\nNumber of rule: %d\n", rules_count);
    printf("Number of symbols: %d\n", symbols_count);
    printf("Number of Digrams: %d\n=======================\n", HASH_COUNT(grammar.digram_table));
    */
    printf("[recorder] Rules: %d, Symbols: %d\n", rules_count, symbols_count);
}
