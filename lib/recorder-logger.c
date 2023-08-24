#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "recorder.h"
#include "recorder-pattern-recognition.h"
#ifdef RECORDER_ENABLE_CUDA_TRACE
#include "recorder-cuda-profiler.h"
#endif

#define VERSION_STR             "2.5.0"
#define DEFAULT_TS_BUFFER_SIZE  (1*1024*1024)       // 1MB


pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool initialized = false;

static RecorderLogger logger;

/**
 * Per-thread FIFO record stack
 * pthread_t tid as key
 *
 * To store cascading calls in tstart order
 * e.g., H5Dwrite -> MPI_File_write_at -> pwrite
 */
struct RecordStack {
    int level;
    pthread_t tid;
    Record *records;
    UT_hash_handle hh;
};
static struct RecordStack *g_record_stack = NULL;


void free_record(Record *record) {
    if(record == NULL)
        return;

    if(record->args) {
        for(int i = 0; i < record->arg_count; i++)
            free(record->args[i]);  // note here we don't use recorder_free
                                    // because the memory was potentially
                                    // allocated by realpath(), strdup() other system calls.
        recorder_free(record->args, sizeof(char*)*record->arg_count);
    }

    record->args = NULL;
    recorder_free(record, sizeof(Record));
}

void write_record(Record *record) {

    // Before pass the record to compose_cs_key()
    // set them to 0 if not needed.
    // TODO: this is a ugly fix for ignoring them, but
    // they still occupy the space in the key.
    if(!logger.log_tid)
        record->tid   = 0;
    if(!logger.log_level)
        record->level = 0;

    int key_len;
    char* key = compose_cs_key(record, &key_len);

    pthread_mutex_lock(&g_mutex);

    CallSignature *entry = NULL;
    HASH_FIND(hh, logger.cst, key, key_len, entry);
    if(entry) {                         // Found
        entry->count++;
        recorder_free(key, key_len);
    } else {                            // Not exist, add to hash table
        entry = (CallSignature*) recorder_malloc(sizeof(CallSignature));
        entry->key = key;
        entry->key_len = key_len;
        entry->rank = logger.rank;
        entry->terminal_id = logger.current_cfg_terminal++;
        entry->count = 1;
        HASH_ADD_KEYPTR(hh, logger.cst, entry->key, entry->key_len, entry);
    }

    append_terminal(&logger.cfg, entry->terminal_id, 1);

    // write timestamps
    uint32_t delta_tstart = (record->tstart-logger.prev_tstart) / logger.ts_resolution;
    uint32_t delta_tend   = (record->tend-logger.prev_tstart)   / logger.ts_resolution;
    logger.prev_tstart = record->tstart;
    logger.ts[logger.ts_index++] = delta_tstart;
    logger.ts[logger.ts_index++] = delta_tend;
    if(logger.ts_index == logger.ts_max_elements) {
        if(!logger.directory_created)
            logger_set_mpi_info(0, 1);
        RECORDER_REAL_CALL(fwrite)(logger.ts, sizeof(uint32_t), logger.ts_max_elements, logger.ts_file);
        logger.ts_index = 0;
    }

    pthread_mutex_unlock(&g_mutex);
}

void logger_record_enter(Record* record) {
    struct RecordStack *rs;
    HASH_FIND(hh, g_record_stack, &record->tid, sizeof(pthread_t), rs);
    if(!rs) {
        rs = recorder_malloc(sizeof(struct RecordStack));
        rs->records = NULL;
        rs->level = 0;
        rs->tid = record->tid;
        HASH_ADD(hh, g_record_stack, tid, sizeof(pthread_t), rs);
    }

    DL_APPEND(rs->records, record);

    record->level = rs->level++;
    record->record_stack = rs;
}

void logger_record_exit(Record* record) {
    struct RecordStack *rs = record->record_stack;
    rs->level--;

    // In most cases, rs->level is 0 and
    // rs->records have only one record
    if(rs->level == 0) {
        Record *current, *tmp;
        DL_FOREACH_SAFE(rs->records, current, tmp) {
            DL_DELETE(rs->records, current);
            write_record(current);
            free_record(current);
        }
    }
}


bool logger_initialized() {
    return initialized;
}

