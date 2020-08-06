#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_

#include <stdbool.h>
#include "../../include/recorder-log-format.h"


typedef struct RecorderReader_t {
    RecorderGlobalDef RGD;
    RecorderLocalDef *RLDs;
    Record **records;       //records[rank] is a list of records of that rank
} RecorderReader;

typedef struct Interval_t {
    int rank;
    size_t offset;
    size_t count;
    bool isRead;
} Interval;

typedef struct IntervalsMap_t {
    char* filename;
    size_t num_intervals;
    Interval *intervals; // Pointer to Interval, copied from vector<Interval>
} IntervalsMap;



void read_global_metadata(char* path, RecorderGlobalDef *RGD);

void read_local_metadata(char* path, RecorderLocalDef *RLD);

Record* read_records(char* path, int len, RecorderGlobalDef *RGD);

void decompress_records(Record* records, int len);



void recorder_read_traces(const char* logs_dir, RecorderReader *reader);
void release_resources(RecorderReader *reader);

IntervalsMap* build_offset_intervals(RecorderReader reader, int *num_files);

#endif
