#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/recorder-log-format.h"

/*
 * Read the global metada file and write the information in *global_def
 */
void read_global_metadata(const char *path, RecorderGlobalDef *global_def);

/*
 * Read one local metadata file (one rank)
 * And output in *local_def
 */
void read_local_metadata(const char* path, RecorderLocalDef *local_def);


/*
 * Read one record (one line) from the trace file FILE* f
 * return 0 on success, -1 if read EOF
 * Note, this function does not perform decoding or decompression
 * in: FILE* f
 * in: RecorderGlobalDef global_def
 * out: record
 */
int read_record(FILE *f, RecorderGlobalDef global_def, RecorderLocalDef local_def, Record *record);


/*
 * Write all original records (encoded and compressed) to text file
 */
void decode(Record *records, RecorderGlobalDef global_def, RecorderLocalDef local_def);


/*
 * Read one rank's log file
 * This function does not perform decoding and decompression
 * return array of records in the file
 */
Record* read_logfile(const char* logfile_path, RecorderGlobalDef global_def, RecorderLocalDef local_def);

int is_posix_read_function(Record *record);

int is_posix_write_function(Record *record);

size_t get_io_size(Record record);

size_t get_io_offset();


typedef struct RecorderBandwidthInfo_t {
    size_t read_size;
    size_t write_size;
    double read_bandwidth;
    double write_bandwidth;
    double read_cost;
    double write_cost;
} RecorderBandwidthInfo;

typedef struct RecorderAccessPattern_t {
    bool read_after_read;
    bool read_after_write;
    bool write_after_write;
    bool write_after_read;
} RecorderAccessPattern;

void get_bandwidth_info(Record *records, RecorderLocalDef local_def, RecorderBandwidthInfo *bwinfo);
