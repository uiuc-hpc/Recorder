#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_
#include <stdbool.h>
#include "recorder-log-format.h"


#define POSIX_SEMANTICS 0
#define COMMIT_SEMANTICS 1
#define SESSION_SEMANTICS 2



typedef struct RecorderReader_t {

    RecorderMetadata metadata;

    char func_list[256][64];
    char logs_dir[1024];
} RecorderReader;

typedef struct Interval_t {
    int rank;
    int seqId;              // The sequence id of the I/O call
    double tstart;
    size_t offset;
    size_t count;
    bool isRead;
} Interval;

typedef struct IntervalsMap_t {
    char* filename;
    size_t num_intervals;
    Interval *intervals;    // Pointer to Interval, copied from vector<Interval>

    int *num_opens;         // num_opens[rank] is list of number of opens for rank
    int *num_closes;
    int *num_commits;

    double **topens;        // topens[rank] is a list of open timestamps for rank
    double **tcloses;
    double **tcommits;
} IntervalsMap;

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct CallSignature_t {
    int terminal;
    int key_len;
    char* key;
} CallSignature;

typedef struct CST_t {
    int rank;
    int entries;
    CallSignature *cst_list;
} CST;

typedef struct RuleHash_t {
    int rule_id;
    int *rule_body;     // 2i+0: val of symbol i,  2i+1: exp of symbol i
    int symbols;        // There are a total of 2*symbols integers in the rule body
    UT_hash_handle hh;
} RuleHash;

typedef struct CFG_t {
    int rank;
    int rules;
    RuleHash* cfg_head;
} CFG;


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

void recorder_read_cst(RecorderReader *reader, int rank, CST *cst);
void recorder_free_cst(CST *cst);

void recorder_read_cfg(RecorderReader *reader, int rank, CFG *cfg);
void recorder_free_cfg(CFG *cfg);

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
void recorder_decode_records(RecorderReader* reader, CST *cst, CFG *cfg,
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


IntervalsMap* build_offset_intervals(RecorderReader reader, int *num_files, int semantics);
#ifdef __cplusplus
}
#endif
#endif
