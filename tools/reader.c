#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "./reader.h"

void check_version(RecorderReader* reader) {
    char version_file[1024];
    snprintf(version_file, sizeof(version_file), "%s/VERSION", reader->logs_dir);

    FILE* fp = fopen(version_file, "r");
    assert(fp != NULL);
    int major, minor, patch;
    fscanf(fp, "%d.%d.%d", &major, &minor, &patch);
    if(major != VERSION_MAJOR || minor != VERSION_MINOR) {
        fprintf(stderr, "incompatible version: file=%d.%d.%d != reader=%d.%d.%d\n",
                major, minor, patch, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
        exit(1);
    }
    fclose(fp);
}

void read_metadata(RecorderReader* reader) {
    char metadata_file[4096];
    snprintf(metadata_file, sizeof(metadata_file), "%s/recorder.mt", reader->logs_dir);

    FILE* fp = fopen(metadata_file, "rb");
    assert(fp != NULL);
    fread(&reader->metadata, sizeof(reader->metadata), 1, fp);

    long pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp) - pos;
    char buf[fsize];

    fseek(fp, pos, SEEK_SET);
    fread(buf, 1, fsize, fp);

    int start_pos = 0, end_pos = 0;
    int func_id = 0;

    for(end_pos = 0; end_pos < fsize; end_pos++) {
        if(buf[end_pos] == '\n') {
            memset(reader->func_list[func_id], 0, sizeof(reader->func_list[func_id]));
            memcpy(reader->func_list[func_id], buf+start_pos, end_pos-start_pos);
            start_pos = end_pos+1;
            if((reader->mpi_start_idx==-1) &&
                (NULL!=strstr(reader->func_list[func_id], "MPI")))
                reader->mpi_start_idx = func_id;

            if((reader->hdf5_start_idx==-1) &&
                (NULL!=strstr(reader->func_list[func_id], "H5")))

                reader->hdf5_start_idx = func_id;

            func_id++;
        }
    }

    fclose(fp);
}

void recorder_init_reader(const char* logs_dir, RecorderReader *reader) {
    assert(logs_dir);
    assert(reader);

    memset(reader, 0, sizeof(*reader));
    strcpy(reader->logs_dir, logs_dir);
    reader->mpi_start_idx = -1;
    reader->hdf5_start_idx = -1;
    reader->prev_tstart = 0.0;

    check_version(reader);

    read_metadata(reader);

	int nprocs= reader->metadata.total_ranks;

	reader->ug_ids = malloc(sizeof(int) * nprocs);
	reader->csts   = malloc(sizeof(CST*) * nprocs);
	reader->cfgs   = malloc(sizeof(CFG*) * nprocs);

	for(int i = 0; i < nprocs; i++) {
		reader->ug_ids[i] = i;
		reader->csts[i] = NULL;
		reader->cfgs[i] = NULL;
	}

	if(reader->metadata.interprocess_compression) {
		recorder_read_cst(reader, 0);
		for(int i = 1; i < nprocs; i++)
			reader->csts[i] = reader->csts[0];

		char ug_metadata_fname[1024] = {0};
		sprintf(ug_metadata_fname, "%s/ug.mt", reader->logs_dir);
		FILE* f = fopen(ug_metadata_fname, "rb");
		fread(reader->ug_ids, sizeof(int), nprocs, f);
		fread(&reader->num_ugs, sizeof(int), 1, f);
		fclose(f);

		for(int i = 0; i < reader->num_ugs; i++) {
			recorder_read_cfg(reader, i);
		}
	}
}

void recorder_free_reader(RecorderReader *reader) {
    assert(reader);
	free(reader->ug_ids);

	if(reader->metadata.interprocess_compression) {
		recorder_free_cst(reader->csts[0]);
		free(reader->csts[0]);
		for(int i = 0; i < reader->num_ugs; i++) {
			recorder_free_cfg(reader->cfgs[i]);
			free(reader->cfgs[i]);
		}
	}

	free(reader->csts);
	free(reader->cfgs);

    memset(reader, 0, sizeof(*reader));
}

