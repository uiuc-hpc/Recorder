#ifndef __RECORDER_PATTERN_RECOGNITION_H_
#define __RECORDER_PATTERN_RECOGNITION_H_
#include "uthash.h"
#include "recorder-logger.h"

#ifndef HAVE_OFF64_T
typedef int64_t off64_t;
#endif

typedef struct offset_map {
    char* func;         // key
    off64_t offset;     // value, this is the previous offset
    UT_hash_handle hh;
} offset_map_t;

off64_t pr_intra_offset(const char* func, off64_t offset);

void interprocess_pattern_recognition(RecorderLogger *logger, char* func_name, int offset_arg_idx);

#endif
