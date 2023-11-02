#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <zlib.h>
#include "reader.h"
#include "reader-private.h"

void* read_zlib(FILE* source) {
    const int CHUNK = 65536;
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK]; // TODO remove out, and store directly to uncompressed.

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return NULL;

    // In the first two size_t we alway store comperssed_size
    // and decompressed_size. See write_zlib in recorder-utils.c
    size_t compressed_size, decompressed_size;
    fread(&compressed_size, sizeof(size_t), 1, source);
    fread(&decompressed_size, sizeof(size_t), 1, source);
    void* compressed   = malloc(compressed_size);
    void* decompressed = malloc(decompressed_size);
    void* p_decompressed = decompressed;
    //printf("compressed size: %ld, decompressed size: %ld\n", 
    //       compressed_size, decompressed_size);

    strm.avail_in = fread(compressed, 1, compressed_size, source);
    strm.next_in  = compressed;
    assert(strm.avail_in == compressed_size);

    /* run inflate() on input until output buffer not full */
    do {
        strm.avail_out = CHUNK;
        strm.next_out  = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                free(compressed);
                free(decompressed);
                return NULL;
        }
        have = CHUNK - strm.avail_out;
        memcpy(p_decompressed, out, have);
        p_decompressed += have;
    } while (strm.avail_out == 0);

    (void)inflateEnd(&strm);
    free(compressed);
    return decompressed;
}

void check_version(RecorderReader* reader) {
    char version_file[1096] = {0};
    sprintf(version_file, "%s/VERSION", reader->logs_dir);

    FILE* fp = fopen(version_file, "r");
    assert(fp != NULL);
    int major, minor, patch;
    fscanf(fp, "%d.%d.%d", &major, &minor, &patch);
    if(major != RECORDER_VERSION_MAJOR || minor != RECORDER_VERSION_MINOR) {
        fprintf(stderr, "incompatible version: file=%d.%d.%d != reader=%d.%d.%d\n",
                major, minor, patch, RECORDER_VERSION_MAJOR,
                RECORDER_VERSION_MINOR, RECORDER_VERSION_PATCH);
        exit(1);
    }
    fclose(fp);
}

void read_metadata(RecorderReader* reader) {
    char metadata_file[1096] = {0};
    sprintf(metadata_file, "%s/recorder.mt", reader->logs_dir);

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
    reader->ugs    = malloc(sizeof(CFG*) * nprocs);
	reader->csts   = malloc(sizeof(CST*) * nprocs);
	reader->cfgs   = malloc(sizeof(CFG*) * nprocs);

	if(reader->metadata.interprocess_compression) {
        // a single file for merged csts
        // and a single for unique cfgs
        void* buf_cst;
        void* buf_cfg;

        // Read and parse the cst file
		char cst_fname[1096] = {0};
		sprintf(cst_fname, "%s/recorder.cst", reader->logs_dir);
		FILE* cst_file = fopen(cst_fname, "rb");
        buf_cst = read_zlib(cst_file);
        reader->csts[0] = (CST*) malloc(sizeof(CST));
		reader_decode_cst(0, buf_cst, reader->csts[0]);
        fclose(cst_file);
        free(buf_cst);

		char ug_metadata_fname[1096] = {0};
		sprintf(ug_metadata_fname, "%s/ug.mt", reader->logs_dir);
		FILE* f = fopen(ug_metadata_fname, "rb");
		fread(reader->ug_ids, sizeof(int), nprocs, f);
		fread(&reader->num_ugs, sizeof(int), 1, f);
		fclose(f);

		char cfg_fname[1096] = {0};
		sprintf(cfg_fname, "%s/ug.cfg", reader->logs_dir);
		FILE* cfg_file = fopen(cfg_fname, "rb");
        for(int i = 0; i < reader->num_ugs; i++) {
            buf_cfg = read_zlib(cfg_file);
            reader->ugs[i] = (CFG*) malloc(sizeof(CFG));
            reader_decode_cfg(i, buf_cfg, reader->ugs[i]);
            free(buf_cfg);
        }
        fclose(cfg_file);

        for(int rank = 0; rank < nprocs; rank++) {
            reader->csts[rank] = reader->csts[0];
            reader->cfgs[rank] = reader->ugs[reader->ug_ids[rank]];
        }

	} else {
        for(int rank = 0; rank < nprocs; rank++) {
            char cst_fname[1096] = {0};
            sprintf(cst_fname, "%s/%d.cst", reader->logs_dir, rank);
            FILE* cst_file = fopen(cst_fname, "rb");
            void* buf_cst = read_zlib(cst_file);
            reader->csts[rank] = (CST*) malloc(sizeof(CST));
		    reader_decode_cst(rank, buf_cst, reader->csts[rank]);
            free(buf_cst);
            fclose(cst_file);

            char cfg_fname[1096] = {0};
            sprintf(cfg_fname, "%s/%d.cfg", reader->logs_dir, rank);
            FILE* cfg_file = fopen(cfg_fname, "rb");
            void* buf_cfg = read_zlib(cfg_file);
            reader->cfgs[rank] = (CFG*) malloc(sizeof(CFG));
            reader_decode_cfg(rank, buf_cfg, reader->cfgs[rank]);
            free(buf_cfg);
            fclose(cfg_file);
        }
    }
}

