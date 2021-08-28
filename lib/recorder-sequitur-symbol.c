/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "recorder-sequitur.h"
#include "recorder-utils.h"


Symbol* new_symbol(int val, int exp, bool terminal, Symbol *rule_head) {
    Symbol* symbol = recorder_malloc(sizeof(Symbol));
    symbol->val = val;
    symbol->exp = exp;
    symbol->terminal = terminal;

    symbol->ref = 0;
    symbol->rule = NULL;
    symbol->rule_head = rule_head;
    symbol->rule_body = NULL;

    symbol->prev = NULL;
    symbol->next = NULL;
    return symbol;
}


/**
 * Insert a symbol after the pos in rule
 *
 * `rule` of terminals and non-terminals point to the rule it belongs to
 *
 * rule_head of an non-terminal points to its corresponding rule
 *          and the rule_head filed in this case will be set before
 *          calling this function
 */
void symbol_put(Symbol *rule, Symbol *pos, Symbol *sym) {
    if(!IS_RULE_HEAD(sym))
        sym->rule = rule;

    if(pos == NULL)     // insert as the head
        DL_PREPEND_ELEM(rule->rule_body, rule->rule_body, sym);
    else
        DL_APPEND_ELEM(rule->rule_body, pos, sym);

    if(IS_NONTERMINAL(sym))
        rule_ref(sym->rule_head);
}
void symbol_delete(Symbol *rule, Symbol *sym, bool deref) {
    if(IS_NONTERMINAL(sym) && deref)
        rule_deref(sym->rule_head);

    DL_DELETE(rule->rule_body, sym);
    recorder_free(sym, sizeof(Symbol));
    sym = NULL;
}


/**
 * New rule head symbol
 */
Symbol* new_rule(Grammar *grammar) {
    Symbol* rule = new_symbol(grammar->rule_id, 1, false, NULL);
    grammar->rule_id = grammar->rule_id - 1;
    return rule;
}

/**
 * Insert a rule into the rule list
 *
 */
void rule_put(Symbol **rules_head, Symbol *rule) {
    DL_APPEND(*rules_head, rule);
}

/**
 * Delete a rule from the list
 *
 */
void rule_delete(Symbol **rules_head, Symbol *rule) {
    DL_DELETE(*rules_head, rule);
    recorder_free(rule, sizeof(Symbol));
    rule = NULL;
}

void rule_ref(Symbol *rule) {
    rule->ref++;
}

void rule_deref(Symbol *rule) {
    rule->ref--;
}
