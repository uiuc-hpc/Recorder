/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mpi.h"
#include "recorder-sequitur.h"
#include "recorder-utils.h"


void delete_symbol(Symbol *sym) {
    symbol_delete(sym->rule, sym, true);
}


int check_digram(Grammar *grammar, Symbol *sym);

/**
 * Replace a digram by a rule (non-terminal)
 *
 * @delete_digram: we only put digram after we perform check_digram();
 * during the process of check_digram, we found a match and try to this==match
 * In this case, the matched digram is already in the digram table, however the
 * new digram has not been inserted into the table yet, so we don't delete becuase
 * other rules body may have the same key.
 *
 */
void replace_digram(Grammar *grammar, Symbol *origin, Symbol *rule, bool delete_digram) {
    if(!IS_RULE_HEAD(rule))
        ERROR_ABORT("replace_digram: not a rule head?\n");

    // Create an non-terminal
    Symbol *replaced = new_symbol(rule->val, 1, false, rule);

    // carefule here, if orgin is the first symbol, then
    // NULL will be used as the tail node.
    Symbol *prev = NULL;
    if(origin->rule->rule_body != origin)
        prev = origin->prev;
    if(prev != NULL)
        digram_delete(&(grammar->digram_table), prev);

    // delete digram before deleting symbols, otherwise we won't have correct digrams
    if(delete_digram) {
        digram_delete(&(grammar->digram_table), origin);
        digram_delete(&(grammar->digram_table), origin->next);
    }

    // delete symbol will set origin to NULL
    // so we need to store its rule and also delete origin->next first.
    Symbol *origin_rule = origin->rule;
    delete_symbol(origin->next);
    delete_symbol(origin);

    symbol_put(origin_rule, prev, replaced);


    // Add a new symbol (replaced) after prev
    // may introduce another repeated digram that we need to check
    if( check_digram(grammar, prev) == 0) {
        if(prev == NULL) {
            check_digram(grammar, replaced);
        } else {
            // it is possible that the 'replaced' symbol was deleted
            // by the check digram function due to twins-removal rule
            // if that's the case, we can not check the 'replaced'.
            if(prev->next==replaced)
                check_digram(grammar, replaced);
        }
    }
}

/**
 * Rule Utility
 * Replace a rule with its body if the rule is used only once
 *
 * @sym: is an non-terminal which should be replaced by sym->rule_head->rule_body
 */
void expand_instance(Grammar *grammar, Symbol *sym) {
    Symbol *rule = sym->rule_head;
    // just double check to make sure
    if(rule->ref != 1)
        ERROR_ABORT("Attempt to delete a rule that has multiple references!\n");

    digram_delete(&(grammar->digram_table), sym);

    int n = 0;
    Symbol *this, *tmp;
    Symbol *tail = sym;
    DL_FOREACH_SAFE(rule->rule_body, this,  tmp) {
        // delete the digram of the old rule (rule body)
        digram_delete(&(grammar->digram_table), this);

        Symbol *s = new_symbol(this->val, this->exp, this->terminal, this->rule_head);
        symbol_put(sym->rule, tail, s);
        tail = s;
        n++;

        // delete the symbol of the old rule (rule body)
        delete_symbol(this);
    }

    this = sym->next;
    for(int i = 0; i < n; i++) {
        digram_put(&(grammar->digram_table), this);
        this = this->next;
    }

    delete_symbol(sym);
    rule_delete(&(grammar->rules), rule);
}

/**
 * Handle the case in which a just-created digram matches
 * a previously existing one.
 *
 */
void process_match(Grammar *grammar, Symbol *this, Symbol *match) {
    Symbol *rule = NULL;

    // 1. The match consists of entire body of a rule
    // Then we replace the new digram with this rule
    if(match->prev == match->next) {
        rule = match->rule;
        replace_digram(grammar, this, match->rule, false);
    } else {
        // 2. Otherwise, we create a new rule and replace the repeated digrams with this rule
        rule = new_rule(grammar);
        symbol_put(rule, rule->rule_body, new_symbol(this->val, this->exp, this->terminal, this->rule_head));
        symbol_put(rule, rule->rule_body->prev, new_symbol(this->next->val, this->next->exp, this->next->terminal, this->next->rule_head));
        rule_put(&(grammar->rules), rule);

        replace_digram(grammar, match, rule, true);
        replace_digram(grammar, this, rule, false);

        // Insert the rule body into the digram table
        digram_put(&(grammar->digram_table), rule->rule_body);
    }


    // Check for "Rule Utility"
    // The first symbol of the just-created rule,
    // if is an non-terminal could be underutilized
    if(rule && rule->rule_body) {
        Symbol* tocheck = rule->rule_body->rule_head;
        if(tocheck && tocheck->ref < 2 && tocheck->exp < 2) {
            #ifdef DEBUG
                printf("rule utility:%d %d\n", tocheck->val, tocheck->ref);
            #endif
            expand_instance(grammar, rule->rule_body);
        }
    }

}

