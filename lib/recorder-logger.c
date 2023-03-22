#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "recorder.h"
#ifdef RECORDER_ENABLE_CUDA_TRACE
#include "recorder-cuda-profiler.h"
#endif

#define VERSION_STR             "2.4.0"
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

    // Before pass the record to compose_call_key()
    // set them to 0 if not needed.
    // TODO: this is a ugly fix for ignoring them, but
    // they still occupy the space in the key.
    if(!logger.log_tid)
        record->tid   = 0;
    if(!logger.log_level)
        record->level = 0;

    int key_len;
    char* key = compose_call_key(record, &key_len);

    pthread_mutex_lock(&g_mutex);

    RecordHash *entry = NULL;
    HASH_FIND(hh, logger.cst, key, key_len, entry);
    if(entry) {                         // Found
        entry->count++;
        recorder_free(key, key_len);
    } else {                            // Not exist, add to hash table
        entry = (RecordHash*) recorder_malloc(sizeof(RecordHash));
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
        strcpy(logger.traces_dir, traces_dir); // current directory

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
    logger.log_tid   = 1;
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

struct lseek_entry {
    int offset_key_start;
    int offset_key_end;
    RecordHash* cs;
};

void offset_pattern_check(char* func_name) {

    int func_count = 0;
    unsigned char filter_func_id = get_function_id_by_name(func_name);

    RecordHash *entry, *tmp;
    HASH_ITER(hh, logger.cst, entry, tmp) {
        void* ptr = entry->key+sizeof(pthread_t);
        unsigned char func_id;
        memcpy(&func_id, ptr, sizeof(func_id));
        if(func_id == filter_func_id)
            func_count++;
    }


    struct lseek_entry *lseek_entries = malloc(sizeof(struct lseek_entry) * func_count);
    long int *lseek_offsets = malloc(sizeof(long int) * func_count);

    Record r;
    size_t arg_offset = sizeof(pthread_t) + sizeof(r.func_id) + sizeof(r.level) + sizeof(r.arg_count) + sizeof(int);

    int idx = 0;
    HASH_ITER(hh, logger.cst, entry, tmp) {
        void* ptr = entry->key+sizeof(pthread_t);

        unsigned char func_id;
        memcpy(&func_id, ptr, sizeof(r.func_id));

        if(func_id == filter_func_id) {
            char* key = (char*) entry->key;

            int start = 0, end = 0;
            for(int i = arg_offset; i < entry->key_len; i++) {
                if(key[i] == ' ') {
                    start = i;
                    break;
                }
            }
            for(int i = start+1; i < entry->key_len; i++) {
                if(key[i] == ' ') {
                    end = i;
                    break;
                }
            }
            assert(end > start);
            char offset_str[64] = {0};
            memcpy(offset_str, key+start+1, end-start-1);
            long int offset = atol(offset_str);

            lseek_offsets[idx] = offset;
            lseek_entries[idx].offset_key_start = start;
            lseek_entries[idx].offset_key_end   = end;
            lseek_entries[idx].cs = entry;
            assert(end > start);

            idx++;
        }
    }

    MAP_OR_FAIL(PMPI_Comm_split);
    MAP_OR_FAIL(PMPI_Comm_size);
    MAP_OR_FAIL(PMPI_Comm_rank);
    MAP_OR_FAIL(PMPI_Comm_free);
    MAP_OR_FAIL(PMPI_Allgather);

    MPI_Comm comm;
    int comm_size, comm_rank;
    RECORDER_REAL_CALL(PMPI_Comm_split)(MPI_COMM_WORLD, func_count, logger.rank, &comm);
    RECORDER_REAL_CALL(PMPI_Comm_size)(comm, &comm_size);
    RECORDER_REAL_CALL(PMPI_Comm_rank)(comm, &comm_rank);

    if(comm_rank == 0)
        printf("%s count: %d, comm size: %d\n", func_name, func_count, comm_size);

    if(comm_size > 2) {
        long int *all_offsets = calloc(comm_size*(func_count), sizeof(long int));
        RECORDER_REAL_CALL(PMPI_Allgather)(lseek_offsets, func_count, MPI_LONG,
                                            all_offsets, func_count, MPI_LONG, comm);

        // Fory every lseek(), i.e, i-th lseek()
        // check if it is the form of offset = a * rank + b;
        for(int i = 0; i < func_count; i++) {

            long int o1 = all_offsets[i];
            long int o2 = all_offsets[i+func_count];
            long int a = o2 - o1;
            long int b = o1;
            int same_pattern = 1;
            for(int r = 0; r < comm_size; r++) {
                long int o = all_offsets[i+func_count*r];
                if(o != a*r+b) {
                    same_pattern = 0;
                    break;
                }
            }

            // Everyone has the same pattern of offset
            // Then modify the call signature to store
            // the pattern instead of the actuall offset
            // TODO we should store a and b, but now we
            // store a only
            if(same_pattern) {
                HASH_DEL(logger.cst, lseek_entries[i].cs);

                int start = lseek_entries[i].offset_key_start;
                int end   = lseek_entries[i].offset_key_end;

                char* tmp = calloc(64, 1);
                sprintf(tmp, "%ld*r+%ld", a, b);

                if(comm_rank == 0)
                    printf("pattern recognized %d: offset = %ld*rank+%ld\n", lseek_entries[i].cs->terminal_id, a, b);

                int old_keylen = lseek_entries[i].cs->key_len;
                int new_keylen = old_keylen - (end-start-1) + strlen(tmp);
                int new_arg_strlen = new_keylen - arg_offset;

                void* newkey = malloc(new_keylen);
                void* oldkey = lseek_entries[i].cs->key;

                memcpy(newkey, oldkey, start+1);
                memcpy(newkey+arg_offset-sizeof(int), &new_arg_strlen, sizeof(int));
                memcpy(newkey+start+1, tmp, strlen(tmp));
                memcpy(newkey+start+1+strlen(tmp), oldkey+end, old_keylen-end);

                lseek_entries[i].cs->key = newkey;
                lseek_entries[i].cs->key_len = new_keylen;
                HASH_ADD_KEYPTR(hh, logger.cst, lseek_entries[i].cs->key, lseek_entries[i].cs->key_len, lseek_entries[i].cs);


                free(oldkey);
                free(tmp);
            }
        }
        free(all_offsets);
    }

    RECORDER_REAL_CALL(PMPI_Comm_free)(&comm);
    free(lseek_offsets);
    free(lseek_entries);
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

    offset_pattern_check("lseek64");
    offset_pattern_check("lseek");
    offset_pattern_check("PMPI_File_write_at");
    offset_pattern_check("PMPI_File_read_at");

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

        fprintf(stdout, "[Recorder] trace files have been written to %s\n", logger.traces_dir);
        RECORDER_REAL_CALL(fflush)(stderr);
    }
}

