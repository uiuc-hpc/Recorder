#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
    //printf("read zlib compressed size: %ld, decompressed size: %ld\n", 
    //       compressed_size, decompressed_size);
    //fflush(stdout);
    void* compressed   = malloc(compressed_size);
    void* decompressed = malloc(decompressed_size);
    void* p_decompressed = decompressed;

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

/*
 * Returns true if the trace directory contains a combined recorder.dat file.
 */
static bool is_combined_format(const char* logs_dir) {
    char path[1096] = {0};
    sprintf(path, "%s/recorder.dat", logs_dir);
    FILE* fp = fopen(path, "rb");
    if (fp) { fclose(fp); return true; }
    return false;
}

/*
 * Returns true if the trace directory uses the legacy format (VERSION file present).
 * Legacy format: RecorderMetadata (old struct without version/num_funcs) at offset 0,
 *                1024-byte reserved block, then newline-delimited function names.
 * New format:    RecorderMetadata (new struct with version/num_funcs) at offset 0,
 *                then fixed RECORDER_FUNC_NAME_LEN-byte binary entries for each function.
 */
static bool is_legacy_format(const char* logs_dir) {
    char version_file[1096] = {0};
    sprintf(version_file, "%s/VERSION", logs_dir);
    FILE* fp = fopen(version_file, "r");
    if (fp) { fclose(fp); return true; }
    return false;
}

/*
 * Load the section index from recorder.dat into reader->sections.
 * Sets reader->combined_path and reader->num_sections.
 */
static void load_combined_sections(RecorderReader* reader) {
    sprintf(reader->combined_path, "%s/recorder.dat", reader->logs_dir);
    FILE* fp = fopen(reader->combined_path, "rb");
    assert(fp != NULL);

    RecorderFileHeader hdr;
    fread(&hdr, sizeof(hdr), 1, fp);
    assert(memcmp(hdr.magic, "RECORDER", 8) == 0);

    reader->num_sections = (int)hdr.num_sections;
    reader->sections = malloc(sizeof(RecorderSectionEntry) * reader->num_sections);
    fread(reader->sections, sizeof(RecorderSectionEntry), reader->num_sections, fp);
    fclose(fp);
}

/*
 * Open the combined recorder.dat positioned at the start of section (type, rank).
 * *size_out receives the section byte size (may be NULL).
 * Returns NULL if the section is not found.
 * Caller must fclose() the returned FILE*.
 */
static FILE* open_section(RecorderReader* reader, int type, int rank, uint64_t* size_out) {
    for (int i = 0; i < reader->num_sections; i++) {
        if ((int)reader->sections[i].type == type &&
            reader->sections[i].rank      == rank) {
            FILE* fp = fopen(reader->combined_path, "rb");
            if (!fp) return NULL;
            fseek(fp, (long)reader->sections[i].offset, SEEK_SET);
            if (size_out) *size_out = reader->sections[i].size;
            return fp;
        }
    }
    return NULL;
}

void check_version(RecorderReader* reader, int* v_major, int* v_minor) {
    if (is_combined_format(reader->logs_dir)) {
        load_combined_sections(reader);
        /* METADATA section starts with the RecorderMetadata struct whose first
           three fields are version_major, version_minor, version_patch. */
        FILE* fp = open_section(reader, RECORDER_SECTION_METADATA, -1, NULL);
        assert(fp != NULL);
        int version[3];
        fread(version, sizeof(int), 3, fp);
        fclose(fp);
        *v_major = version[0];
        *v_minor = version[1];
    } else if (is_legacy_format(reader->logs_dir)) {
        char version_file[1096] = {0};
        sprintf(version_file, "%s/VERSION", reader->logs_dir);
        FILE* fp = fopen(version_file, "r");
        int major, minor, patch;
        fscanf(fp, "%d.%d.%d", &major, &minor, &patch);
        fclose(fp);
        *v_major = major;
        *v_minor = minor;
    } else {
        // Multi-file new format: version fields are the first three ints in recorder.mt
        char metadata_file[1096] = {0};
        sprintf(metadata_file, "%s/recorder.mt", reader->logs_dir);
        FILE* fp = fopen(metadata_file, "rb");
        assert(fp != NULL);
        int version[3];
        fread(version, sizeof(int), 3, fp);
        fclose(fp);
        *v_major = version[0];
        *v_minor = version[1];
    }

    double v1 = *v_major + *v_minor/10.0;
    double v2 = RECORDER_VERSION_MAJOR + RECORDER_VERSION_MINOR/10.0;
    if (v1 > v2) {
        fprintf(stderr, "incompatible version: trace=%d.%d > reader=%d.%d\n",
                *v_major, *v_minor, RECORDER_VERSION_MAJOR, RECORDER_VERSION_MINOR);
        exit(1);
    }
}

