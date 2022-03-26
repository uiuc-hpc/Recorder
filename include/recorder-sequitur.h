/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef _RECORDER_SEQUITUR_H_
#define _RECORDER_SEQUITUR_H_

#include <stdbool.h>
#include "utlist.h"
#include "uthash.h"

#define IS_RULE_HEAD(sym) ((!(sym)->terminal) && ((sym)->rule_head==NULL))
#define IS_NONTERMINAL(sym) ((!(sym)->terminal) && ((sym)->rule_head!=NULL))
#define IS_TERMINAL(sym) ((sym)->terminal)

#define ERROR_ABORT(msg) {fprintf(stderr, msg);abort();}

//#define DEBUG



/**
 * There are three types of Symbols
 *
 * 1. Terminal:
 *      `rule` filed is the rule (rule head) it blongs to
 *      `rule_head`, `ref_body` and `ref` are ignored
 *
 * 2. Non-terminal:
 *      `rule` filed is the rule (rule head) it blongs to
 *      `rule_head` points to the rule_head node
 *      `ref_body` and `ref` are ignored
 *
 *  Terminals and Non-terminals are both stored in rule_body list.
 *
 * 3. Rule head:
 *      Represents a rule.  This is a special type that are stored in the rule list
 *      It will never be inserted into the rules body.
 *      `rule_body` is the right hand side
 *      `ref` is the number of usages
 *      `rule` and `rule_head` filed are ignored
 *
 */
typedef struct Symbol_t {           // utlist node, sizeof(Symbol) = 56
    int val;
    int exp;
    bool terminal;

    // For terminal and non-termial this field
    // remembers the rule (Symbol of Rule Head type) they belong to
    struct Symbol_t *rule;

    // Only used by non-terminals, points to the rule (Symbol of Rule Head type) it represents
    struct Symbol_t *rule_head;

    // if this is a rule (Rule Head type)
    // rule_body will be a list of symbols this rule represent
    // ref will be the number of usages of this rule
    struct Symbol_t *rule_body;
    int ref;

    struct Symbol_t *prev, *next;
} Symbol;


typedef struct Digram_t {           // uthash node, sizesof(Digram) = 72
    void *key;                      // the key is composed of two symbol values (sym->val)
    Symbol *symbol;                 // first symbol of the digram
    UT_hash_handle hh;
} Digram;

typedef struct Grammar_t {
    Symbol *rules;
    Digram *digram_table;
    int start_rule_id;              // first rule id, normally is -1
    int rule_id;                    // current_rule id, a negative number start from 'start_rule_id'
    bool twins_removal;             // if or not we will apply the twins-removal rule
} Grammar;


/* Only these five functions should be exposed
 * to the recorder looger code.
 * Alls the rest are used internally for the Sequitur
 * algorithm implementation.
 */
Symbol* append_terminal(Grammar *grammar, int val, int exp);
void sequitur_init(Grammar *grammar);
void sequitur_init_rule_id(Grammar *grammar, int start_rule_id, bool twins_removal);
void sequitur_update(Grammar *grammar, int *update_terminal_id);
void sequitur_cleanup(Grammar *grammar);


/* recorder_sequitur_symbol.c */
Symbol* new_symbol(int val, int exp, bool terminal, Symbol* rule_head);
void symbol_put(Symbol *rule, Symbol *pos, Symbol *sym);
void symbol_delete(Symbol *rule, Symbol *sym, bool deref);

Symbol* new_rule(Grammar *grammar);
void rule_put(Symbol **rules_head, Symbol *rule);
void rule_delete(Symbol **rules_head, Symbol *rule);
void rule_ref(Symbol *rule);
void rule_deref(Symbol *rule);



/* recorder_sequitur_digram.c */
#define DIGRAM_KEY_LEN sizeof(int)*4
Symbol* digram_get(Digram *digram_table, Symbol* sym1, Symbol* sym2);
int digram_put(Digram **digram_table, Symbol *symbol);
int digram_delete(Digram **digram_table, Symbol *symbol);


/* recorder_sequitur_logger.c */
int* serialize_grammar(Grammar *grammar, int *integers);
double sequitur_dump(const char *path, Grammar *grammar, int mpi_rank, int mpi_size);

/* recorder_sequitur_utils.c */
void  sequitur_print_rules(Grammar *grammar);
void  sequitur_print_digrams(Grammar *grammar);


#endif
