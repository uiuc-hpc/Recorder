#ifndef __RECORDER_OPTIMIZATION_H
#define __RECORDER_OPTIMIZATION_H

#include "recorder.h"
#include "uthash.h"
#include "recorder-analysis.h"

int apply_optimizations(RecorderLogger* logger, Knowledge* knowledge, Record* record, const char * func_name, int timestep, char file_name[512]);

#endif