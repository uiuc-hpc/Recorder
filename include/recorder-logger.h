#ifndef __RECORDER_LOGGER_H
#define __RECORDER_LOGGER_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "recorder-sequitur.h"
#include "uthash.h"

/**
 * In post-processing, reader.c will check
 * the version of the traces. a matching 
 * major.minor guarantees compatibility
 */
#define RECORDER_VERSION_MAJOR  2
#define RECORDER_VERSION_MINOR  5
#define RECORDER_VERSION_PATCH  0

#define RECORDER_POSIX          0
#define RECORDER_MPIIO          1
#define RECORDER_MPI            2
#define RECORDER_HDF5           3
#define RECORDER_FTRACE         4

#define RECORDER_USER_FUNCTION  255



/* For each function call in the trace file */
typedef struct Record_t {
    double tstart, tend;
    unsigned char call_depth;
    unsigned char func_id;      // we have about 200 functions in total
    unsigned char arg_count;
    char **args;                // Store all arguments in array
    pthread_t tid;

    void* record_stack;         // per-thread record stack of cascading calls
    struct Record_t *prev, *next;
} Record;


/*
 * Call Signature
 */
typedef struct CallSignature_t {
    void *key;
    int key_len;
    int rank;
    int terminal_id;
    int count;
    UT_hash_handle hh;
} CallSignature;


typedef struct RecorderMetadata_t {
    int    total_ranks;
    bool   posix_tracing;
    bool   mpi_tracing;
    bool   mpiio_tracing;
    bool   hdf5_tracing;
    bool   store_tid;            // Wether to store thread id
    bool   store_call_depth;     // Wether to store the call depth
    double start_ts;
    double time_resolution;
    int    ts_buffer_elements;
    bool   ts_compression;              // whether to compress timestamps (using zlib)
    bool   interprocess_compression;    // interprocess compression of cst/cfg
    bool   interprocess_pattern_recognition;
    bool   intraprocess_pattern_recognition;
} RecorderMetadata;


/**
 * Per-process CST and CFG
 */
typedef struct RecorderLogger_t {
    int rank;
    int nprocs;
    int num_records;            // total number of records stored by this rank

    bool directory_created;

    int current_cfg_terminal;

    Grammar        cfg;
    CallSignature* cst;

    char traces_dir[512];
    char cst_path[1024];
    char cfg_path[1024];

    double    start_ts;
    double    prev_tstart;      // delta compression for timestamps
    FILE*     ts_file;
    uint32_t* ts;               // memory buffer for timestamps (tstart, tend-tstart)
    int       ts_index;         // current position of ts buffer, spill to file once full.
    int       ts_max_elements;  // max elements can be stored in the buffer
    double    ts_resolution;
    bool      ts_compression;

    bool      store_tid;            // Wether to store thread id
    bool      store_call_depth;     // Wether to store the call depth
    bool      interprocess_compression; // Wether to perform interprocess compression of cst/cfg
    bool      interprocess_pattern_recognition; 
    bool      intraprocess_pattern_recognition; 
} RecorderLogger;





/* recorder-logger.c */
void logger_init();
void logger_set_mpi_info(int mpi_rank, int mpi_size);
void logger_finalize();
bool logger_initialized();
void logger_record_enter(Record *record);
void logger_record_exit(Record *record);
bool logger_intraprocess_pattern_recognition();
bool logger_interprocess_pattern_recognition();

void free_record(Record *record);
// TODO only used by ftrace logger
// Need to see how to replace it
void write_record(Record* record);


/* recorder-cst-cfg.c */
int  cs_key_args_start();
int  cs_key_args_strlen(Record* record);
int  cs_key_length(Record* record);
char* compose_cs_key(Record *record, int* key_len);
Record* cs_to_record(CallSignature* cs);
void cleanup_cst(CallSignature* cst);
void save_cst_local(RecorderLogger* logger);
void save_cst_merged(RecorderLogger* logger);
void save_cfg_local(RecorderLogger* logger);
void save_cfg_merged(RecorderLogger* logger);




