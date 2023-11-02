#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <zlib.h>
#include "./reader-private.h"

void reader_free_cst(CST* cst) {
    for(int i = 0; i < cst->entries; i++)
        free(cst->cs_list[i].key);
    free(cst->cs_list);
}

void reader_free_cfg(CFG* cfg) {
    RuleHash *r, *tmp;
    HASH_ITER(hh, cfg->cfg_head, r, tmp) {
        HASH_DEL(cfg->cfg_head, r);
        free(r->rule_body);
        free(r);
    }
}

void reader_decode_cst(int rank, void* buf, CST* cst) {
    cst->rank = rank;

    memcpy(&cst->entries, buf, sizeof(int));
    buf += sizeof(int);

	// cst->cs_list will be stored in the terminal_id order.
    cst->cs_list = malloc(cst->entries * sizeof(CallSignature));

    for(int i = 0; i < cst->entries; i++) {

		int terminal_id;
        memcpy(&terminal_id, buf, sizeof(int));
        buf += sizeof(int);
		assert(terminal_id < cst->entries);

		CallSignature* cs = &(cst->cs_list[terminal_id]);
		cs->terminal_id = terminal_id;

        memcpy(&cs->rank, buf, sizeof(int));
        buf += sizeof(int);
        memcpy(&cs->key_len, buf, sizeof(int));
        buf += sizeof(int);
        memcpy(&cs->count, buf, sizeof(int));
        buf += sizeof(int);

        cs->key = malloc(cs->key_len);
        memcpy(cs->key, buf, cs->key_len);
        buf += cs->key_len;
    }
}

void reader_decode_cfg(int rank, void* buf, CFG* cfg) {

    cfg->rank = rank;

    memcpy(&cfg->rules, buf, sizeof(int));
    buf += sizeof(int);

    cfg->cfg_head = NULL;
    for(int i = 0; i < cfg->rules; i++) {
        RuleHash *rule = malloc(sizeof(RuleHash));

        memcpy(&rule->rule_id, buf, sizeof(int));
        buf += sizeof(int);
        memcpy(&rule->symbols, buf, sizeof(int));
        buf += sizeof(int);

        rule->rule_body = (int*) malloc(sizeof(int)*rule->symbols*2);
        memcpy(rule->rule_body, buf, sizeof(int)*rule->symbols*2);
        buf += sizeof(int)*rule->symbols*2;
        HASH_ADD_INT(cfg->cfg_head, rule_id, rule);
    }
}

CST* reader_get_cst(RecorderReader* reader, int rank) {
	CST* cst = reader->csts[rank];
    return cst;
}

CFG* reader_get_cfg(RecorderReader* reader, int rank) {
    CFG* cfg;
    if (reader->metadata.interprocess_compression)
	    cfg = reader->cfgs[reader->ug_ids[rank]];
    else
        cfg = reader->cfgs[rank];
    return cfg;
}

// Caller needs to free the record after use
// by using recorder_free_record() call.
Record* reader_cs_to_record(CallSignature *cs) {

    Record *record = malloc(sizeof(Record));

    char* key = cs->key;

    int pos = 0;
    memcpy(&record->tid, key+pos, sizeof(pthread_t));
    pos += sizeof(pthread_t);
    memcpy(&record->func_id, key+pos, sizeof(record->func_id));
    pos += sizeof(record->func_id);
    memcpy(&record->call_depth, key+pos, sizeof(record->call_depth));
    pos += sizeof(record->call_depth);
    memcpy(&record->arg_count, key+pos, sizeof(record->arg_count));
    pos += sizeof(record->arg_count);

    record->args = malloc(sizeof(char*) * record->arg_count);

    int arg_strlen;
    memcpy(&arg_strlen, key+pos, sizeof(int));
    pos += sizeof(int);

    char* arg_str = key+pos;
    int ai = 0;
    int start = 0;
    for(int i = 0; i < arg_strlen; i++) {
        if(arg_str[i] == ' ') {
            record->args[ai++] = strndup(arg_str+start, (i-start));
            start = i + 1;
        }
    }

    assert(ai == record->arg_count);
    return record;
}

