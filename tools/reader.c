#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "./reader.h"

void read_global_metadata(char* path, RecorderGlobalDef *RGD) {
    FILE* fp = fopen(path, "r+b");
    fread(RGD, sizeof(RecorderGlobalDef), 1, fp);
    fclose(fp);
}

void read_func_list(char* path, RecorderReader *reader) {
    FILE* fp = fopen(path, "r+b");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp) - sizeof(RecorderGlobalDef);
    char buf[fsize];

    fseek(fp, sizeof(RecorderGlobalDef), SEEK_SET); // skip GlobalDef object
    fread(buf, 1, fsize, fp);

    int start_pos = 0, end_pos = 0;
    int func_id = 0;

    for(end_pos = 0; end_pos < fsize; end_pos++) {
        if(buf[end_pos] == '\n') {
            memset(reader->func_list[func_id], 0, sizeof(reader->func_list[func_id]));
            memcpy(reader->func_list[func_id], buf+start_pos, end_pos-start_pos);
            start_pos = end_pos+1;
            func_id++;
        }
    }

    fclose(fp);
}

/*

// Return an array of char*, where each element is an argument
// The input is the original arguments string
char** get_record_arguments(char* str, int arg_count) {

char** args = (char**) malloc(sizeof(char*) * arg_count);

int i = 0;
char* token = strtok(str, " ");

while( token != NULL ) {
args[i++] = strdup(token);
token = strtok(NULL, " ");
}

return args;
}


Record* read_records(char* path, RecorderLocalDef* RLD, RecorderGlobalDef *RGD) {

Record *records = (Record*) malloc(sizeof(Record) * RLD->total_records);

FILE* fp = fopen(path, "r+b");

fseek(fp, 0, SEEK_END);
long fsize = ftell(fp);
fseek(fp, 0, SEEK_SET);

char *content = (char*)malloc(fsize);
fread(content, 1, fsize, fp);


long args_start_pos = 0;
long rec_start_pos = 0;

int i, ri = 0;
while(rec_start_pos < fsize) {
// read one record
Record *r = &(records[ri++]);

// 1. First 14 bytes: status, tstart, tend, func_id, res;
int tstart; int tend;
memcpy(&(r->status), content+rec_start_pos+0, 1);
memcpy(&tstart, content+rec_start_pos+1, 4);
memcpy(&tend, content+rec_start_pos+5, 4);
memcpy(&(r->res), content+rec_start_pos+9, 4);
memcpy(&(r->func_id), content+rec_start_pos+13, 1);

r->tstart = tstart * RGD->time_resolution;
r->tend = tend * RGD->time_resolution;
r->arg_count = 0;

// 2. Then arguments splited by ' '
// '\n' marks the end of one record
args_start_pos = rec_start_pos + 15;
for(i = rec_start_pos+14; i < fsize; i++) {
if(' ' == content[i])
r->arg_count++;
if('\n' == content[i]) {
rec_start_pos = i + 1;
break;
}
}

if(r->arg_count) {
int len = rec_start_pos-args_start_pos;
char* arguments_str = (char*) malloc(sizeof(char) * len);
memcpy(arguments_str, content+args_start_pos, len-1);
arguments_str[len-1] = 0;
r->args = get_record_arguments(arguments_str, r->arg_count);
free(arguments_str);
}
}

free(content);
fclose(fp);

return records;
}

void release_resources(RecorderReader *reader) {
    int ranks = reader->RGD.total_ranks;

    int i, j, rank;
    for (rank = 0; rank < ranks; rank++) {

        Record* records = reader->records[rank];
        for(i = 0; i < reader->RLDs[rank].total_records; i++) {
            for(j = 0; j < records[i].arg_count; j++)
                free(records[i].args[j]);
            free(records[i].args);
        }

        free(records);
    }
    free(reader->records);
    free(reader->RLDs);
}
*/

void recorder_init_reader(const char* logs_dir, RecorderReader *reader) {

    char global_metadata_file[256];
    strcpy(reader->logs_dir, logs_dir);

    sprintf(global_metadata_file, "%s/recorder.mt", logs_dir);
    read_global_metadata(global_metadata_file, &(reader->RGD));
    read_func_list(global_metadata_file, reader);
}

void recorder_free_reader(RecorderReader *reader) {
}


CallSignature* read_cst_file(const char* path, int *entries) {
    FILE* f = fopen(path, "rb");

    int key_len, terminal;
    fread(entries, sizeof(int), 1, f);

    CallSignature *cst = malloc(*entries * sizeof(CallSignature));

    for(int i = 0; i < *entries; i++) {
        fread(&cst[i].terminal, sizeof(int), 1, f);
        fread(&cst[i].key_len, sizeof(int), 1, f);

        cst[i].key = malloc(cst[i].key_len);
        fread(cst[i].key, 1, cst[i].key_len, f);

        assert(terminal < *entries);
    }

    fclose(f);
    return cst;
}

void recorder_free_cst(CST* cst) {
    for(int i = 0; i < cst->entries; i++)
        free(cst->cst_list[i].key);
    free(cst->cst_list);
}

void recorder_free_cfg(CFG* cfg) {
    RuleHash *r, *tmp;
    HASH_ITER(hh, cfg->cfg_head, r, tmp) {
        HASH_DEL(cfg->cfg_head, r);
        free(r->rule_body);
        free(r);
    }
}

void recorder_read_cst(RecorderReader *reader, int rank, CST *cst) {

    char cst_filename[1096] = {0};
    sprintf(cst_filename, "%s/%d.cst", reader->logs_dir, rank);

    cst->cst_list = read_cst_file(cst_filename, &cst->entries);
    for(int i = 0; i < cst->entries; i++) {
        printf("%d, terminal %d, key len: %d\n", i, cst->cst_list[i].terminal, cst->cst_list[i].key_len);
    }
}

void recorder_read_cfg(RecorderReader *reader, int rank, CFG* cfg) {
    char cfg_filename[1096] = {0};
    sprintf(cfg_filename, "%s/%d.cfg", reader->logs_dir, rank);

    FILE* f = fopen(cfg_filename, "rb");

    fread(&cfg->rules, sizeof(int), 1, f);

    cfg->cfg_head = NULL;
    for(int i = 0; i < cfg->rules; i++) {
        RuleHash *rule = malloc(sizeof(RuleHash));

        fread(&(rule->rule_id), sizeof(int), 1, f);
        fread(&(rule->symbols), sizeof(int), 1, f);
        printf("rule id: %d, symbols: %d\n", rule->rule_id, rule->symbols);

        rule->rule_body = (int*) malloc(sizeof(int)*rule->symbols*2);
        fread(rule->rule_body, sizeof(int), rule->symbols*2, f);
        HASH_ADD_INT(cfg->cfg_head, rule_id, rule);
    }
    fclose(f);
}


void recorder_decode_records() {
}
