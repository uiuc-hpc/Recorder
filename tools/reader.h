#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_
#include <stdbool.h>
#include "recorder-log-format.h"


#define POSIX_SEMANTICS 0
#define COMMIT_SEMANTICS 1
#define SESSION_SEMANTICS 2

typedef struct RecorderReader_t {
    RecorderGlobalDef RGD;
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




typedef struct CallSignature_t {
    int terminal;
    int key_len;
    char* key;
} CallSignature;


typedef struct RuleHash_t {
    int rule_id;
    int *rule_body;     // 2i+0: val of symbol i,  2i+1: exp of symbol i
    int symbols;        // There are a total of 2*symbols integers in the rule body
    UT_hash_handle hh;
} RuleHash;

void recorder_init_reader(const char* logs_dir, RecorderReader *reader);
CallSignature* recorder_read_cst(RecorderReader *reader, int rank, int* entries);
void recorder_free_cst(CallSignature* cst, int entries);
RuleHash* recorder_read_cfg(RecorderReader *reader, int rank);
void recorder_free_cfg(RuleHash* cfg);
void recorder_free_reader(RecorderReader *reader);


IntervalsMap* build_offset_intervals(RecorderReader reader, int *num_files, int semantics);

#endif