// Traces dir: recorder-YYYYMMDD/appname-username-HHmmSS.fff
void create_traces_dir() {
    if(logger.rank != 0) return;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char* traces_dir = alloca(800);
    char* tmp = realrealpath("/proc/self/exe");
    char* exec_name = basename(tmp);
    char* user_name = getlogin();

    long unsigned int pid = (long unsigned int)getpid();
    char hostname[64] = {0};
    gethostname(hostname, 64);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    sprintf(traces_dir, "recorder-%d%02d%02d/%s-%s-%s-%lu-%02d%02d%02d.%03d/",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, hostname, user_name,
            exec_name, pid, tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec/1000));
    free(tmp);

    const char* base_dir = getenv(RECORDER_TRACES_DIR);
    if(base_dir)
        sprintf(logger.traces_dir, "%s/%s", base_dir, traces_dir);
    else
        strcpy(logger.traces_dir, realrealpath(traces_dir)); // current directory

    if(RECORDER_REAL_CALL(access) (logger.traces_dir, F_OK) != -1)
        RECORDER_REAL_CALL(rmdir) (logger.traces_dir);
    mkpath(logger.traces_dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
}


void logger_set_mpi_info(int mpi_rank, int mpi_size) {

    logger.rank   = mpi_rank;
    logger.nprocs = mpi_size;

    int mpi_initialized;
    PMPI_Initialized(&mpi_initialized);      // MPI_Initialized() is not intercepted
    if(mpi_initialized)
        RECORDER_REAL_CALL(PMPI_Bcast) (&logger.start_ts, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Create traces directory
    create_traces_dir();

    // Rank 0 broadcasts the trace direcotry path
    if(mpi_initialized)
        RECORDER_REAL_CALL(PMPI_Bcast) (logger.traces_dir, sizeof(logger.traces_dir), MPI_BYTE, 0, MPI_COMM_WORLD);

    sprintf(logger.cst_path, "%s/%d.cst", logger.traces_dir, mpi_rank);
    sprintf(logger.cfg_path, "%s/%d.cfg", logger.traces_dir, mpi_rank);

    if(mpi_initialized)
        RECORDER_REAL_CALL(PMPI_Barrier) (MPI_COMM_WORLD);

    char ts_filename[1024];
    sprintf(ts_filename, "%s/%d.ts", logger.traces_dir, mpi_rank);
    logger.ts_file = RECORDER_REAL_CALL(fopen) (ts_filename, "wb");

    logger.directory_created = true;
}


void logger_init() {
    // Map the functions we will use later
    // We did not intercept fprintf
    MAP_OR_FAIL(fopen);
    MAP_OR_FAIL(fflush);
    MAP_OR_FAIL(fclose);
    MAP_OR_FAIL(fwrite);
    MAP_OR_FAIL(rmdir);
    MAP_OR_FAIL(access);
    MAP_OR_FAIL(PMPI_Barrier);
    MAP_OR_FAIL(PMPI_Bcast);
    MAP_OR_FAIL(PMPI_Recv);
    MAP_OR_FAIL(PMPI_Send);

    double global_tstart = recorder_wtime();

    // Initialize CUDA profiler
    #ifdef RECORDER_ENABLE_CUDA_TRACE
    cuda_profiler_init();
    #endif

    // Initialize the global values
    logger.rank   = 0;
    logger.nprocs = 1;
    logger.start_ts = global_tstart;
    logger.prev_tstart = logger.start_ts;
    logger.cst = NULL;
    sequitur_init(&logger.cfg);
    logger.current_cfg_terminal = 0;
    logger.directory_created = false;
    logger.log_tid   = 0;
    logger.log_level = 1;
    logger.interprocess_compression = 0;

    // ts buffer size in MB
    const char* buffer_size_str = getenv(RECORDER_BUFFER_SIZE);
    size_t buffer_size = DEFAULT_TS_BUFFER_SIZE;
    if(buffer_size_str)
        buffer_size = atoi(buffer_size_str) * 1024 * 1024;

    logger.ts = recorder_malloc(buffer_size);
    logger.ts_max_elements = buffer_size / sizeof(uint32_t);    // make sure its can be divided by 2
    if(logger.ts_max_elements % 2 != 0) logger.ts_max_elements += 1;
    logger.ts_index = 0;
    logger.ts_resolution = 1e-7; // 100ns

    const char* time_resolution_str = getenv(RECORDER_TIME_RESOLUTION);
    if(time_resolution_str)
        logger.ts_resolution = atof(time_resolution_str);


    const char* log_tid_str = getenv(RECORDER_LOG_TID);
    if(log_tid_str)
        logger.log_tid = atoi(log_tid_str);
    const char* log_level_str = getenv(RECORDER_LOG_LEVEL);
    if(log_level_str)
        logger.log_level = atoi(log_level_str);
    const char* interprocess_compression = getenv(RECORDER_INTERPROCESS_COMPRESSION);
    if(interprocess_compression)
        logger.interprocess_compression = atoi(interprocess_compression);


    initialized = true;
}

void cleanup_record_stack() {
    struct RecordStack *rs, *tmp;
    HASH_ITER(hh, g_record_stack, rs, tmp) {
        HASH_DEL(g_record_stack, rs);
        assert(rs->records == NULL);
        recorder_free(rs, sizeof(struct RecordStack));
    }
}

void save_global_metadata() {
    if (logger.rank != 0) return;

    char metadata_filename[1024] = {0};
    sprintf(metadata_filename, "%s/recorder.mt", logger.traces_dir);
    FILE* metafh = RECORDER_REAL_CALL(fopen) (metadata_filename, "wb");
    RecorderMetadata metadata = {
        .time_resolution     = logger.ts_resolution,
        .total_ranks         = logger.nprocs,
        .start_ts            = logger.start_ts,
        .ts_buffer_elements  = logger.ts_max_elements,
        .ts_compression_algo = TS_COMPRESSION_NO,
        .interprocess_compression = logger.interprocess_compression,
    };
    RECORDER_REAL_CALL(fwrite)(&metadata, sizeof(RecorderMetadata), 1, metafh);

    for(int i = 0; i < sizeof(func_list)/sizeof(char*); i++) {
        const char *funcname = get_function_name_by_id(i);
        if(strstr(funcname, "PMPI_"))       // replace PMPI with MPI
            RECORDER_REAL_CALL(fwrite)(funcname+1, strlen(funcname)-1, 1, metafh);
        else
            RECORDER_REAL_CALL(fwrite)(funcname, strlen(funcname), 1, metafh);
        RECORDER_REAL_CALL(fwrite)("\n", sizeof(char), 1, metafh);
    }
    RECORDER_REAL_CALL(fflush)(metafh);
    RECORDER_REAL_CALL(fclose)(metafh);

    char version_filename[1024];
    sprintf(version_filename, "%s/VERSION", logger.traces_dir);
    FILE* version_file = RECORDER_REAL_CALL(fopen) (version_filename, "w");
    RECORDER_REAL_CALL(fwrite) (VERSION_STR, 5, 1, version_file);
    RECORDER_REAL_CALL(fflush)(version_file);
    RECORDER_REAL_CALL(fclose)(version_file);
}

void logger_finalize() {

    if(!logger.directory_created)
        logger_set_mpi_info(0, 1);

    initialized = false;

    #ifdef RECORDER_ENABLE_CUDA_TRACE
    cuda_profiler_exit();
    #endif

    if(logger.ts_index > 0)
        RECORDER_REAL_CALL(fwrite)(logger.ts, sizeof(int), logger.ts_index, logger.ts_file);

    RECORDER_REAL_CALL(fflush)(logger.ts_file);
    RECORDER_REAL_CALL(fclose)(logger.ts_file);
    recorder_free(logger.ts, sizeof(uint32_t)*logger.ts_max_elements);

    /*
    interprocess_pattern_recognition("lseek64");
    interprocess_pattern_recognition("lseek");
    interprocess_pattern_recognition("pread");
    interprocess_pattern_recognition("PMPI_File_write_at");
    interprocess_pattern_recognition("PMPI_File_read_at");
    */
    //interprocess_pattern_recognition(&logger, "PMPI_File_write_at", 1);
    //interprocess_pattern_recognition(&logger, "pwrite", 3);

    cleanup_record_stack();
    if(logger.interprocess_compression) {
        save_cst_merged(&logger);
        save_cfg_merged(&logger);
    } else {
        save_cst_local(&logger);
        save_cfg_local(&logger);
    }
    cleanup_cst(logger.cst);
    sequitur_cleanup(&logger.cfg);

    if(logger.rank == 0) {
        save_global_metadata();

        fprintf(stderr, "[Recorder] trace files have been written to %s\n", logger.traces_dir);
        RECORDER_REAL_CALL(fflush)(stderr);
    }
}

