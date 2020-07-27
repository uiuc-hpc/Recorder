#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_

#include "../../include/recorder-log-format.h"

typedef struct RecorderReader_t {
    RecorderGlobalDef RGD;
    RecorderLocalDef *RLDs;
    Record **records;
} RecorderReader;


void read_global_metadata(char* path, RecorderGlobalDef *RGD);

void read_local_metadata(char* path, RecorderLocalDef *RLD);

Record* read_records(char* path, int len, RecorderGlobalDef *RGD);

void decompress_records(Record* records, int len);

#endif