void recorder_free_reader(RecorderReader *reader) {
    assert(reader);

	if(reader->metadata.interprocess_compression) {
		reader_free_cst(reader->csts[0]);
		free(reader->csts[0]);
		for(int i = 0; i < reader->num_ugs; i++) {
			reader_free_cfg(reader->ugs[i]);
			free(reader->ugs[i]);
		}
	} else {
		for(int rank = 0; rank < reader->metadata.total_ranks; rank++) {
            reader_free_cst(reader->csts[rank]);
            reader_free_cfg(reader->cfgs[rank]);
        }
    }

	free(reader->csts);
	free(reader->cfgs);
	free(reader->ugs);
	free(reader->ug_ids);

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

void recorder_free_record(Record* r) {
    for(int i = 0; i < r->arg_count; i++)
        free(r->args[i]);
    free(r->args);
    free(r);
}


#define TERMINAL_START_ID 0

void rule_application(RecorderReader* reader, CFG* cfg, CST* cst, int rule_id, uint32_t* ts_buf,
                      void (*user_op)(Record*, void*), void* user_arg, int free_record) {

    RuleHash *rule = NULL;
    HASH_FIND_INT(cfg->cfg_head, &rule_id, rule);
    assert(rule != NULL);

    for(int i = 0; i < rule->symbols; i++) {
        int sym_val = rule->rule_body[2*i+0];
        int sym_exp = rule->rule_body[2*i+1];
        if (sym_val >= TERMINAL_START_ID) { // terminal
            for(int j = 0; j < sym_exp; j++) {
                Record* record = reader_cs_to_record(&(cst->cs_list[sym_val]));

                // Fill in timestamps
                uint32_t ts[2] = {ts_buf[0], ts_buf[1]};
                ts_buf += 2;
                record->tstart = ts[0] * reader->metadata.time_resolution + reader->prev_tstart;
                record->tend   = ts[1] * reader->metadata.time_resolution + reader->prev_tstart;
                reader->prev_tstart = record->tstart;

                user_op(record, user_arg);

                if(free_record)
                    recorder_free_record(record);
            }
        } else {                            // non-terminal (i.e., rule)
            for(int j = 0; j < sym_exp; j++)
                rule_application(reader, cfg, cst, sym_val, ts_buf, user_op, user_arg, free_record);
        }
    }
}

void decode_records_core(RecorderReader *reader, int rank,
                             void (*user_op)(Record*, void*), void* user_arg, bool free_record) {

    int nprocs = reader->metadata.total_ranks;
	CST* cst = reader_get_cst(reader, rank);
	CFG* cfg = reader_get_cfg(reader, rank);

    reader->prev_tstart = 0.0;

    char ts_fname[1096] = {0};
    sprintf(ts_fname, "%s/recorder.ts", reader->logs_dir);
    FILE* ts_file = fopen(ts_fname, "rb");

    // the first nprocs size_t store the buf size 
    // of timestamps of each rank
    // see lib/recorder-timestamps.c
    size_t buf_sizes[nprocs];
    fread(buf_sizes, sizeof(size_t), nprocs, ts_file);

    // calculate the starting offset of the desired rank
    // nprocs*sizeof(size_t) + offset
    size_t offset = 0;
    for(int r = 0; r < rank; r++) {
        offset += buf_sizes[r];
    }
    fseek(ts_file, offset, SEEK_CUR);

    // finally read to the buffer
    uint32_t* ts_buf;
    if (reader->metadata.ts_compression) {
        ts_buf = (uint32_t*) read_zlib(ts_file);
    } else {
        ts_buf = (uint32_t*) malloc(buf_sizes[rank]); 
        fread(ts_buf, 1, buf_sizes[rank], ts_file);
    }
    fclose(ts_file);

    rule_application(reader, cfg, cst, -1, ts_buf, user_op, user_arg, free_record);

    free(ts_buf);
}

// Decode all records for one rank
// one record at a time
void recorder_decode_records(RecorderReader *reader, int rank,
                             void (*user_op)(Record*, void*), void* user_arg) {
    decode_records_core(reader, rank, user_op, user_arg, true);
}

void recorder_decode_records2(RecorderReader *reader, int rank,
                             void (*user_op)(Record*, void*), void* user_arg) {
    decode_records_core(reader, rank, user_op, user_arg, false);
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
    r->call_depth = record->call_depth;
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

        CST* cst = reader_get_cst(&reader, rank);
        CFG* cfg = reader_get_cfg(&reader, rank);

        counts[rank] = get_uncompressed_count(&reader, cfg, -1);
        records[rank] = malloc(sizeof(PyRecord)* counts[rank]);

        records_with_idx_t ri;
        ri.records = records[rank];
        ri.idx = 0;

        recorder_decode_records2(&reader, rank, insert_one_record, &ri);
    }

    recorder_free_reader(&reader);

    return records;
}
