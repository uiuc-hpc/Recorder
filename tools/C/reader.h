#ifndef _RECORDER_READER_H_
#define _RECORDER_READER_H_

#include <vector>
#include "../../include/recorder-log-format.h"

typedef struct RecorderReader_t {
    RecorderGlobalDef RGD;
    std::vector<RecorderLocalDef> RLDs;
    std::vector<std::vector<Record*>> records;
} RecorderReader;


void read_global_metadata(char* path, RecorderGlobalDef *RGD);

void read_local_metadata(char* path, RecorderLocalDef *RLD);

std::vector<Record*> read_records(char* path);

void decompress_records(std::vector<Record*> records);

#endif
