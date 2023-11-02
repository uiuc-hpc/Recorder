#ifndef _RECORDER_READER_PRIVATE_H_
#define _RECORDER_READER_PRIVATE_H_
#include <stdbool.h>
#include "reader.h"

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
    char mpifh[10];
} Interval;

/* Per-file intervals
 * <filename, intervals>
 */
typedef struct IntervalsMap_t {
    char* filename;
    size_t num_intervals;
    Interval *intervals;    // Pointer to Interval, copied from vector<Interval>
} IntervalsMap;



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
void reader_decode_cst(int rank, void* buf, CST* cst);
void reader_decode_cfg(int rank, void* buf, CFG* cfg);
void reader_free_cst(CST *cst);
void reader_free_cfg(CFG *cfg);
CST* reader_get_cst(RecorderReader* reader, int rank);
CFG* reader_get_cfg(RecorderReader* reader, int rank);

Record* reader_cs_to_record(CallSignature *cs);

IntervalsMap* build_offset_intervals(RecorderReader *reader, int *num_files);

#ifdef __cplusplus
}
#endif

#endif
