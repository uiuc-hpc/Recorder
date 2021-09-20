#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "./reader.h"

static int mpi_start_idx = -1;
static int hdf5_start_idx = -1;

static double prev_tstart = 0;

void read_metadata(char* path, RecorderMetadata *metadata) {
    FILE* fp = fopen(path, "rb");
    assert(fp != NULL);
    fread(metadata, sizeof(RecorderMetadata), 1, fp);
    fclose(fp);
}

void read_func_list(char* path, RecorderReader *reader) {
    FILE* fp = fopen(path, "rb");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp) - sizeof(RecorderMetadata);
    char buf[fsize];

    fseek(fp, sizeof(RecorderMetadata), SEEK_SET); // skip RecorderMetadata object
    fread(buf, 1, fsize, fp);

    int start_pos = 0, end_pos = 0;
    int func_id = 0;

    for(end_pos = 0; end_pos < fsize; end_pos++) {
        if(buf[end_pos] == '\n') {
            memset(reader->func_list[func_id], 0, sizeof(reader->func_list[func_id]));
            memcpy(reader->func_list[func_id], buf+start_pos, end_pos-start_pos);
            start_pos = end_pos+1;
            if((mpi_start_idx==-1) &&
                (NULL!=strstr(reader->func_list[func_id], "MPI")))
                mpi_start_idx = func_id;

            if((hdf5_start_idx==-1) &&
                (NULL!=strstr(reader->func_list[func_id], "H5")))

                hdf5_start_idx = func_id;

            func_id++;
        }
    }

    fclose(fp);
}

void recorder_init_reader(const char* logs_dir, RecorderReader *reader) {
    char metadata_file[1024];
    strcpy(reader->logs_dir, logs_dir);

    sprintf(metadata_file, "%s/recorder.mt", logs_dir);
    read_metadata(metadata_file, &reader->metadata);
    read_func_list(metadata_file, reader);
}

void recorder_free_reader(RecorderReader *reader) {
}

const char* recorder_get_func_name(RecorderReader* reader, Record* record) {
    if(record->func_id == RECORDER_USER_FUNCTION)
        return record->args[0];
    return reader->func_list[record->func_id];
}

int recorder_get_func_type(RecorderReader* reader, Record* record) {
    if(record->func_id < mpi_start_idx)
        return RECORDER_POSIX;
    if(record->func_id < hdf5_start_idx) {
        const char* func_name = recorder_get_func_name(reader, record);
        if(strncmp(func_name, "MPI_File", 8) == 0)
            return RECORDER_MPIIO;
        return RECORDER_MPI;
    }
    if(record->func_id == RECORDER_USER_FUNCTION)
        return RECORDER_FTRACE;
    return RECORDER_HDF5;
}

void cs_to_record(CallSignature *cs, Record *record) {
    char* key = cs->key;

    int pos = 0;
    memcpy(&record->tid, key+pos, sizeof(pthread_t));
    pos += sizeof(pthread_t);
    memcpy(&record->func_id, key+pos, sizeof(record->func_id));
    pos += sizeof(record->func_id);
    memcpy(&record->level, key+pos, sizeof(record->level));
    pos += sizeof(record->level);
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

void recorder_free_record(Record* r) {
    for(int i = 0; i < r->arg_count; i++)
        free(r->args[i]);
    free(r->args);
}

void recorder_read_cst(RecorderReader *reader, int rank, CST *cst) {
    cst->rank = rank;
    char cst_filename[1096] = {0};
    sprintf(cst_filename, "%s/%d.cst", reader->logs_dir, rank);

    FILE* f = fopen(cst_filename, "rb");

    int key_len;
    fread(&cst->entries, sizeof(int), 1, f);

    cst->cst_list = malloc(cst->entries * sizeof(CallSignature));

    for(int i = 0; i < cst->entries; i++) {
        fread(&cst->cst_list[i].terminal, sizeof(int), 1, f);
        fread(&cst->cst_list[i].key_len, sizeof(int), 1, f);

        cst->cst_list[i].key = malloc(cst->cst_list[i].key_len);
        fread(cst->cst_list[i].key, 1, cst->cst_list[i].key_len, f);

        assert(cst->cst_list[i].terminal < cst->entries);
    }
    fclose(f);

    //for(int i = 0; i < cst->entries; i++)
    //    printf("%d, terminal %d, key len: %d\n", i, cst->cst_list[i].terminal, cst->cst_list[i].key_len);
}

void recorder_read_cfg(RecorderReader *reader, int rank, CFG* cfg) {
    cfg->rank = rank;
    char cfg_filename[1096] = {0};
    sprintf(cfg_filename, "%s/%d.cfg", reader->logs_dir, rank);

    FILE* f = fopen(cfg_filename, "rb");

    fread(&cfg->rules, sizeof(int), 1, f);

    cfg->cfg_head = NULL;
    for(int i = 0; i < cfg->rules; i++) {
        RuleHash *rule = malloc(sizeof(RuleHash));

        fread(&(rule->rule_id), sizeof(int), 1, f);
        fread(&(rule->symbols), sizeof(int), 1, f);
        //printf("rule id: %d, symbols: %d\n", rule->rule_id, rule->symbols);

        rule->rule_body = (int*) malloc(sizeof(int)*rule->symbols*2);
        fread(rule->rule_body, sizeof(int), rule->symbols*2, f);
        HASH_ADD_INT(cfg->cfg_head, rule_id, rule);
    }
    fclose(f);
}


#define TERMINAL_START_ID 0

void rule_application(RecorderReader* reader, RuleHash* rules, int rule_id, CallSignature *cst_list, FILE* ts_file,
                      void (*user_op)(Record*, void*), void* user_arg) {

    RuleHash *rule = NULL;
    HASH_FIND_INT(rules, &rule_id, rule);
    assert(rule != NULL);

    for(int i = 0; i < rule->symbols; i++) {
        int sym_val = rule->rule_body[2*i+0];
        int sym_exp = rule->rule_body[2*i+1];
        if (sym_val >= TERMINAL_START_ID) { // terminal
            for(int j = 0; j < sym_exp; j++) {
                Record record;
                cs_to_record(&cst_list[sym_val], &record);

                // Fill in timestamps
                uint32_t ts[2];
                fread(ts, sizeof(uint32_t), 2, ts_file);
                record.tstart = ts[0] * reader->metadata.time_resolution + prev_tstart;
                record.tend   = ts[1] * reader->metadata.time_resolution + prev_tstart;
                prev_tstart = record.tstart;

                user_op(&record, user_arg);

                recorder_free_record(&record);
            }
        } else {                            // non-terminal (i.e., rule)
            for(int j = 0; j < sym_exp; j++)
                rule_application(reader, rules, sym_val, cst_list, ts_file, user_op, user_arg);
        }
    }
}

void recorder_decode_records(RecorderReader *reader, CST *cst, CFG *cfg,
                             void (*user_op)(Record*, void*), void* user_arg) {

    assert(cst->rank == cfg->rank);

    char ts_filename[1096] = {0};
    sprintf(ts_filename, "%s/%d.ts", reader->logs_dir, cst->rank);
    FILE* ts_file = fopen(ts_filename, "rb");

    rule_application(reader, cfg->cfg_head, -1, cst->cst_list, ts_file, user_op, user_arg);

    fclose(ts_file);
}

