#ifndef __RECORDER_OPTIMIZATION_H
#define __RECORDER_OPTIMIZATION_H

#include "recorder.h"
#include "uthash.h"
#include "recorder-analysis.h"

int apply_optimizations(Knowledge* knowledge,  hid_t dcplID, hid_t faplID, int nprocs);

#endif