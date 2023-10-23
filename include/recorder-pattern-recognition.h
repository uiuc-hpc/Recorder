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

/* all functions in this unit starts with iopr_
 * (I/O Pattern Recognition)
 *
 */

// intraprocess
off64_t iopr_intraprocess(const char* func, off64_t offset);

// interprocess
void    iopr_interprocess(RecorderLogger* logger);

#endif
