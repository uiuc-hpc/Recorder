#ifndef __RECORDER_TIMESTAMPS_H_
#define __RECORDER_TIMESTAMPS_H_
#include <stdio.h>
#include <stdint.h>
#include "recorder-logger.h"

/* 
 * get the per-rank timestamp filename
 */
void ts_get_filename(RecorderLogger* logger, char* ts_filename);

/*
 * write out the current buffer to the per-rank timestamp filename
 */
void ts_write_out(RecorderLogger* logger);

/* 
 * merge per-rank timestamp files into a single file
 */
void ts_merge_files(RecorderLogger* logger);

#endif
