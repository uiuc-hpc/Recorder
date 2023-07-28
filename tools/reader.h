#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_
#include <stdbool.h>
#include "recorder-logger.h"

// keep in sync with VERSION_STR in lib/recorder-logger.c
// equal (major, minor) is needed for compatibility
#define VERSION_MAJOR 		2
#define VERSION_MINOR 		5
#define VERSION_PATCH 		0

#define POSIX_SEMANTICS 	0
#define COMMIT_SEMANTICS 	1
#define SESSION_SEMANTICS	2

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct Interval_t {
    int rank;
    int seqId;              // The sequence id of the I/O call
    double tstart;
    size_t offset;
    size_t count;
    bool isRead;
} Interval;

/* Per-file intervals
 * <filename, intervals>
 */
typedef struct IntervalsMap_t {
    char* filename;
    size_t num_intervals;
    Interval *intervals;    // Pointer to Interval, copied from vector<Interval>
} IntervalsMap;

typedef struct CST_t {
    int rank;
    int entries;
    CallSignature *cs_list; // CallSignature is defined in recorder-logger.h
} CST;

typedef struct RuleHash_t {
    int rule_id;
    int *rule_body;         // 2i+0: val of symbol i,  2i+1: exp of symbol i
    int symbols;            // There are a total of 2*symbols integers in the rule body
    UT_hash_handle hh;
} RuleHash;

typedef struct CFG_t {
    int rank;
    int rules;
    RuleHash* cfg_head;
} CFG;

typedef struct RecorderReader_t {

    RecorderMetadata metadata;

    char func_list[256][64];
    char logs_dir[1024];

    int mpi_start_idx;
    int hdf5_start_idx;

    double prev_tstart;

	int   num_ugs;			// number of unique grammars
	int*  ug_ids;			// index of unique grammar in cfgs
	CST** csts;
	CFG** cfgs;
} RecorderReader;


/**
 * Similar but simplified structure
 * for use by recorder-viz
 */
typedef struct PyRecord_t {
    double tstart, tend;
    unsigned char level;
    unsigned char func_id;
    int tid;
    unsigned char arg_count;
    char **args;
} PyRecord;



void recorder_init_reader(const char* logs_dir, RecorderReader *reader);
void recorder_free_reader(RecorderReader *reader);


/**
 * Read CST and CFG from files to RecorderReader
 *
 * With interprocess compression, we have
 * one merged CST and multiple CFG files.
 * Each CFG file stores a unique grammar.
 *
 * Without interprocess compression, we have
 * one CST and one CFG file per process.
 *
 * ! These two functions should only be used internally
 * recorder_get_cst_cfg() can be used to perform
 * custom tasks with CST and CFG
 */
void recorder_read_cst(RecorderReader *reader, int rank);
void recorder_read_cfg(RecorderReader *reader, int rank);
void recorder_free_cst(CST *cst);
void recorder_free_cfg(CFG *cfg);

void recorder_get_cst_cfg(RecorderReader* reader, int rank, CST** cst, CFG** cfg);


Record* recorder_cs_to_record(CallSignature *cs);
void recorder_free_record(Record* r);


/**
 * This function reads all records of a rank
 *
 * For each record decoded, the user_op() function
 * will be called with the decoded record
 *
 * void user_op(Record *r, void* user_arg);
 * void* user_arg can be used to pass in user argument.
 *
 */
void recorder_decode_records_core(RecorderReader* reader, CST *cst, CFG *cfg,
                             void (*user_op)(Record* r, void* user_arg), void* user_arg, bool free_record);
void recorder_decode_records(RecorderReader* reader, int rank,
                             void (*user_op)(Record* r, void* user_arg), void* user_arg);


void recorder_decode_records2(RecorderReader *reader, CST *cst, CFG *cfg,
                             void (*user_op)(Record* r, void* user_arg), void* user_arg);


const char* recorder_get_func_name(RecorderReader* reader, Record* record);

/*
 * Return one of the follows (mutual exclusive) :
 *  - RECORDER_POSIX
 *  - RECORDER_MPIIO
 *  - RECORDER_MPI
 *  - RECORDER_HDF5
 *  - RECORDER_FTRACE
 */
int recorder_get_func_type(RecorderReader* reader, Record* record);


IntervalsMap* build_offset_intervals(RecorderReader *reader, int *num_files);

#ifdef __cplusplus
}
#endif

#endif