const char* recorder_get_func_name(RecorderReader* reader, Record* record) {
    if(record->func_id == RECORDER_USER_FUNCTION)
        return record->args[1];
    return reader->func_list[record->func_id];
}

int recorder_get_func_type(RecorderReader* reader, Record* record) {
    if(record->func_id < reader->mpi_start_idx)
        return RECORDER_POSIX;
    if(record->func_id < reader->hdf5_start_idx) {
        const char* func_name = recorder_get_func_name(reader, record);
        if(strncmp(func_name, "MPI_File", 8) == 0)
            return RECORDER_MPIIO;
        return RECORDER_MPI;
    }
    if(record->func_id == RECORDER_USER_FUNCTION)
        return RECORDER_FTRACE;
    return RECORDER_HDF5;
}

// Caller needs to free the record after use
// with the recorder_free_record() call.
Record* recorder_cs_to_record(CallSignature *cs) {

    Record *record = malloc(sizeof(Record));

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
    return record;
}

void recorder_free_cst(CST* cst) {
    for(int i = 0; i < cst->entries; i++)
        free(cst->cs_list[i].key);
    free(cst->cs_list);
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
    free(r);
}

void recorder_read_cst(RecorderReader *reader, int rank) {
	reader->csts[rank] = malloc(sizeof(CST));
	CST* cst = reader->csts[rank];

    cst->rank = rank;
    char cst_filename[1096] = {0};
    sprintf(cst_filename, "%s/%d.cst", reader->logs_dir, rank);

    FILE* f = fopen(cst_filename, "rb");

    int key_len;
    fread(&cst->entries, sizeof(int), 1, f);

	// cst->cs_list will be stored in the terminal_id order.
    cst->cs_list = malloc(cst->entries * sizeof(CallSignature));

    for(int i = 0; i < cst->entries; i++) {
		int terminal_id;
        fread(&terminal_id, sizeof(int), 1, f);
		assert(terminal_id < cst->entries);

		CallSignature* cs = &(cst->cs_list[terminal_id]);
		cs->terminal_id = terminal_id;

        fread(&(cs->rank), sizeof(int), 1, f);
        fread(&(cs->key_len), sizeof(int), 1, f);
        fread(&(cs->count), sizeof(int), 1, f);

        cs->key = malloc(cs->key_len);
        fread(cs->key, 1, cs->key_len, f);
    }
    fclose(f);
}