static void index_func_list(RecorderReader* reader) {
    for (int i = 0; i < reader->supported_funcs; i++) {
        if (reader->mpi_start_idx == -1 && strstr(reader->func_list[i], "MPI"))
            reader->mpi_start_idx = i;
        if (reader->hdf5_start_idx == -1 && strstr(reader->func_list[i], "H5"))
            reader->hdf5_start_idx = i;
        if (reader->pnetcdf_start_idx == -1 && strstr(reader->func_list[i], "ncmpi"))
            reader->pnetcdf_start_idx = i;
        if (reader->netcdf_start_idx == -1 && strstr(reader->func_list[i], "nc_"))
            reader->netcdf_start_idx = i;
    }
}

void read_metadata(RecorderReader* reader) {
    FILE* fp;
    if (reader->num_sections > 0) {
        fp = open_section(reader, RECORDER_SECTION_METADATA, -1, NULL);
    } else {
        char metadata_file[1096] = {0};
        sprintf(metadata_file, "%s/recorder.mt", reader->logs_dir);
        fp = fopen(metadata_file, "rb");
    }
    assert(fp != NULL);

    if (reader->trace_version_major == 2 && reader->trace_version_minor == 3) {
        // v2.3 legacy struct: no version fields, different field set
        struct RecorderMetadata_2_3 {
            int    total_ranks;
            double start_ts;
            double time_resolution;
            int    ts_buffer_elements;
            int    ts_compression_algo;
        };
        struct RecorderMetadata_2_3 metadata_2_3;
        fread(&metadata_2_3, sizeof(metadata_2_3), 1, fp);
        reader->metadata.version_major = 2;
        reader->metadata.version_minor = 3;
        reader->metadata.version_patch = 0;
        reader->metadata.total_ranks = metadata_2_3.total_ranks;
        reader->metadata.posix_tracing = 1;
        reader->metadata.mpi_tracing = 1;
        reader->metadata.mpiio_tracing = 1;
        reader->metadata.hdf5_tracing = 1;
        reader->metadata.pnetcdf_tracing = 0;
        reader->metadata.netcdf_tracing = 0;
        reader->metadata.store_tid = 1;
        reader->metadata.store_call_depth = 1;
        reader->metadata.start_ts = metadata_2_3.start_ts;
        reader->metadata.time_resolution = metadata_2_3.time_resolution;
        reader->metadata.ts_buffer_elements = metadata_2_3.ts_buffer_elements;
        reader->metadata.interprocess_compression = 0;
        reader->metadata.interprocess_pattern_recognition = 0;
        reader->metadata.intraprocess_pattern_recognition = 0;
        reader->metadata.ts_compression = 0;
    } else if (is_legacy_format(reader->logs_dir)) {
        // Old v3 format: struct without version/num_funcs fields at offset 0
        struct OldRecorderMetadata_3 {
            int    total_ranks;
            bool   posix_tracing;
            bool   mpi_tracing;
            bool   mpiio_tracing;
            bool   hdf5_tracing;
            bool   pnetcdf_tracing;
            bool   netcdf_tracing;
            bool   store_tid;
            bool   store_call_depth;
            double start_ts;
            double time_resolution;
            int    ts_buffer_elements;
            bool   ts_compression;
            bool   interprocess_compression;
            bool   interprocess_pattern_recognition;
            bool   intraprocess_pattern_recognition;
        };
        struct OldRecorderMetadata_3 old_meta;
        fread(&old_meta, sizeof(old_meta), 1, fp);
        reader->metadata.version_major = reader->trace_version_major;
        reader->metadata.version_minor = reader->trace_version_minor;
        reader->metadata.version_patch = 0;
        reader->metadata.total_ranks = old_meta.total_ranks;
        reader->metadata.posix_tracing = old_meta.posix_tracing;
        reader->metadata.mpi_tracing = old_meta.mpi_tracing;
        reader->metadata.mpiio_tracing = old_meta.mpiio_tracing;
        reader->metadata.hdf5_tracing = old_meta.hdf5_tracing;
        reader->metadata.pnetcdf_tracing = old_meta.pnetcdf_tracing;
        reader->metadata.netcdf_tracing = old_meta.netcdf_tracing;
        reader->metadata.store_tid = old_meta.store_tid;
        reader->metadata.store_call_depth = old_meta.store_call_depth;
        reader->metadata.start_ts = old_meta.start_ts;
        reader->metadata.time_resolution = old_meta.time_resolution;
        reader->metadata.ts_buffer_elements = old_meta.ts_buffer_elements;
        reader->metadata.ts_compression = old_meta.ts_compression;
        reader->metadata.interprocess_compression = old_meta.interprocess_compression;
        reader->metadata.interprocess_pattern_recognition = old_meta.interprocess_pattern_recognition;
        reader->metadata.intraprocess_pattern_recognition = old_meta.intraprocess_pattern_recognition;
    } else {
        // New format: struct includes version and num_funcs
        fread(&reader->metadata, sizeof(reader->metadata), 1, fp);
    }

    if (!is_legacy_format(reader->logs_dir)) {
        // New binary format: RECORDER_FUNC_NAME_LEN-byte null-padded entries follow the struct
        reader->supported_funcs = reader->metadata.num_funcs;
        reader->func_list = (char**) malloc(sizeof(char*) * reader->supported_funcs);
        for (int i = 0; i < reader->supported_funcs; i++) {
            reader->func_list[i] = (char*) malloc(RECORDER_FUNC_NAME_LEN);
            fread(reader->func_list[i], RECORDER_FUNC_NAME_LEN, 1, fp);
        }
    } else {
        // Legacy text format: newline-delimited function names after 1024-byte offset
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp) - 1024;
        char* buf = (char*) malloc(fsize);

        fseek(fp, 1024, SEEK_SET);
        fread(buf, 1, fsize, fp);

        reader->supported_funcs = 0;
        for (long i = 0; i < fsize; i++) {
            if (buf[i] == '\n') reader->supported_funcs++;
        }

        reader->func_list = (char**) malloc(sizeof(char*) * reader->supported_funcs);
        for (int i = 0; i < reader->supported_funcs; i++) {
            reader->func_list[i] = (char*) malloc(64);
            memset(reader->func_list[i], 0, 64);
        }

        int start_pos = 0, func_id = 0;
        for (long end_pos = 0; end_pos < fsize; end_pos++) {
            if (buf[end_pos] == '\n') {
                memcpy(reader->func_list[func_id], buf + start_pos, end_pos - start_pos);
                start_pos = end_pos + 1;
                func_id++;
            }
        }
        free(buf);
        reader->metadata.num_funcs = reader->supported_funcs;
    }

    index_func_list(reader);
    fclose(fp);
}

