#ifndef __RECORDER_ANALYSIS_H
#define __RECORDER_ANALYSIS_H

#include "recorder.h"
#include "uthash.h"

typedef struct RuleConfidence_t {
    int rule;
    int count;
    int probability;
    UT_hash_handle hh;
} RuleConfidence;

int recorder_analysis(RecorderLogger* logger, Record* record, CallSignature* entry);

#endif 