/**
 * Return 1 means the digram is replaced by a rule
 * (Either a new rule or an exisiting rule)
 */
int check_digram(Grammar *grammar, Symbol *sym) {

    if(sym == NULL || sym->next == NULL || sym->next == sym)
        return 0;

    // First of all, twins-removal rule.
    // Check if digram is of form a^i a^j
    // If so, represent it using a^(i+j)
    if(grammar->twins_removal && sym->val == sym->next->val) {
        digram_delete(&(grammar->digram_table), sym->prev);
        sym->exp = sym->exp + sym->next->exp;
        //delete_symbol(sym->next);
        symbol_delete(sym->next->rule, sym->next, false);
        return check_digram(grammar, sym->prev);
    }


    Symbol *match = digram_get(grammar->digram_table, sym, sym->next);

    if(match == NULL) {
        // Case 1. new digram, put it in the table
        #ifdef DEBUG
            printf("new digram %d %d\n", sym->val, sym->next->val);
        #endif
        digram_put(&(grammar->digram_table), sym);
        return 0;
    }

    if(match->next == sym) {
        // Case 2. match found but overlap: do nothing
        #ifdef DEBUG
            printf("found digram but overlap\n");
        #endif
        return 0;
    } else {
        // Case 3. non-overlapping match found
        #ifdef DEBUG
            printf("found non-overlapping digram %d %d\n", sym->val, sym->next->val);
        #endif
        process_match(grammar, sym, match);
        return 1;
    }

}

Symbol* append_terminal(Grammar* grammar, int val, int exp) {

    Symbol *sym = new_symbol(val, exp, true, NULL);

    Symbol *main_rule = grammar->rules;
    Symbol *tail;

    if(main_rule->rule_body)
        tail = main_rule->rule_body->prev;  // Get the last symbol
    else
        tail = main_rule->rule_body;        // NULL, no symbol yet

    symbol_put(main_rule, tail, sym);
    check_digram(grammar, sym->prev);

    return sym;
}

void sequitur_cleanup(Grammar *grammar) {
    Digram *digram, *tmp;
    HASH_ITER(hh, grammar->digram_table, digram, tmp) {
        HASH_DEL(grammar->digram_table, digram);
        recorder_free(digram->key, DIGRAM_KEY_LEN);
        recorder_free(digram, sizeof(Digram));
    }

    Symbol *rule, *sym, *tmp2, *tmp3;
    DL_FOREACH_SAFE(grammar->rules, rule, tmp2) {
        DL_FOREACH_SAFE(rule->rule_body, sym, tmp3) {
            DL_DELETE(rule->rule_body, sym);
            recorder_free(sym, sizeof(Symbol));
        }
        DL_DELETE(grammar->rules, rule);
        recorder_free(rule, sizeof(Symbol));
    }

    grammar->digram_table = NULL;
    grammar->rules = NULL;
    grammar->rule_id = -1;
}

void sequitur_init_rule_id(Grammar *grammar, int start_rule_id, bool twins_removal) {
    grammar->digram_table = NULL;
    grammar->rules = NULL;
    grammar->rule_id = start_rule_id;
    grammar->twins_removal = twins_removal;


    // Add the main rule: S, which will be the head of the rule list
    rule_put(&(grammar->rules), new_rule(grammar));
}

void sequitur_init(Grammar *grammar) {
    sequitur_init_rule_id(grammar, -1, true);
}

void sequitur_update(Grammar *grammar, int *update_terminal_id) {
    Symbol* rule, *sym;
    DL_FOREACH(grammar->rules, rule) {
        DL_FOREACH(rule->rule_body, sym) {
            if(sym->val >= 0)
                sym->val = update_terminal_id[sym->val];
        }
    }
}