void recorder_read_cfg(RecorderReader *reader, int rank) {
	reader->cfgs[rank] = malloc(sizeof(CFG));
	CFG* cfg = reader->cfgs[rank];

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

void recorder_get_cst_cfg(RecorderReader* reader, int rank, CST** cst, CFG** cfg) {
	if(reader->metadata.interprocess_compression) {
		// Do nothing, as we have already read
		// csts and cfgs during initialization
	} else {
		if(reader->csts[rank] == NULL)
			recorder_read_cst(reader, rank);
		if(reader->cfgs[rank] == NULL)
			recorder_read_cfg(reader, rank);
	}

	*cst = reader->csts[rank];
	*cfg = reader->cfgs[reader->ug_ids[rank]];
}


#define TERMINAL_START_ID 0

void rule_application(RecorderReader* reader, CFG* cfg, CST* cst, int rule_id, FILE* ts_file,
                      void (*user_op)(Record*, void*), void* user_arg, int free_record) {

    RuleHash *rule = NULL;
    HASH_FIND_INT(cfg->cfg_head, &rule_id, rule);
    assert(rule != NULL);

    for(int i = 0; i < rule->symbols; i++) {
        int sym_val = rule->rule_body[2*i+0];
        int sym_exp = rule->rule_body[2*i+1];
        if (sym_val >= TERMINAL_START_ID) { // terminal
            for(int j = 0; j < sym_exp; j++) {
                Record* record = recorder_cs_to_record(&(cst->cs_list[sym_val]));

                // Fill in timestamps
                uint32_t ts[2];
                fread(ts, sizeof(uint32_t), 2, ts_file);
                record->tstart = ts[0] * reader->metadata.time_resolution + reader->prev_tstart;
                record->tend   = ts[1] * reader->metadata.time_resolution + reader->prev_tstart;
                reader->prev_tstart = record->tstart;

                user_op(record, user_arg);

                if(free_record)
                    recorder_free_record(record);
            }
        } else {                            // non-terminal (i.e., rule)
            for(int j = 0; j < sym_exp; j++)
                rule_application(reader, cfg, cst, sym_val, ts_file, user_op, user_arg, free_record);
        }
    }
}


// Decode all records for one rank
// one record at a time
void recorder_decode_records_core(RecorderReader *reader, CST *cst, CFG *cfg,
                             void (*user_op)(Record*, void*), void* user_arg, bool free_record) {

    reader->prev_tstart = 0.0;

    char ts_filename[1096] = {0};
    sprintf(ts_filename, "%s/%d.ts", reader->logs_dir, cst->rank);
    FILE* ts_file = fopen(ts_filename, "rb");

    rule_application(reader, cfg, cst, -1, ts_file, user_op, user_arg, free_record);

    fclose(ts_file);
}

void recorder_decode_records(RecorderReader *reader, int rank,
                             void (*user_op)(Record*, void*), void* user_arg) {

	CST* cst;
	CFG* cfg;
	recorder_get_cst_cfg(reader, rank, &cst, &cfg);
    recorder_decode_records_core(reader, cst, cfg, user_op, user_arg, true);
}



/**
 * Similar to rule application, but only calcuate
 * the total number of calls if uncompressed.
 */
size_t get_uncompressed_count(RecorderReader* reader, CFG* cfg, int rule_id) {
    RuleHash *rule = NULL;
    HASH_FIND_INT(cfg->cfg_head, &rule_id, rule);
    assert(rule != NULL);

    size_t count = 0;

    for(int i = 0; i < rule->symbols; i++) {
        int sym_val = rule->rule_body[2*i+0];
        int sym_exp = rule->rule_body[2*i+1];
        if (sym_val >= TERMINAL_START_ID) { // terminal
            count += sym_exp;
        } else {                            // non-terminal (i.e., rule)
            count += sym_exp * get_uncompressed_count(reader, cfg, sym_val);
        }
    }

    return count;
}



/**
 * Code below is used for recorder-viz
 */
typedef struct records_with_idx {
    PyRecord* records;
    int idx;
} records_with_idx_t;


void insert_one_record(Record *record, void* arg) {
    records_with_idx_t* ri = (records_with_idx_t*) arg;

    PyRecord *r = &(ri->records[ri->idx]);
    r->func_id = record->func_id;
    r->level = record->level;
    r->tstart = record->tstart;
    r->tend = record->tend;
    r->arg_count = record->arg_count;
    r->args = record->args;

    ri->idx++;

    // free record but not record->args
    free(record);
}

PyRecord** read_all_records(char* traces_dir, size_t* counts, RecorderMetadata *metadata) {

    RecorderReader reader;
    recorder_init_reader(traces_dir, &reader);
    memcpy(metadata, &(reader.metadata), sizeof(RecorderMetadata));

    PyRecord** records = malloc(sizeof(PyRecord*) * reader.metadata.total_ranks);

    for(int rank = 0; rank < reader.metadata.total_ranks; rank++) {

        CST* cst;
        CFG* cfg;
		recorder_get_cst_cfg(&reader, rank, &cst, &cfg);

        counts[rank] = get_uncompressed_count(&reader, cfg, -1);
        records[rank] = malloc(sizeof(PyRecord)* counts[rank]);

        records_with_idx_t ri;
        ri.records = records[rank];
        ri.idx = 0;

        recorder_decode_records_core(&reader, cst, cfg, insert_one_record, &ri, false);
    }

    recorder_free_reader(&reader);

    return records;
}