void recorder_init_reader(const char* logs_dir, RecorderReader *reader) {
    assert(logs_dir);
    assert(reader);

    memset(reader, 0, sizeof(*reader));
    strcpy(reader->logs_dir, logs_dir);
    reader->mpi_start_idx = -1;
    reader->hdf5_start_idx = -1;
    reader->pnetcdf_start_idx = -1;
    reader->netcdf_start_idx = -1;
    reader->prev_tstart = 0.0;

    check_version(reader, &reader->trace_version_major, &reader->trace_version_minor);

    read_metadata(reader);

    int nprocs= reader->metadata.total_ranks;

    reader->ug_ids = malloc(sizeof(int) * nprocs);
    reader->ugs    = malloc(sizeof(CFG*) * nprocs);
    reader->csts   = malloc(sizeof(CST*) * nprocs);
    reader->cfgs   = malloc(sizeof(CFG*) * nprocs);

    if(reader->metadata.interprocess_compression) {
        void* buf_cst;
        void* buf_cfg;

        FILE* cst_file;
        if (reader->num_sections > 0)
            cst_file = open_section(reader, RECORDER_SECTION_GLOBAL_CST, -1, NULL);
        else {
            char cst_fname[1096] = {0};
            sprintf(cst_fname, "%s/recorder.cst", reader->logs_dir);
            cst_file = fopen(cst_fname, "rb");
        }
        buf_cst = read_zlib(cst_file);
        reader->csts[0] = (CST*) malloc(sizeof(CST));
        reader_decode_cst(0, buf_cst, reader->csts[0]);
        fclose(cst_file);
        free(buf_cst);

        FILE* f;
        if (reader->num_sections > 0)
            f = open_section(reader, RECORDER_SECTION_CFG_META, -1, NULL);
        else {
            char ug_metadata_fname[1096] = {0};
            sprintf(ug_metadata_fname, "%s/ug.mt", reader->logs_dir);
            f = fopen(ug_metadata_fname, "rb");
        }
        fread(reader->ug_ids, sizeof(int), nprocs, f);
        fread(&reader->num_ugs, sizeof(int), 1, f);
        fclose(f);

        FILE* cfg_file;
        if (reader->num_sections > 0)
            cfg_file = open_section(reader, RECORDER_SECTION_GLOBAL_CFG, -1, NULL);
        else {
            char cfg_fname[1096] = {0};
            sprintf(cfg_fname, "%s/ug.cfg", reader->logs_dir);
            cfg_file = fopen(cfg_fname, "rb");
        }
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
    } else { // interprocess_compression == false

        for(int rank = 0; rank < nprocs; rank++) {
            if (reader->trace_version_major == 2 && reader->trace_version_minor == 3) {
                reader->csts[rank] = (CST*) malloc(sizeof(CST));
                reader_decode_cst_2_3(reader, rank, reader->csts[rank]);
            } else {
                FILE* cst_file;
                if (reader->num_sections > 0)
                    cst_file = open_section(reader, RECORDER_SECTION_RANK_CST, rank, NULL);
                else {
                    char cst_fname[1096] = {0};
                    sprintf(cst_fname, "%s/%d.cst", reader->logs_dir, rank);
                    cst_file = fopen(cst_fname, "rb");
                }
                void* buf_cst = read_zlib(cst_file);
                reader->csts[rank] = (CST*) malloc(sizeof(CST));
                reader_decode_cst(rank, buf_cst, reader->csts[rank]);
                free(buf_cst);
                fclose(cst_file);
            }

            if (reader->trace_version_major == 2 && reader->trace_version_minor == 3) {
                reader->cfgs[rank] = (CFG*) malloc(sizeof(CFG));
                reader_decode_cfg_2_3(reader, rank, reader->cfgs[rank]);
            } else {
                FILE* cfg_file;
                if (reader->num_sections > 0)
                    cfg_file = open_section(reader, RECORDER_SECTION_RANK_CFG, rank, NULL);
                else {
                    char cfg_fname[1096] = {0};
                    sprintf(cfg_fname, "%s/%d.cfg", reader->logs_dir, rank);
                    cfg_file = fopen(cfg_fname, "rb");
                }
                void* buf_cfg = read_zlib(cfg_file);
                reader->cfgs[rank] = (CFG*) malloc(sizeof(CFG));
                reader_decode_cfg(rank, buf_cfg, reader->cfgs[rank]);
                free(buf_cfg);
                fclose(cfg_file);
            }
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
    for(int i = 0; i < reader->supported_funcs; i++)
        free(reader->func_list[i]);
    free(reader->func_list);

    if (reader->sections)
        free(reader->sections);

    memset(reader, 0, sizeof(*reader));
}

const char* recorder_get_func_name(RecorderReader* reader, Record* record) {
    if(record->func_id == RECORDER_USER_FUNCTION)
        return record->args[1];
    return reader->func_list[record->func_id];
}

int recorder_get_func_type(RecorderReader* reader, Record* record) {
    // TODO why not just use func name to determine?
    // is it because the user function may have name
    // collision?
    if(record->func_id < reader->mpi_start_idx)
        return RECORDER_POSIX;
    if(record->func_id < reader->hdf5_start_idx) {
        const char* func_name = recorder_get_func_name(reader, record);
        if(strncmp(func_name, "MPI_File", 8) == 0)
            return RECORDER_MPIIO;
        return RECORDER_MPI;
    }
    if(record->func_id < reader->pnetcdf_start_idx)
        return RECORDER_HDF5;
    if(record->func_id < reader->netcdf_start_idx)
        return RECORDER_PNETCDF;
    if(record->func_id == RECORDER_USER_FUNCTION)
        return RECORDER_FTRACE;
    return RECORDER_NETCDF;
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
                // update timestamps
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

// caller must free the timestamp
// buffer after use
uint32_t* read_timestamp_file(RecorderReader* reader, int rank) {

    char ts_fname[1096] = {0};
    uint32_t* ts_buf = NULL;

    if (reader->trace_version_major==2 && reader->trace_version_minor==3) {
        sprintf(ts_fname, "%s/%d.ts", reader->logs_dir, rank);
        FILE* ts_file = fopen(ts_fname, "rb");
        fseek(ts_file, 0, SEEK_END);
        long filesize = ftell(ts_file);
        fseek(ts_file, 0, SEEK_CUR);

        ts_buf = (uint32_t*) malloc(filesize); 
        fread(ts_buf, 1, filesize, ts_file);
        fclose(ts_file);

        return ts_buf;
    }

    int nprocs = reader->metadata.total_ranks;

    FILE* ts_file;
    if (reader->num_sections > 0)
        ts_file = open_section(reader, RECORDER_SECTION_TIMESTAMPS, -1, NULL);
    else {
        sprintf(ts_fname, "%s/recorder.ts", reader->logs_dir);
        ts_file = fopen(ts_fname, "rb");
    }

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
    if (reader->metadata.ts_compression) {
        ts_buf = (uint32_t*) read_zlib(ts_file);
    } else {
        ts_buf = (uint32_t*) malloc(buf_sizes[rank]); 
        fread(ts_buf, 1, buf_sizes[rank], ts_file);
    }
    fclose(ts_file);
    return ts_buf;
}


void decode_records_core(RecorderReader *reader, int rank,
        void (*user_op)(Record*, void*), void* user_arg, bool free_record) {

    CST* cst = reader_get_cst(reader, rank);
    CFG* cfg = reader_get_cfg(reader, rank);

    reader->prev_tstart = 0.0;

    uint32_t* ts_buf = read_timestamp_file(reader, rank);

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

void recorder_fill_metadata(char* traces_dir, RecorderMetadata *metadata) {
    RecorderReader reader;
    recorder_init_reader(traces_dir, &reader);
    memcpy(metadata, &(reader.metadata), sizeof(RecorderMetadata));
    recorder_free_reader(&reader);
}

typedef struct {
    RecorderReader* reader;
    VerifyIORecord* records;
    size_t used;
    size_t size;
} verifyio_record_filler;

static inline
void verifyio_record_copy_args(VerifyIORecord* vir, Record* r, int arg_count, ...) {
    vir->arg_count = arg_count;
    vir->args = (char**) malloc(sizeof(char*)*vir->arg_count);

    va_list valist;
    va_start(valist, arg_count);
    for(int i = 0; i < arg_count; i++) {
        int arg_idx = va_arg(valist, int);
        vir->args[i] = strdup(r->args[arg_idx]);
    }
    va_end(valist);
}

/**
 * In case of MIP_ANY_SOURCE or MIP_ANY_TAG
 * we need to extract the actual src and tag
 * from the MPI_Status* argument
 */
static inline
int update_mpi_src_tag(VerifyIORecord* vir, Record* r, int src_idx, int tag_idx, int status_idx) {
    int src = atoi(r->args[src_idx]);
    int tag = atoi(r->args[tag_idx]);
    char* status = r->args[status_idx];

    if(src == RECORDER_MPI_ANY_SOURCE) {
        char* p = strstr(status, "_");
        if (p == NULL) {
            printf("Failed to parse source rank from MPI status: %s\n", status);
            fflush(stdout);
            exit(1);
        }
        int len = (int)(p - status);    // including the trailing \0
        free(vir->args[0]);
        vir->args[0] = calloc(len, sizeof(char));
        memcpy(vir->args[0], status+1, len-1);
    } 

    if(tag == RECORDER_MPI_ANY_TAG) {
        char* p1 = strstr(status, "_");
        char* p2 = strstr(status, "]");
        if (p1 == NULL || p2 == NULL) {
            printf("Failed to parse tag from MPI status: %s\n", status);
            fflush(stdout);
            exit(1);
        }
        int len = (int)(p2 - p1);    // including the trailing \0
        free(vir->args[0]);
        vir->args[0] = calloc(len, sizeof(char));
        memcpy(vir->args[1], p1+1, len-1);
    }
}

/**
 * Filtering only needed Record for VerifyIO
 * Also keep setsubt of needed arguments
 * Here, we copy the arguments as they will be
 * freed later.
 * Record* r: [in]
 * VerifyIORecord* vir: [out]
 */
int create_verifyio_record(RecorderReader* reader, Record* r, VerifyIORecord* vir) {

    // return if we will keep the record
    int included = 1;

    // all records keep func_id and call_depth
    vir->func_id = r->func_id;
    vir->call_depth = r->call_depth;
    vir->arg_count = 0;
    vir->args = NULL;

    int func_type = recorder_get_func_type(reader, r);
    const char* func_name = recorder_get_func_name(reader, r);

    if (func_type == RECORDER_HDF5 || func_type == RECORDER_PNETCDF || func_type == RECORDER_NETCDF) {
        // HDF5, PnetCDF and NetCDF functions need no arguments
        vir->arg_count = 0;
        vir->args = NULL;
    } else if (func_type == RECORDER_MPIIO) {
        // MPI-IO functions only need MPI_File handle
        if (strstr(func_name, "MPI_File_open")){
            verifyio_record_copy_args(vir, r, 1, r->arg_count-1);
        } else {
            verifyio_record_copy_args(vir, r, 1, 0);
        }
    } else if (func_type == RECORDER_MPI) {
        if (strcmp(func_name, "MPI_Send") == 0  ||
                strcmp(func_name, "MPI_Ssend") == 0) {
            // dst, tag, comm
            verifyio_record_copy_args(vir, r, 3, 3, 4, 5);
        } else if (strcmp(func_name, "MPI_Issend") == 0 ||
                strcmp(func_name, "MPI_Isend") == 0) {
            // dst, tag, comm, req
            verifyio_record_copy_args(vir, r, 4, 3, 4, 5, 6);
        } else if (strcmp(func_name, "MPI_Recv") == 0) {
            // src, tag, comm
            verifyio_record_copy_args(vir, r, 3, 3, 4, 5);
            update_mpi_src_tag(vir, r, 3, 4, 6);
        } else if (strcmp(func_name, "MPI_Irecv") == 0) {
            // src, tag, comm, req
            verifyio_record_copy_args(vir, r, 4, 3, 4, 5, 6);
        } else if (strcmp(func_name, "MPI_Sendrecv") == 0) {
            // src, dst, stag, rtag, comm
            verifyio_record_copy_args(vir, r, 5, 8, 3, 4, 9, 10);
        } else if (strcmp(func_name, "MPI_Bcast") == 0) {
            // src, comm
            verifyio_record_copy_args(vir, r, 2, 3, 4);
        } else if (strcmp(func_name, "MPI_Ibcast") == 0) {
            // src, comm, req
            verifyio_record_copy_args(vir, r, 3, 3, 4, 5);
        } else if (strcmp(func_name, "MPI_Reduce") == 0) {
            // src, comm
            verifyio_record_copy_args(vir, r, 2, 5, 6);
        } else if (strcmp(func_name, "MPI_Ireduce") == 0) {
            // src, comm, req
            verifyio_record_copy_args(vir, r, 3, 5, 6, 7);
        } else if (strcmp(func_name, "MPI_Gather") == 0) {
            // src, comm
            verifyio_record_copy_args(vir, r, 2, 6, 7);
        } else if (strcmp(func_name, "MPI_Igather") == 0) {
            // src, comm, req
            verifyio_record_copy_args(vir, r, 3, 6, 7, 8);
        } else if (strcmp(func_name, "MPI_Gatherv") == 0) {
            // src, comm
            verifyio_record_copy_args(vir, r, 2, 7, 8);
        } else if (strcmp(func_name, "MPI_Igatherv") == 0) {
            // src, comm, req
            verifyio_record_copy_args(vir, r, 3, 7, 8, 9);
        } else if (strcmp(func_name, "MPI_Barrier") == 0) {
            // comm
            verifyio_record_copy_args(vir, r, 1, 0);
        } else if (strcmp(func_name, "MPI_Alltoall") == 0) {
            // comm
            verifyio_record_copy_args(vir, r, 1, 6);
        } else if (strcmp(func_name, "MPI_Allreduce") == 0 ||
                strcmp(func_name, "MPI_Reduce_scatter") == 0) {
            // comm
            verifyio_record_copy_args(vir, r, 1, 5);
        } else if (strcmp(func_name, "MPI_Allgatherv") == 0) {
            // comm
            verifyio_record_copy_args(vir, r, 1, 7);
        } else if (strcmp(func_name, "MPI_Comm_dup") == 0) {
            // comm, local_rank
            verifyio_record_copy_args(vir, r, 2, 1, 2);
        } else if (strcmp(func_name, "MPI_Comm_split") == 0) {
            // comm, local_rank
            verifyio_record_copy_args(vir, r, 2, 3, 4);
        } else if (strcmp(func_name, "MPI_Comm_split_type") == 0) {
            // comm, local_rank
            verifyio_record_copy_args(vir, r, 2, 4, 5);
        } else if (strcmp(func_name, "MPI_Cart_create") == 0) {
            // comm, local_rank
            verifyio_record_copy_args(vir, r, 2, 5, 6);
        } else if ((strcmp(func_name, "MPI_Cart_sub") == 0) || 
                (strcmp(func_name, "MPI_Comm_create") == 0)) {
            // comm, local_rank
            verifyio_record_copy_args(vir, r, 2, 2, 3);
        } else if ((strcmp(func_name, "MPI_Waitall")) == 0) {
            verifyio_record_copy_args(vir, r, 1, 1);
        } else if ((strcmp(func_name, "MPI_Wait")) == 0) {
            verifyio_record_copy_args(vir, r, 1, 0);
        } else if ((strcmp(func_name, "MPI_Testall")) == 0) {
            verifyio_record_copy_args(vir, r, 1, 1);
            int flag = atoi(r->args[2]);
            if (!flag) sprintf(vir->args[0], "%s", "[]");
        } else if ((strcmp(func_name, "MPI_Test")) == 0) {
            verifyio_record_copy_args(vir, r, 1, 0);
            int flag = atoi(r->args[1]);
            if (!flag) sprintf(vir->args[0], "%s", "[]");
        }
    } else if (func_type == RECORDER_POSIX) {
        // only keep needed *write* *read* POSIX calls
        // additionally, MPI-IO often uses fcntl to lock files.
        if (strstr(func_name, "write") ||
                strstr(func_name, "read")  ||
                strstr(func_name, "fcntl")) {
            verifyio_record_copy_args(vir, r, 1, 0);
        } else if (strstr(func_name, "fsync") ||
                strstr(func_name, "open")  ||
                strstr(func_name, "close")  ||
                strstr(func_name, "fopen")  ||
                strstr(func_name, "fclose")) {
            verifyio_record_copy_args(vir, r, 1, 0);
            //included = 0;
        }
    } else {
        //included = 0;
    }
    return included;
}

void insert_verifyio_record(Record* record, void* arg) {

    verifyio_record_filler* vrf = (verifyio_record_filler*) arg;

    if (vrf->used == vrf->size) {
        vrf->size *= 2;
        vrf->records = realloc(vrf->records, vrf->size * sizeof(VerifyIORecord));
    }

    int included = create_verifyio_record(vrf->reader, record, &(vrf->records[vrf->used]));

    if (included)
        vrf->used++;
    //printf("insert one %s\n", recorder_get_func_name(vrf->reader, record));
}

VerifyIORecord** recorder_read_verifyio_records(char* traces_dir, size_t* num_records) {

    RecorderReader reader;
    recorder_init_reader(traces_dir, &reader);

    VerifyIORecord** records = malloc(sizeof(VerifyIORecord*) * reader.metadata.total_ranks);

    for(int rank = 0; rank < reader.metadata.total_ranks; rank++) {

        CFG* cfg = reader_get_cfg(&reader, rank);

        verifyio_record_filler vrf;
        vrf.reader = &reader;
        vrf.used = 0;
        vrf.size = 1024*1024;
        vrf.records = malloc(vrf.size * sizeof(VerifyIORecord));

        recorder_decode_records(&reader, rank, insert_verifyio_record, &vrf);

        records[rank] = vrf.records;
        num_records[rank] = vrf.used;
    }

    recorder_free_reader(&reader);

    return records;
}

void recorder_free_verifyio_records(VerifyIORecord* records) {
}