static const char* func_list[] = {
    // POSIX I/O - 72 functions
    "creat",        "creat64",      "open",         "open64",   "close",
    "write",        "read",         "lseek",        "lseek64",  "pread",
    "pread64",      "pwrite",       "pwrite64",     "readv",    "writev",
    "mmap",         "mmap64",       "fopen",        "fopen64",  "fclose",
    "fwrite",       "fread",        "ftell",        "fseek",    "fsync",
    "fdatasync",    "__xstat",      "__xstat64",    "__lxstat", "__lxstat64",
    "__fxstat",     "__fxstat64",   "getcwd",       "mkdir",    "rmdir",
    "chdir",        "link",         "linkat",       "unlink",   "symlink",
    "symlinkat",    "readlink",     "readlinkat",   "rename",   "chmod",
    "chown",        "lchown",       "utime",        "opendir",  "readdir",
    "closedir",     "rewinddir",    "mknod",        "mknodat",  "fcntl",
    "dup",          "dup2",         "pipe",         "mkfifo",   "umask",
    "fdopen",       "fileno",       "access",       "faccessat","tmpfile",
    "remove",       "truncate",     "ftruncate",    "msync",
    "fseeko",       "ftello",       "fflush",


    // MPI 84 functions
    "MPI_File_close",              "MPI_File_set_size",       "MPI_File_iread_at",
    "MPI_File_iread",              "MPI_File_iread_shared",   "MPI_File_iwrite_at",
    "MPI_File_iwrite",             "MPI_File_iwrite_shared",  "MPI_File_open",
    "MPI_File_read_all_begin",     "MPI_File_read_all",       "MPI_File_read_at_all",
    "MPI_File_read_at_all_begin",  "MPI_File_read_at",        "MPI_File_read",
    "MPI_File_read_ordered_begin", "MPI_File_read_ordered",   "MPI_File_read_shared",
    "MPI_File_set_view",           "MPI_File_sync",           "MPI_File_write_all_begin",
    "MPI_File_write_all",          "MPI_File_write_at_all_begin", "MPI_File_write_at_all",
    "MPI_File_write_at",           "MPI_File_write",          "MPI_File_write_ordered_begin",
    "MPI_File_write_ordered",      "MPI_File_write_shared", 
    "MPI_Finalized",
    "MPI_Wtime",                   "MPI_Comm_rank",           "MPI_Comm_size",
    "MPI_Get_processor_name",      "MPI_Get_processor_name",  "MPI_Comm_set_errhandler",
    "MPI_Barrier",                 "MPI_Bcast",               "MPI_Gather",
    "MPI_Gatherv",                 "MPI_Scatter",             "MPI_Scatterv",
    "MPI_Allgather",               "MPI_Allgatherv",          "MPI_Alltoall",
    "MPI_Reduce",                  "MPI_Allreduce",           "MPI_Reduce_scatter",
    "MPI_Scan",                    "MPI_Type_commit",         "MPI_Type_contiguous",
    "MPI_Type_extent",             "MPI_Type_free",           "MPI_Type_hindexed",
    "MPI_Op_create",               "MPI_Op_free",             "MPI_Type_get_envelope",
    "MPI_Type_size",               "MPI_Type_create_darray",
    // Added 2019/01/07
    "MPI_Cart_rank",               "MPI_Cart_create",         "MPI_Cart_get",
    "MPI_Cart_shift",              "MPI_Wait",                "MPI_Send",
    "MPI_Recv",                    "MPI_Sendrecv",            "MPI_Isend",
    "MPI_Irecv",
    // Added 2020/02/24
    "MPI_Info_create",             "MPI_Info_set",            "MPI_Info_get",
    // Added 2020/08/06
    "MPI_Waitall",                 "MPI_Waitsome",            "MPI_Waitany",
    "MPI_Ssend",
    // Added 2020/08/17
    "MPI_Comm_split",              "MPI_Comm_dup",            "MPI_Comm_create",
    // Added 2020/08/27
    "MPI_File_seek",               "MPI_File_seek_shared",
    // Added 2020/11/05, 2020/11/13
    "MPI_File_get_size",           "MPI_Ibcast",
    // Added 2020/12/18
    "MPI_Test",                    "MPI_Testall",             "MPI_Testsome",
    "MPI_Testany",                 "MPI_Ireduce",             "MPI_Iscatter",
    "MPI_Igather",                 "MPI_Ialltoall",
    // Added 2021/01/25
    "MPI_Comm_free",               "MPI_Cart_sub",            "MPI_Comm_split_type",

    // HDF5 I/O - 74 functions
    "H5Fcreate",            "H5Fopen",              "H5Fclose",     "H5Fflush", // File interface
    "H5Gclose",             "H5Gcreate1",           "H5Gcreate2",   // Group interface
    "H5Gget_objinfo",       "H5Giterate",           "H5Gopen1",
    "H5Gopen2",             "H5Dclose",             "H5Dcreate1",
    "H5Dcreate2",           "H5Dget_create_plist",  "H5Dget_space", // Dataset interface
    "H5Dget_type",          "H5Dopen1",             "H5Dopen2",
    "H5Dread",              "H5Dwrite",             "H5Dset_extent",
    "H5Sclose",                                                     // Dataspace interface
    "H5Screate",            "H5Screate_simple",     "H5Sget_select_npoints",
    "H5Sget_simple_extent_dims", "H5Sget_simple_extent_npoints", "H5Sselect_elements",
    "H5Sselect_hyperslab",  "H5Sselect_none",       "H5Tclose",     // Datatype interface
    "H5Tcopy",              "H5Tget_class",         "H5Tget_size",
    "H5Tset_size",          "H5Tcreate",            "H5Tinsert",
    "H5Aclose",             "H5Acreate1",           "H5Acreate2",   // Attribute interface
    "H5Aget_name",          "H5Aget_num_attrs",     "H5Aget_space",
    "H5Aget_type",          "H5Aopen",              "H5Aopen_idx",
    "H5Aopen_name",         "H5Aread",              "H5Awrite",
    "H5Pclose",             "H5Pcreate",            "H5Pget_chunk", // Property List interface
    "H5Pget_mdc_config",    "H5Pset_alignment",     "H5Pset_chunk",
    "H5Pset_dxpl_mpio",     "H5Pset_fapl_core",     "H5Pset_fapl_mpio",
    "H5Pset_istore_k",      "H5Pset_mdc_config",
    "H5Pset_meta_block_size","H5Lexists",           "H5Lget_val",   // Link interface
    "H5Literate",            "H5Literate1",         "H5Literate2",           
    "H5Oclose",              "H5Oget_info",                         // Object interface
    "H5Oget_info_by_name",   "H5Oopen",
    "H5Pset_coll_metadata_write",                   "H5Pget_coll_metadata_write",   // collective metadata
    "H5Pset_all_coll_metadata_ops",                 "H5Pget_all_coll_metadata_ops"
};

#endif /* __RECORDER_LOGGER_H */
