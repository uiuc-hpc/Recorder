#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_
#include <stdbool.h>
#include "recorder-log-format.h"


#define POSIX_SEMANTICS 0
#define COMMIT_SEMANTICS 1
#define SESSION_SEMANTICS 2

typedef struct RecorderReader_t {
    RecorderGlobalDef RGD;
    RecorderLocalDef *RLDs;
    Record **records;       //records[rank] is a list of records of that rank
    char func_list[256][64];
} RecorderReader;

typedef struct Interval_t {
    int rank;
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


void read_global_metadata(char* path, RecorderGlobalDef *RGD);

void read_local_metadata(char* path, RecorderLocalDef *RLD);

Record* read_records(char* path, int len, RecorderGlobalDef *RGD);

void decompress_records(Record* records, int len);

void recorder_read_traces(const char* logs_dir, RecorderReader *reader);
void release_resources(RecorderReader *reader);


IntervalsMap* build_offset_intervals(RecorderReader reader, int *num_files, int semantics);

#endif
