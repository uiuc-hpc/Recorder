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
#define RECORDER_VERSION_MAJOR  3
#define RECORDER_VERSION_MINOR  0
#define RECORDER_VERSION_PATCH  0

#define RECORDER_POSIX          0
#define RECORDER_MPIIO          1
#define RECORDER_MPI            2
#define RECORDER_HDF5           3
#define RECORDER_FTRACE         4
#define RECORDER_PNETCDF        5
#define RECORDER_NETCDF         6

#define RECORDER_USER_FUNCTION  255


#define RECORDER_MPI_ANY_SOURCE -1
#define RECORDER_MPI_ANY_TAG    -2

/* For each function call in the trace file */
typedef struct Record_t {
    double tstart;
    double tend;
    unsigned char call_depth;
    int func_id;
    unsigned char arg_count;
    char **args;                // Store all arguments in array
    pthread_t tid;
    void* res;                  // return value

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


typedef struct RuleConfidence_t {
    int rule;
    int count;
    int probability;
    UT_hash_handle hh;
} RuleConfidence;


typedef struct RecorderMetadata_t {
    int    total_ranks;
    bool   posix_tracing;
    bool   mpi_tracing;
    bool   mpiio_tracing;
    bool   hdf5_tracing;
    bool   pnetcdf_tracing;
    bool   netcdf_tracing;
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
    int pattern_id;
       
    Grammar        cfg;
    CallSignature* cst;
    RuleConfidence* cfg_conf;

    char traces_dir[512];
    char cst_path[1024];
    char cfg_path[1024];
    char pattern[512];

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



void push(int x, int inp_array[]);
void pop(int inp_array[]);
void show(int inp_array[]);
void analysis(CallSignature *entry);

/* recorder-logger.c */
void logger_init();
void logger_set_mpi_info(int mpi_rank, int mpi_size);
void logger_finalize();
bool logger_initialized();
void logger_record_enter(Record *record);
void logger_record_exit(Record *record, int real_arg_count, void** real_args);
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
    "MPI_File_iwrite",             "MPI_File_iwrite_shared",
    "MPI_File_iwrite_all",         "MPI_File_iwrite_at_all",
    "MPI_File_open",
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
    "MPI_Ssend",                   "MPI_Issend",
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

    // HDF5 1.14.4.2
    "H5Dread", "H5Adelete_by_idx", "H5Fflush",
    "H5Pget_chunk", "H5Pset_buffer", "H5LTget_attribute",
    "H5Pset_all_coll_metadata_ops", "H5Pmodify_filter", "H5Fmount",
    "H5Rget_obj_type1", "H5IMmake_palette", "H5Pget_driver_config_str",
    "H5Ewalk2", "H5Oget_info3", "H5Eclose_stack",
    "H5PTfree_vlen_buff", "H5Ecreate_stack", "H5VLget_connector_id_by_name",
    "H5TBinsert_record", "H5Otoken_cmp", "H5PLremove",
    "H5Pset_hyper_vector_size", "H5Sget_select_hyper_nblocks", "H5Aget_info_by_idx",
    "H5Adelete_by_name", "H5Iinc_ref", "H5Ovisit_by_name1",
    "H5PLreplace", "H5Dwrite_chunk", "H5Sselect_copy",
    "H5Pset_dset_no_attrs_hint", "H5Ssel_iter_create", "H5Rdestroy",
    "H5Oget_info_by_name3", "H5Pget_obj_track_times", "H5Sselect_none",
    "H5Lcopy", "H5Fset_libver_bounds", "H5Dwrite_multi",
    "H5Fset_latest_format", "H5Eset_current_stack", "H5Sget_select_elem_npoints",
    "H5Pset_type_conv_cb", "H5Pset_fapl_log", "H5Orefresh",
    "H5Acreate2", "H5Dextend", "H5TBread_table",
    "H5Sis_regular_hyperslab", "H5Pset_modify_write_buf", "H5Scopy",
    "H5Sdecode", "H5LTset_attribute_string", "H5Pset_sieve_buf_size",
    "H5Pget_class_parent", "H5Sselect_shape_same", "H5Pget_fapl_ros3",
    "H5TBwrite_records", "H5Sget_select_elem_pointlist", "H5LTread_bitfield_value",
    "H5IMis_palette", "H5Freopen", "H5Pset_dxpl_mpio_chunk_opt",
    "H5Punregister", "H5Pset_szip", "H5Fget_create_plist",
    "H5Tget_class", "H5EScancel", "H5Tcommit1",
    "H5Sselect_adjust", "H5Pset_filter_callback", "H5Tdecode",
    "H5Rget_obj_type2", "H5Pcopy", "H5Dget_num_chunks",
    "H5Pget_coll_metadata_write", "H5IMmake_image_24bit", "H5set_free_list_limits",
    "H5Iget_name", "H5Scombine_hyperslab", "H5Aget_name",
    "H5TBget_table_info", "H5Aread", "H5Pset_deflate",
    "H5Ropen_object", "H5Pget_mcdt_search_cb", "H5LTmake_dataset_double",
    "H5LTtext_to_dtype", "H5Pset_multi_type", "H5Eget_num",
    "H5Pget_fill_time", "H5LTset_attribute_double", "H5Pget_cache",
    "H5ESwait", "H5VLclose", "H5Eget_class_name",
    "H5Pset_dxpl_mpio_chunk_opt_ratio", "H5LTget_attribute_int", "H5Pset_obj_track_times",
    "H5TBwrite_fields_name", "H5atclose", "H5Rget_name",
    "H5Pget_userblock", "H5Gopen1", "H5Dfill",
    "H5Pset_relax_file_integrity_checks", "H5Sget_simple_extent_ndims", "H5Pset_fapl_direct",
    "H5Pget_fclose_degree", "H5Gmove", "H5Aopen_idx",
    "H5Eprint2", "H5Aclose", "H5DSget_label",
    "H5Pget_mpio_actual_io_mode", "H5Pset_selection_io", "H5LDget_dset_type_size",
    "H5LTget_attribute_double", "H5IMunlink_palette", "H5Tcommit_async",
    "H5Pset_external", "H5Aget_storage_size", "H5Pset_fapl_subfiling",
    "H5Pget_alloc_time", "H5Pget_small_data_block_size", "H5Zfilter_avail",
    "H5Rget_file_name", "H5Pget_elink_prefix", "H5allocate_memory",
    "H5PLinsert", "H5Pget_no_selection_io_cause", "H5LTget_dataset_ndims",
    "H5Pget_class", "H5Eget_minor", "H5Ropen_region",
    "H5Pget_preserve", "H5Ovisit1", "H5Pget_gc_references",
    "H5Ssel_iter_close", "H5Pall_filters_avail", "H5Pexist",
    "H5Fget_vfd_handle", "H5LRcopy_region", "H5Acreate1",
    "H5Tdetect_class", "H5ESget_err_info", "H5Eclose_msg",
    "H5Rcreate_attr", "H5Olink", "H5PTget_next",
    "H5Lexists", "H5Fget_info1", "H5Pset_fapl_windows",
    "H5Pset_mdc_image_config", "H5Eregister_class", "H5Dget_chunk_info_by_coord",
    "H5Pget_size", "H5Pget_virtual_count", "H5Pget_edc_check",
    "H5VLquery_optional", "H5dont_atexit", "H5Pget_vol_id",
    "H5IMis_image", "H5PLget", "H5IMlink_palette",
    "H5Pfill_value_defined", "H5Oopen_by_token", "H5Pget_virtual_dsetname",
    "H5Pget", "H5Ropen_attr", "H5Dwrite",
    "H5LTset_attribute_uchar", "H5Pget_vlen_mem_manager", "H5ESregister_insert_func",
    "H5Dcreate2", "H5TBAget_fill", "H5Pset_driver_by_name",
    "H5Pget_fapl_ioc", "H5Tcommitted", "H5Pset_attr_creation_order",
    "H5PLsize", "H5Lunpack_elink_val", "H5Fget_eoa",
    "H5Sget_simple_extent_dims", "H5PTget_dataset", "H5TBadd_records_from",
    "H5Eset_auto2", "H5TBcombine_tables", "H5Pget_filter1",
    "H5Pset_dxpl_mpio_collective_opt", "H5Lget_val_by_idx", "H5Oclose",
    "H5Fget_fileno", "H5Pget_fapl_onion", "H5Fcreate",
    "H5Pset_userblock", "H5Oget_comment", "H5Pset_chunk",
    "H5DSis_attached", "H5LTmake_dataset_string", "H5Dread_chunk",
    "H5VLget_connector_id", "H5Pset_layout", "H5Sselect_valid",
    "H5Tencode", "H5Pget_dset_no_attrs_hint", "H5Pset_btree_ratios",
    "H5Iget_type", "H5LRmake_dataset", "H5Pclose",
    "H5Fclear_elink_file_cache", "H5Pget_multi_type", "H5TBmake_table",
    "H5Pequal", "H5Pset_evict_on_close", "H5Dopen2",
    "H5Ldelete_by_idx", "H5LTset_attribute_long_long", "H5Oget_native_info_by_idx",
    "H5Pget_create_intermediate_group", "H5Epush1", "H5Gset_comment",
    "H5Pget_data_transform", "H5Pget_sizes", "H5Lget_name_by_idx",
    "H5PTread_packets", "H5Oget_info1", "H5DSattach_scale",
    "H5Zunregister", "H5Pset_cache", "H5resize_memory",
    "H5TBdelete_field", "H5Padd_merge_committed_dtype_path", "H5Tget_create_plist",
    "H5Pset_edc_check", "H5Pset_vlen_mem_manager", "H5Grefresh",
    "H5Dvlen_get_buf_size", "H5Pset_libver_bounds", "H5Tcommit_anon",
    "H5Oset_comment_by_name", "H5LTmake_dataset_short", "H5Epop",
    "H5IMget_palette", "H5LTmake_dataset", "H5Pset_coll_metadata_write",
    "H5Sencode1", "H5Rget_type", "H5Pget_selection_io",
    "H5Pset_fill_value", "H5Pset_alignment", "H5Gclose",
    "H5Sis_simple", "H5DSdetach_scale", "H5Dcreate1",
    "H5Pget_copy_object", "H5Ssel_iter_get_seq_list", "H5Pset_dataset_io_hyperslab_selection",
    "H5Pset_chunk_opts", "H5PTcreate_fl", "H5Aiterate1",
    "H5Topen1", "H5Sselect_project_intersection", "H5LTset_attribute_int",
    "H5Pget_elink_acc_flags", "H5Pset_page_buffer_size", "H5Fclose",
    "H5Giterate", "H5Tequal", "H5Oget_native_info_by_name",
    "H5Pset_file_space", "H5Requal", "H5DOwrite_chunk",
    "H5PTget_num_packets", "H5Dget_access_plist", "H5Pset_mdc_log_options",
    "H5Adelete", "H5Pset_shuffle", "H5Pset_file_space_strategy",
    "H5Eauto_is_v2", "H5Aopen_name", "H5Ecreate_msg",
    "H5Odecr_refcount", "H5Rget_region", "H5Scombine_select",
    "H5Pset_driver", "H5Dget_create_plist", "H5Pset_fapl_split",
    "H5Gget_num_objs", "H5Dget_chunk_info", "H5PTcreate_index",
    "H5Aget_num_attrs", "H5Oenable_mdc_flushes", "H5LTdtype_to_text",
    "H5LRcopy_reference", "H5Pget_modify_write_buf", "H5TBget_field_info",
    "H5Pget_filter_by_id2", "H5Pget_fapl_splitter", "H5Zregister",
    "H5LTget_attribute_uint", "H5Fget_file_image", "H5Gget_objinfo",
    "H5Gcreate1", "H5Oopen_by_addr", "H5Pget_mpi_params",
    "H5is_library_threadsafe", "H5Sget_select_npoints", "H5LTset_attribute_ulong",
    "H5Pset_fapl_ros3", "H5VLget_connector_id_by_value", "H5Pget_nprops",
    "H5Pset_shared_mesg_phase_change", "H5Pget_metadata_read_attempts", "H5Sselect_hyperslab",
    "H5Oexists_by_name", "H5Pget_alignment", "H5Eget_msg",
    "H5Pset_attr_phase_change", "H5Rcreate_region", "H5Iget_ref",
    "H5Acreate_by_name", "H5Dflush", "H5LTmake_dataset_float",
    "H5Pset_fapl_onion", "H5Pget_mpio_no_collective_cause", "H5Tflush",
    "H5Pget_mpio_actual_chunk_opt_mode", "H5PTget_type", "H5Gmove2",
    "H5Pset_fapl_ros3_token", "H5LDget_dset_elmts", "H5Pset_scaleoffset",
    "H5Premove", "H5Fis_hdf5", "H5Aopen_by_name",
    "H5Eget_auto2", "H5Pget_core_write_tracking", "H5Sselect_intersect_block",
    "H5Sextent_equal", "H5Pget_virtual_srcspace", "H5LTget_dataset_info",
    "H5Fget_free_sections", "H5Tset_size", "H5Rget_obj_type3",
    "H5TBAget_title", "H5garbage_collect", "H5Pget_char_encoding",
    "H5Lget_val", "H5Pregister1", "H5Aget_info_by_name",
    "H5Pget_file_space_page_size", "H5Pregister2", "H5Idec_ref",
    "H5Pset_vol", "H5PLset_loading_state", "H5Iis_valid",
    "H5LTget_attribute_string", "H5Screate", "H5Pget_fapl_multi",
    "H5DSget_num_scales", "H5EScreate", "H5LTget_attribute_info",
    "H5Eclear2", "H5Pget_object_flush_cb", "H5Pset_shared_mesg_nindexes",
    "H5Pget_driver", "H5Gget_linkval", "H5Pset_driver_by_value",
    "H5LTread_dataset_float", "H5Oincr_refcount", "H5PTclose",
    "H5Pget_elink_file_cache_size", "H5TBinsert_field", "H5Fincrement_filesize",
    "H5Pget_btree_ratios", "H5Fget_dset_no_attrs_hint", "H5Pset_elink_fapl",
    "H5VLregister_connector_by_value", "H5Pinsert1", "H5Pset_file_space_page_size",
    "H5Pget_nfilters", "H5Sget_select_type", "H5Gget_create_plist",
    "H5LTset_attribute_ullong", "H5Rget_obj_name", "H5Gget_info_by_name",
    "H5Tcreate", "H5LTread_dataset_long", "H5PTcreate",
    "H5Awrite", "H5LTget_attribute_ulong", "H5Pset_alloc_time",
    "H5Pget_hyper_vector_size", "H5VLis_connector_registered_by_value", "H5Dget_space",
    "H5Otoken_to_str", "H5DSiterate_scales", "H5Pget_fapl_hdfs",
    "H5Pset", "H5Diterate", "H5Pset_fletcher32",
    "H5Pget_file_image_callbacks", "H5Pget_fapl_family", "H5Pget_attr_creation_order",
    "H5Pcopy_prop", "H5Lget_info_by_idx1", "H5Aget_space",
    "H5Aget_name_by_idx", "H5Dget_space_status", "H5Aget_create_plist",
    "H5ESget_op_counter", "H5LTset_attribute_float", "H5Pget_elink_fapl",
    "H5Pget_layout", "H5Freset_page_buffering_stats", "H5Pget_fill_value",
    "H5Pset_small_data_block_size", "H5Epush2", "H5Sget_simple_extent_npoints",
    "H5Lget_info2", "H5Pget_sieve_buf_size", "H5Fget_filesize",
    "H5Lget_info1", "H5Pget_fapl_mirror", "H5Pget_mdc_image_config",
    "H5Oget_comment_by_name", "H5Sget_simple_extent_type", "H5Pset_nbit",
    "H5Pset_meta_block_size", "H5Fget_access_plist", "H5Dset_extent",
    "H5LTset_attribute_char", "H5Pget_sym_k", "H5Dget_offset",
    "H5LTget_attribute_ushort", "H5Pget_filter2", "H5Pget_attr_phase_change",
    "H5Pget_evict_on_close", "H5LTget_attribute_short", "H5Pset_object_flush_cb",
    "H5Screate_simple", "H5Pset_fill_time", "H5Sget_regular_hyperslab",
    "H5Pget_mdc_log_options", "H5Pisa_class", "H5Ssel_iter_reset",
    "H5Pcreate_class", "H5Tclose", "H5Pset_core_write_tracking",
    "H5Sselect_all", "H5LTread_dataset_short", "H5Ewalk1",
    "H5Soffset_simple", "H5Fget_freespace", "H5Aget_type",
    "H5Pget_shared_mesg_index", "H5Gget_info", "H5Fget_obj_ids",
    "H5Pget_fapl_ros3_token", "H5Arename", "H5DSis_scale",
    "H5get_libversion", "H5Pget_nlinks", "H5Oget_info2",
    "H5Pset_create_intermediate_group", "H5Pget_file_locking", "H5Pget_virtual_filename",
    "H5Oget_info_by_idx3", "H5Pset_fapl_core", "H5Pset_mpi_params",
    "H5Glink2", "H5Oget_info_by_idx2", "H5Rcreate_object",
    "H5Ovisit_by_name3", "H5Pencode1", "H5Pset_fapl_family",
    "H5LRread_region", "H5Pset_elink_prefix", "H5Pset_family_offset",
    "H5Pget_external", "H5VLobject_is_native", "H5TBwrite_fields_index",
    "H5LTread_dataset_char", "H5Pset_sym_k", "H5DSwith_new_ref",
    "H5Iregister_future", "H5Pget_relax_file_integrity_checks", "H5Tlock",
    "H5VLunregister_connector", "H5Pset_fapl_ioc", "H5Pget_istore_k",
    "H5VLis_connector_registered_by_name", "H5Fget_page_buffering_stats", "H5LTset_attribute_ushort",
    "H5Pget_fapl_core", "H5IMget_image_info", "H5ESget_count",
    "H5Tget_size", "H5Pset_metadata_read_attempts", "H5Pset_file_locking",
    "H5Arename_by_name", "H5Pset_dxpl_mpio", "H5Aexists_by_name",
    "H5Eget_major", "H5Pencode2", "H5Pset_filter",
    "H5Sset_extent_none", "H5Gopen2", "H5Tcommit2",
    "H5Dread_multi", "H5Aexists", "H5LTget_attribute_long_long",
    "H5Dscatter", "H5Fset_dset_no_attrs_hint", "H5Pset_fapl_splitter",
    "H5Pset_elink_cb", "H5Pget_file_space_strategy", "H5Premove_filter",
    "H5Dget_chunk_storage_size", "H5Oget_info_by_name1", "H5PTis_varlen",
    "H5Pset_fclose_degree", "H5LRget_region_info", "H5LTpath_valid",
    "H5Topen_async", "H5Pset_mcdt_search_cb", "H5Zget_filter_info",
    "H5Pget_fapl_direct", "H5VLget_connector_name", "H5Oget_info_by_name2",
    "H5Tget_super", "H5Eget_auto1", "H5Rget_attr_name",
    "H5Pget_virtual_vspace", "H5Ovisit2", "H5Gcreate2",
    "H5PTis_valid", "H5Dvlen_reclaim", "H5VLregister_connector_by_name",
    "H5LTread_dataset_double", "H5Iget_file_id", "H5Pset_fapl_mirror",
    "H5Dchunk_iter", "H5Ldelete", "H5IMread_image",
    "H5Gflush", "H5PTopen", "H5Gcreate_anon",
    "H5PTget_index", "H5LRcreate_ref_to_all", "H5TBread_records",
    "H5Pcreate", "H5Pget_page_buffer_size", "H5Pset_gc_references",
    "H5close", "H5Fdelete", "H5Sclose",
    "H5Oflush", "H5Pget_file_space", "H5DSset_label",
    "H5Aopen", "H5Sset_extent_simple", "H5Pinsert2",
    "H5PTset_index", "H5Dgather", "H5Pget_fapl_mpio",
    "H5Pset_dxpl_mpio_chunk_opt_num", "H5LTread_region", "H5Sextent_copy",
    "H5Lget_info_by_idx2", "H5Sget_select_bounds", "H5Pget_filter_by_id1",
    "H5Aopen_by_idx", "H5LTset_attribute_long", "H5Pget_shared_mesg_phase_change",
    "H5PLget_loading_state", "H5Pset_data_transform", "H5Eclear1",
    "H5Pset_elink_acc_flags", "H5Drefresh", "H5Odisable_mdc_flushes",
    "H5LTget_attribute_float", "H5LTmake_dataset_int", "H5Pdecode",
    "H5LTget_attribute_uchar", "H5Fget_info2", "H5Oopen_by_idx",
    "H5Gget_info_by_idx", "H5LTget_attribute_ndims", "H5DOappend",
    "H5Tclose_async", "H5LTread_dataset_int", "H5Rdereference1",
    "H5LDget_dset_dims", "H5Sget_select_hyper_blocklist", "H5Pget_driver_info",
    "H5Sencode2", "H5Lcreate_ud", "H5Pget_fapl_subfiling",
    "H5Tcopy", "H5Dget_storage_size", "H5Dopen1",
    "H5Pget_elink_cb", "H5LTmake_dataset_char", "H5Pget_meta_block_size",
    "H5Eappend_stack", "H5Pset_nlinks", "H5Pset_copy_object",
    "H5Smodify_select", "H5Pget_shared_mesg_nindexes", "H5Lcreate_soft",
    "H5DSget_scale_name", "H5Pget_mdc_config", "H5LTfind_attribute",
    "H5TBdelete_record", "H5Pget_class_name", "H5Pset_sizes",
    "H5Pset_fapl_mpio", "H5Pget_all_coll_metadata_ops", "H5Funmount",
    "H5Eset_auto1", "H5Pset_char_encoding", "H5Pget_version",
    "H5Gget_objtype_by_idx", "H5is_library_terminating", "H5IMget_npalettes",
    "H5Fis_accessible", "H5open", "H5ESget_err_status",
    "H5Pset_mdc_config", "H5Oset_comment", "H5LTget_attribute_char",
    "H5Pset_fapl_sec2", "H5Pget_type_conv_cb", "H5PLappend",
    "H5Sselect_elements", "H5Fopen", "H5Ocopy",
    "H5DOread_chunk", "H5Aget_info", "H5Pget_chunk_opts",
    "H5Aiterate_by_name", "H5LTmake_dataset_long", "H5Pset_fapl_stdio",
    "H5Pset_file_image_callbacks", "H5TBread_fields_name", "H5Gget_comment",
    "H5Oopen", "H5Lmove", "H5Trefresh",
    "H5Eunregister_class", "H5Rcopy", "H5Pget_dxpl_mpio",
    "H5Eget_current_stack", "H5Glink", "H5Pget_buffer",
    "H5IMget_palette_info", "H5Ovisit3", "H5Pget_vol_info",
    "H5Pget_external_count", "H5TBappend_records", "H5Gget_objname_by_idx",
    "H5IMmake_image_8bit", "H5Fget_intent", "H5VLwrap_register",
    "H5Eprint1", "H5Pset_fapl_hdfs", "H5LTfind_dataset",
    "H5Tget_native_type", "H5Dget_type", "H5check_version",
    "H5ESclose", "H5Oare_mdc_flushes_disabled", "H5LTopen_file_image",
    "H5LTset_attribute_uint", "H5Dclose", "H5Aiterate2",
    "H5Rcreate", "H5Pget_file_image", "H5Pset_virtual",
    "H5get_free_list_sizes", "H5Gunlink", "H5Topen2",
    "H5Lcreate_external", "H5TBread_fields_index", "H5PLprepend",
    "H5Fget_name", "H5Pset_fapl_multi", "H5LTset_attribute_short",
    "H5LTget_attribute_ullong", "H5Pclose_class", "H5Oget_native_info",
    "H5Otoken_from_str", "H5Pget_libver_bounds", "H5Pset_istore_k",
    "H5Pfree_merge_committed_dtype_paths", "H5ESregister_complete_func", "H5Rdereference2",
    "H5ESfree_err_info", "H5LTcopy_region", "H5LTread_dataset",
    "H5Pset_preserve", "H5Piterate", "H5Pget_family_offset",
    "H5Pget_vol_cap_flags", "H5LRcreate_region_references", "H5LTget_attribute_long",
    "H5free_memory", "H5ESget_err_count", "H5Fget_obj_count",
    "H5LTread_dataset_string", "H5Ovisit_by_name2", "H5Pset_file_image",
    "H5Pset_shared_mesg_index", "H5Lcreate_hard", "H5PTappend",
    "H5Oget_info_by_idx1", "H5DSset_scale", "H5Dcreate_anon",
    "H5Pget_actual_selection_io_mode", "H5Pset_elink_file_cache_size", "H5Aclose_async",
    "H5Acreate_async", "H5Acreate_by_name_async", "H5Aexists_async",
    "H5Aexists_by_name_async", "H5Aopen_async", "H5Aopen_by_idx_async",
    "H5Aopen_by_name_async", "H5Aread_async", "H5Arename_async",
    "H5Arename_by_name_async", "H5Awrite_async", "H5Dcreate_async",
    "H5Dopen_async", "H5Dget_space_async", "H5Dread_async",
    "H5Dread_multi_async", "H5Dwrite_async", "H5Dwrite_multi_async",
    "H5Dset_extent_async", "H5Dclose_async", "H5Fcreate_async",
    "H5Fopen_async", "H5Freopen_async", "H5Fflush_async",
    "H5Fclose_async", "H5Gcreate_async", "H5Gopen_async",
    "H5Gget_info_async", "H5Gget_info_by_name_async", "H5Gget_info_by_idx_async",
    "H5Gclose_async", "H5Lcreate_hard_async", "H5Lcreate_soft_async",
    "H5Ldelete_async", "H5Ldelete_by_idx_async", "H5Lexists_async",
    "H5Literate_async", "H5Mcreate_async", "H5Mopen_async",
    "H5Mclose_async", "H5Mput_async", "H5Mget_async",
    "H5Oopen_async", "H5Oopen_by_idx_async", "H5Oget_info_by_name_async",
    "H5Ocopy_async", "H5Oclose_async", "H5Oflush_async",
    "H5Orefresh_async", "H5Ropen_object_async", "H5Ropen_region_async",
    "H5Ropen_attr_async",

    // PnetCDF 1.13.0
    "ncmpi_strerror", "ncmpi_strerrno", "ncmpi_create",
    "ncmpi_open", "ncmpi_delete", "ncmpi_enddef",
    "ncmpi__enddef", "ncmpi_redef", "ncmpi_set_default_format",
    "ncmpi_inq_default_format", "ncmpi_flush", "ncmpi_sync",
    "ncmpi_sync_numrecs", "ncmpi_abort", "ncmpi_begin_indep_data",
    "ncmpi_end_indep_data", "ncmpi_close", "ncmpi_set_fill",
    "ncmpi_def_dim", "ncmpi_def_var", "ncmpi_rename_dim",
    "ncmpi_rename_var", "ncmpi_inq_libvers", "ncmpi_inq",
    "ncmpi_inq_format", "ncmpi_inq_file_format", "ncmpi_inq_version",
    "ncmpi_inq_striping", "ncmpi_inq_ndims", "ncmpi_inq_nvars",
    "ncmpi_inq_num_rec_vars", "ncmpi_inq_num_fix_vars", "ncmpi_inq_natts",
    "ncmpi_inq_unlimdim", "ncmpi_inq_dimid", "ncmpi_inq_dim",
    "ncmpi_inq_dimname", "ncmpi_inq_dimlen", "ncmpi_inq_var",
    "ncmpi_inq_varid", "ncmpi_inq_varname", "ncmpi_inq_vartype",
    "ncmpi_inq_varndims", "ncmpi_inq_vardimid", "ncmpi_inq_varnatts",
    "ncmpi_inq_varoffset", "ncmpi_inq_put_size", "ncmpi_inq_get_size",
    "ncmpi_inq_header_size", "ncmpi_inq_header_extent", "ncmpi_inq_file_info",
    "ncmpi_get_file_info", "ncmpi_inq_malloc_size", "ncmpi_inq_malloc_max_size",
    "ncmpi_inq_malloc_list", "ncmpi_inq_files_opened", "ncmpi_inq_recsize",
    "ncmpi_def_var_fill", "ncmpi_inq_var_fill", "ncmpi_inq_path",
    "ncmpi_inq_att", "ncmpi_inq_attid", "ncmpi_inq_atttype",
    "ncmpi_inq_attlen", "ncmpi_inq_attname", "ncmpi_copy_att",
    "ncmpi_rename_att", "ncmpi_del_att", "ncmpi_put_att",
    "ncmpi_put_att_text", "ncmpi_put_att_schar", "ncmpi_put_att_short",
    "ncmpi_put_att_int", "ncmpi_put_att_float", "ncmpi_put_att_double",
    "ncmpi_put_att_longlong", "ncmpi_get_att", "ncmpi_get_att_text",
    "ncmpi_get_att_schar", "ncmpi_get_att_short", "ncmpi_get_att_int",
    "ncmpi_get_att_float", "ncmpi_get_att_double", "ncmpi_get_att_longlong",
    "ncmpi_put_att_uchar", "ncmpi_put_att_ubyte", "ncmpi_put_att_ushort",
    "ncmpi_put_att_uint", "ncmpi_put_att_long", "ncmpi_put_att_ulonglong",
    "ncmpi_get_att_uchar", "ncmpi_get_att_ubyte", "ncmpi_get_att_ushort",
    "ncmpi_get_att_uint", "ncmpi_get_att_long", "ncmpi_get_att_ulonglong",
    "ncmpi_fill_var_rec", "ncmpi_put_var1", "ncmpi_put_var1_all",
    "ncmpi_put_var1_text", "ncmpi_put_var1_text_all", "ncmpi_put_var1_schar",
    "ncmpi_put_var1_schar_all", "ncmpi_put_var1_short", "ncmpi_put_var1_short_all",
    "ncmpi_put_var1_int", "ncmpi_put_var1_int_all", "ncmpi_put_var1_float",
    "ncmpi_put_var1_float_all", "ncmpi_put_var1_double", "ncmpi_put_var1_double_all",
    "ncmpi_put_var1_longlong", "ncmpi_put_var1_longlong_all", "ncmpi_get_var1",
    "ncmpi_get_var1_all", "ncmpi_get_var1_text", "ncmpi_get_var1_text_all",
    "ncmpi_get_var1_schar", "ncmpi_get_var1_schar_all", "ncmpi_get_var1_short",
    "ncmpi_get_var1_short_all", "ncmpi_get_var1_int", "ncmpi_get_var1_int_all",
    "ncmpi_get_var1_float", "ncmpi_get_var1_float_all", "ncmpi_get_var1_double",
    "ncmpi_get_var1_double_all", "ncmpi_get_var1_longlong", "ncmpi_get_var1_longlong_all",
    "ncmpi_put_var1_uchar", "ncmpi_put_var1_uchar_all", "ncmpi_put_var1_ushort",
    "ncmpi_put_var1_ushort_all", "ncmpi_put_var1_uint", "ncmpi_put_var1_uint_all",
    "ncmpi_put_var1_long", "ncmpi_put_var1_long_all", "ncmpi_put_var1_ulonglong",
    "ncmpi_put_var1_ulonglong_all", "ncmpi_get_var1_uchar", "ncmpi_get_var1_uchar_all",
    "ncmpi_get_var1_ushort", "ncmpi_get_var1_ushort_all", "ncmpi_get_var1_uint",
    "ncmpi_get_var1_uint_all", "ncmpi_get_var1_long", "ncmpi_get_var1_long_all",
    "ncmpi_get_var1_ulonglong", "ncmpi_get_var1_ulonglong_all", "ncmpi_put_var",
    "ncmpi_put_var_all", "ncmpi_put_var_text", "ncmpi_put_var_text_all",
    "ncmpi_put_var_schar", "ncmpi_put_var_schar_all", "ncmpi_put_var_short",
    "ncmpi_put_var_short_all", "ncmpi_put_var_int", "ncmpi_put_var_int_all",
    "ncmpi_put_var_float", "ncmpi_put_var_float_all", "ncmpi_put_var_double",
    "ncmpi_put_var_double_all", "ncmpi_put_var_longlong", "ncmpi_put_var_longlong_all",
    "ncmpi_get_var", "ncmpi_get_var_all", "ncmpi_get_var_text",
    "ncmpi_get_var_text_all", "ncmpi_get_var_schar", "ncmpi_get_var_schar_all",
    "ncmpi_get_var_short", "ncmpi_get_var_short_all", "ncmpi_get_var_int",
    "ncmpi_get_var_int_all", "ncmpi_get_var_float", "ncmpi_get_var_float_all",
    "ncmpi_get_var_double", "ncmpi_get_var_double_all", "ncmpi_get_var_longlong",
    "ncmpi_get_var_longlong_all", "ncmpi_put_var_uchar", "ncmpi_put_var_uchar_all",
    "ncmpi_put_var_ushort", "ncmpi_put_var_ushort_all", "ncmpi_put_var_uint",
    "ncmpi_put_var_uint_all", "ncmpi_put_var_long", "ncmpi_put_var_long_all",
    "ncmpi_put_var_ulonglong", "ncmpi_put_var_ulonglong_all", "ncmpi_get_var_uchar",
    "ncmpi_get_var_uchar_all", "ncmpi_get_var_ushort", "ncmpi_get_var_ushort_all",
    "ncmpi_get_var_uint", "ncmpi_get_var_uint_all", "ncmpi_get_var_long",
    "ncmpi_get_var_long_all", "ncmpi_get_var_ulonglong", "ncmpi_get_var_ulonglong_all",
    "ncmpi_put_vara", "ncmpi_put_vara_all", "ncmpi_put_vara_text",
    "ncmpi_put_vara_text_all", "ncmpi_put_vara_schar", "ncmpi_put_vara_schar_all",
    "ncmpi_put_vara_short", "ncmpi_put_vara_short_all", "ncmpi_put_vara_int",
    "ncmpi_put_vara_int_all", "ncmpi_put_vara_float", "ncmpi_put_vara_float_all",
    "ncmpi_put_vara_double", "ncmpi_put_vara_double_all", "ncmpi_put_vara_longlong",
    "ncmpi_put_vara_longlong_all", "ncmpi_get_vara", "ncmpi_get_vara_all",
    "ncmpi_get_vara_text", "ncmpi_get_vara_text_all", "ncmpi_get_vara_schar",
    "ncmpi_get_vara_schar_all", "ncmpi_get_vara_short", "ncmpi_get_vara_short_all",
    "ncmpi_get_vara_int", "ncmpi_get_vara_int_all", "ncmpi_get_vara_float",
    "ncmpi_get_vara_float_all", "ncmpi_get_vara_double", "ncmpi_get_vara_double_all",
    "ncmpi_get_vara_longlong", "ncmpi_get_vara_longlong_all", "ncmpi_put_vara_uchar",
    "ncmpi_put_vara_uchar_all", "ncmpi_put_vara_ushort", "ncmpi_put_vara_ushort_all",
    "ncmpi_put_vara_uint", "ncmpi_put_vara_uint_all", "ncmpi_put_vara_long",
    "ncmpi_put_vara_long_all", "ncmpi_put_vara_ulonglong", "ncmpi_put_vara_ulonglong_all",
    "ncmpi_get_vara_uchar", "ncmpi_get_vara_uchar_all", "ncmpi_get_vara_ushort",
    "ncmpi_get_vara_ushort_all", "ncmpi_get_vara_uint", "ncmpi_get_vara_uint_all",
    "ncmpi_get_vara_long", "ncmpi_get_vara_long_all", "ncmpi_get_vara_ulonglong",
    "ncmpi_get_vara_ulonglong_all", "ncmpi_put_vars", "ncmpi_put_vars_all",
    "ncmpi_put_vars_text", "ncmpi_put_vars_text_all", "ncmpi_put_vars_schar",
    "ncmpi_put_vars_schar_all", "ncmpi_put_vars_short", "ncmpi_put_vars_short_all",
    "ncmpi_put_vars_int", "ncmpi_put_vars_int_all", "ncmpi_put_vars_float",
    "ncmpi_put_vars_float_all", "ncmpi_put_vars_double", "ncmpi_put_vars_double_all",
    "ncmpi_put_vars_longlong", "ncmpi_put_vars_longlong_all", "ncmpi_get_vars",
    "ncmpi_get_vars_all", "ncmpi_get_vars_schar", "ncmpi_get_vars_schar_all",
    "ncmpi_get_vars_text", "ncmpi_get_vars_text_all", "ncmpi_get_vars_short",
    "ncmpi_get_vars_short_all", "ncmpi_get_vars_int", "ncmpi_get_vars_int_all",
    "ncmpi_get_vars_float", "ncmpi_get_vars_float_all", "ncmpi_get_vars_double",
    "ncmpi_get_vars_double_all", "ncmpi_get_vars_longlong", "ncmpi_get_vars_longlong_all",
    "ncmpi_put_vars_uchar", "ncmpi_put_vars_uchar_all", "ncmpi_put_vars_ushort",
    "ncmpi_put_vars_ushort_all", "ncmpi_put_vars_uint", "ncmpi_put_vars_uint_all",
    "ncmpi_put_vars_long", "ncmpi_put_vars_long_all", "ncmpi_put_vars_ulonglong",
    "ncmpi_put_vars_ulonglong_all", "ncmpi_get_vars_uchar", "ncmpi_get_vars_uchar_all",
    "ncmpi_get_vars_ushort", "ncmpi_get_vars_ushort_all", "ncmpi_get_vars_uint",
    "ncmpi_get_vars_uint_all", "ncmpi_get_vars_long", "ncmpi_get_vars_long_all",
    "ncmpi_get_vars_ulonglong", "ncmpi_get_vars_ulonglong_all", "ncmpi_put_varm",
    "ncmpi_put_varm_all", "ncmpi_put_varm_text", "ncmpi_put_varm_text_all",
    "ncmpi_put_varm_schar", "ncmpi_put_varm_schar_all", "ncmpi_put_varm_short",
    "ncmpi_put_varm_short_all", "ncmpi_put_varm_int", "ncmpi_put_varm_int_all",
    "ncmpi_put_varm_float", "ncmpi_put_varm_float_all", "ncmpi_put_varm_double",
    "ncmpi_put_varm_double_all", "ncmpi_put_varm_longlong", "ncmpi_put_varm_longlong_all",
    "ncmpi_get_varm", "ncmpi_get_varm_all", "ncmpi_get_varm_schar",
    "ncmpi_get_varm_schar_all", "ncmpi_get_varm_text", "ncmpi_get_varm_text_all",
    "ncmpi_get_varm_short", "ncmpi_get_varm_short_all", "ncmpi_get_varm_int",
    "ncmpi_get_varm_int_all", "ncmpi_get_varm_float", "ncmpi_get_varm_float_all",
    "ncmpi_get_varm_double", "ncmpi_get_varm_double_all", "ncmpi_get_varm_longlong",
    "ncmpi_get_varm_longlong_all", "ncmpi_put_varm_uchar", "ncmpi_put_varm_uchar_all",
    "ncmpi_put_varm_ushort", "ncmpi_put_varm_ushort_all", "ncmpi_put_varm_uint",
    "ncmpi_put_varm_uint_all", "ncmpi_put_varm_long", "ncmpi_put_varm_long_all",
    "ncmpi_put_varm_ulonglong", "ncmpi_put_varm_ulonglong_all", "ncmpi_get_varm_uchar",
    "ncmpi_get_varm_uchar_all", "ncmpi_get_varm_ushort", "ncmpi_get_varm_ushort_all",
    "ncmpi_get_varm_uint", "ncmpi_get_varm_uint_all", "ncmpi_get_varm_long",
    "ncmpi_get_varm_long_all", "ncmpi_get_varm_ulonglong", "ncmpi_get_varm_ulonglong_all",
    "ncmpi_wait", "ncmpi_wait_all", "ncmpi_cancel",
    "ncmpi_buffer_attach", "ncmpi_buffer_detach", "ncmpi_inq_buffer_usage",
    "ncmpi_inq_buffer_size", "ncmpi_inq_nreqs", "ncmpi_iput_var1",
    "ncmpi_iput_var1_text", "ncmpi_iput_var1_schar", "ncmpi_iput_var1_short",
    "ncmpi_iput_var1_int", "ncmpi_iput_var1_float", "ncmpi_iput_var1_double",
    "ncmpi_iput_var1_longlong", "ncmpi_iget_var1", "ncmpi_iget_var1_schar",
    "ncmpi_iget_var1_text", "ncmpi_iget_var1_short", "ncmpi_iget_var1_int",
    "ncmpi_iget_var1_float", "ncmpi_iget_var1_double", "ncmpi_iget_var1_longlong",
    "ncmpi_bput_var1", "ncmpi_bput_var1_text", "ncmpi_bput_var1_schar",
    "ncmpi_bput_var1_short", "ncmpi_bput_var1_int", "ncmpi_bput_var1_float",
    "ncmpi_bput_var1_double", "ncmpi_bput_var1_longlong", "ncmpi_iput_var1_uchar",
    "ncmpi_iput_var1_ushort", "ncmpi_iput_var1_uint", "ncmpi_iput_var1_long",
    "ncmpi_iput_var1_ulonglong", "ncmpi_iget_var1_uchar", "ncmpi_iget_var1_ushort",
    "ncmpi_iget_var1_uint", "ncmpi_iget_var1_long", "ncmpi_iget_var1_ulonglong",
    "ncmpi_bput_var1_uchar", "ncmpi_bput_var1_ushort", "ncmpi_bput_var1_uint",
    "ncmpi_bput_var1_long", "ncmpi_bput_var1_ulonglong", "ncmpi_iput_var",
    "ncmpi_iput_var_schar", "ncmpi_iput_var_text", "ncmpi_iput_var_short",
    "ncmpi_iput_var_int", "ncmpi_iput_var_float", "ncmpi_iput_var_double",
    "ncmpi_iput_var_longlong", "ncmpi_iget_var", "ncmpi_iget_var_schar",
    "ncmpi_iget_var_text", "ncmpi_iget_var_short", "ncmpi_iget_var_int",
    "ncmpi_iget_var_float", "ncmpi_iget_var_double", "ncmpi_iget_var_longlong",
    "ncmpi_bput_var", "ncmpi_bput_var_schar", "ncmpi_bput_var_text",
    "ncmpi_bput_var_short", "ncmpi_bput_var_int", "ncmpi_bput_var_float",
    "ncmpi_bput_var_double", "ncmpi_bput_var_longlong", "ncmpi_iput_var_uchar",
    "ncmpi_iput_var_ushort", "ncmpi_iput_var_uint", "ncmpi_iput_var_long",
    "ncmpi_iput_var_ulonglong", "ncmpi_iget_var_uchar", "ncmpi_iget_var_ushort",
    "ncmpi_iget_var_uint", "ncmpi_iget_var_long", "ncmpi_iget_var_ulonglong",
    "ncmpi_bput_var_uchar", "ncmpi_bput_var_ushort", "ncmpi_bput_var_uint",
    "ncmpi_bput_var_long", "ncmpi_bput_var_ulonglong", "ncmpi_iput_vara",
    "ncmpi_iput_vara_schar", "ncmpi_iput_vara_text", "ncmpi_iput_vara_short",
    "ncmpi_iput_vara_int", "ncmpi_iput_vara_float", "ncmpi_iput_vara_double",
    "ncmpi_iput_vara_longlong", "ncmpi_iget_vara", "ncmpi_iget_vara_schar",
    "ncmpi_iget_vara_text", "ncmpi_iget_vara_short", "ncmpi_iget_vara_int",
    "ncmpi_iget_vara_float", "ncmpi_iget_vara_double", "ncmpi_iget_vara_longlong",
    "ncmpi_bput_vara", "ncmpi_bput_vara_schar", "ncmpi_bput_vara_text",
    "ncmpi_bput_vara_short", "ncmpi_bput_vara_int", "ncmpi_bput_vara_float",
    "ncmpi_bput_vara_double", "ncmpi_bput_vara_longlong", "ncmpi_iput_vara_uchar",
    "ncmpi_iput_vara_ushort", "ncmpi_iput_vara_uint", "ncmpi_iput_vara_long",
    "ncmpi_iput_vara_ulonglong", "ncmpi_iget_vara_uchar", "ncmpi_iget_vara_ushort",
    "ncmpi_iget_vara_uint", "ncmpi_iget_vara_long", "ncmpi_iget_vara_ulonglong",
    "ncmpi_bput_vara_uchar", "ncmpi_bput_vara_ushort", "ncmpi_bput_vara_uint",
    "ncmpi_bput_vara_long", "ncmpi_bput_vara_ulonglong", "ncmpi_iput_vars",
    "ncmpi_iput_vars_schar", "ncmpi_iput_vars_text", "ncmpi_iput_vars_short",
    "ncmpi_iput_vars_int", "ncmpi_iput_vars_float", "ncmpi_iput_vars_double",
    "ncmpi_iput_vars_longlong", "ncmpi_iget_vars", "ncmpi_iget_vars_schar",
    "ncmpi_iget_vars_text", "ncmpi_iget_vars_short", "ncmpi_iget_vars_int",
    "ncmpi_iget_vars_float", "ncmpi_iget_vars_double", "ncmpi_iget_vars_longlong",
    "ncmpi_bput_vars", "ncmpi_bput_vars_schar", "ncmpi_bput_vars_text",
    "ncmpi_bput_vars_short", "ncmpi_bput_vars_int", "ncmpi_bput_vars_float",
    "ncmpi_bput_vars_double", "ncmpi_bput_vars_longlong", "ncmpi_iput_vars_uchar",
    "ncmpi_iput_vars_ushort", "ncmpi_iput_vars_uint", "ncmpi_iput_vars_long",
    "ncmpi_iput_vars_ulonglong", "ncmpi_iget_vars_uchar", "ncmpi_iget_vars_ushort",
    "ncmpi_iget_vars_uint", "ncmpi_iget_vars_long", "ncmpi_iget_vars_ulonglong",
    "ncmpi_bput_vars_uchar", "ncmpi_bput_vars_ushort", "ncmpi_bput_vars_uint",
    "ncmpi_bput_vars_long", "ncmpi_bput_vars_ulonglong", "ncmpi_iput_varm",
    "ncmpi_iput_varm_schar", "ncmpi_iput_varm_text", "ncmpi_iput_varm_short",
    "ncmpi_iput_varm_int", "ncmpi_iput_varm_float", "ncmpi_iput_varm_double",
    "ncmpi_iput_varm_longlong", "ncmpi_iget_varm", "ncmpi_iget_varm_schar",
    "ncmpi_iget_varm_text", "ncmpi_iget_varm_short", "ncmpi_iget_varm_int",
    "ncmpi_iget_varm_float", "ncmpi_iget_varm_double", "ncmpi_iget_varm_longlong",
    "ncmpi_bput_varm", "ncmpi_bput_varm_schar", "ncmpi_bput_varm_text",
    "ncmpi_bput_varm_short", "ncmpi_bput_varm_int", "ncmpi_bput_varm_float",
    "ncmpi_bput_varm_double", "ncmpi_bput_varm_longlong", "ncmpi_iput_varm_uchar",
    "ncmpi_iput_varm_ushort", "ncmpi_iput_varm_uint", "ncmpi_iput_varm_long",
    "ncmpi_iput_varm_ulonglong", "ncmpi_iget_varm_uchar", "ncmpi_iget_varm_ushort",
    "ncmpi_iget_varm_uint", "ncmpi_iget_varm_long", "ncmpi_iget_varm_ulonglong",
    "ncmpi_bput_varm_uchar", "ncmpi_bput_varm_ushort", "ncmpi_bput_varm_uint",
    "ncmpi_bput_varm_long", "ncmpi_bput_varm_ulonglong", "ncmpi_put_varn",
    "ncmpi_put_varn_all", "ncmpi_get_varn", "ncmpi_get_varn_all",
    "ncmpi_put_varn_text", "ncmpi_put_varn_schar", "ncmpi_put_varn_short",
    "ncmpi_put_varn_int", "ncmpi_put_varn_float", "ncmpi_put_varn_double",
    "ncmpi_put_varn_longlong", "ncmpi_put_varn_text_all", "ncmpi_put_varn_schar_all",
    "ncmpi_put_varn_short_all", "ncmpi_put_varn_int_all", "ncmpi_put_varn_float_all",
    "ncmpi_put_varn_double_all", "ncmpi_put_varn_longlong_all", "ncmpi_get_varn_text",
    "ncmpi_get_varn_schar", "ncmpi_get_varn_short", "ncmpi_get_varn_int",
    "ncmpi_get_varn_float", "ncmpi_get_varn_double", "ncmpi_get_varn_longlong",
    "ncmpi_get_varn_text_all", "ncmpi_get_varn_schar_all", "ncmpi_get_varn_short_all",
    "ncmpi_get_varn_int_all", "ncmpi_get_varn_float_all", "ncmpi_get_varn_double_all",
    "ncmpi_get_varn_longlong_all", "ncmpi_put_varn_uchar", "ncmpi_put_varn_ushort",
    "ncmpi_put_varn_uint", "ncmpi_put_varn_long", "ncmpi_put_varn_ulonglong",
    "ncmpi_put_varn_uchar_all", "ncmpi_put_varn_ushort_all", "ncmpi_put_varn_uint_all",
    "ncmpi_put_varn_long_all", "ncmpi_put_varn_ulonglong_all", "ncmpi_get_varn_uchar",
    "ncmpi_get_varn_ushort", "ncmpi_get_varn_uint", "ncmpi_get_varn_long",
    "ncmpi_get_varn_ulonglong", "ncmpi_get_varn_uchar_all", "ncmpi_get_varn_ushort_all",
    "ncmpi_get_varn_uint_all", "ncmpi_get_varn_long_all", "ncmpi_get_varn_ulonglong_all",
    "ncmpi_iput_varn", "ncmpi_iget_varn", "ncmpi_iput_varn_text",
    "ncmpi_iput_varn_schar", "ncmpi_iput_varn_short", "ncmpi_iput_varn_int",
    "ncmpi_iput_varn_float", "ncmpi_iput_varn_double", "ncmpi_iput_varn_longlong",
    "ncmpi_iget_varn_text", "ncmpi_iget_varn_schar", "ncmpi_iget_varn_short",
    "ncmpi_iget_varn_int", "ncmpi_iget_varn_float", "ncmpi_iget_varn_double",
    "ncmpi_iget_varn_longlong", "ncmpi_iput_varn_uchar", "ncmpi_iput_varn_ushort",
    "ncmpi_iput_varn_uint", "ncmpi_iput_varn_long", "ncmpi_iput_varn_ulonglong",
    "ncmpi_iget_varn_uchar", "ncmpi_iget_varn_ushort", "ncmpi_iget_varn_uint",
    "ncmpi_iget_varn_long", "ncmpi_iget_varn_ulonglong", "ncmpi_bput_varn",
    "ncmpi_bput_varn_text", "ncmpi_bput_varn_schar", "ncmpi_bput_varn_short",
    "ncmpi_bput_varn_int", "ncmpi_bput_varn_float", "ncmpi_bput_varn_double",
    "ncmpi_bput_varn_longlong", "ncmpi_bput_varn_uchar", "ncmpi_bput_varn_ushort",
    "ncmpi_bput_varn_uint", "ncmpi_bput_varn_long", "ncmpi_bput_varn_ulonglong",
    "ncmpi_get_vard", "ncmpi_get_vard_all", "ncmpi_put_vard",
    "ncmpi_put_vard_all", "ncmpi_mput_var", "ncmpi_mput_var_all",
    "ncmpi_mput_var_text", "ncmpi_mput_var_schar", "ncmpi_mput_var_uchar",
    "ncmpi_mput_var_short", "ncmpi_mput_var_ushort", "ncmpi_mput_var_int",
    "ncmpi_mput_var_uint", "ncmpi_mput_var_long", "ncmpi_mput_var_float",
    "ncmpi_mput_var_double", "ncmpi_mput_var_longlong", "ncmpi_mput_var_ulonglong",
    "ncmpi_mput_var_text_all", "ncmpi_mput_var_schar_all", "ncmpi_mput_var_uchar_all",
    "ncmpi_mput_var_short_all", "ncmpi_mput_var_ushort_all", "ncmpi_mput_var_int_all",
    "ncmpi_mput_var_uint_all", "ncmpi_mput_var_long_all", "ncmpi_mput_var_float_all",
    "ncmpi_mput_var_double_all", "ncmpi_mput_var_longlong_all", "ncmpi_mput_var_ulonglong_all",
    "ncmpi_mput_var1", "ncmpi_mput_var1_all", "ncmpi_mput_var1_text",
    "ncmpi_mput_var1_schar", "ncmpi_mput_var1_uchar", "ncmpi_mput_var1_short",
    "ncmpi_mput_var1_ushort", "ncmpi_mput_var1_int", "ncmpi_mput_var1_uint",
    "ncmpi_mput_var1_long", "ncmpi_mput_var1_float", "ncmpi_mput_var1_double",
    "ncmpi_mput_var1_longlong", "ncmpi_mput_var1_ulonglong", "ncmpi_mput_var1_text_all",
    "ncmpi_mput_var1_schar_all", "ncmpi_mput_var1_uchar_all", "ncmpi_mput_var1_short_all",
    "ncmpi_mput_var1_ushort_all", "ncmpi_mput_var1_int_all", "ncmpi_mput_var1_uint_all",
    "ncmpi_mput_var1_long_all", "ncmpi_mput_var1_float_all", "ncmpi_mput_var1_double_all",
    "ncmpi_mput_var1_longlong_all", "ncmpi_mput_var1_ulonglong_all", "ncmpi_mput_vara",
    "ncmpi_mput_vara_all", "ncmpi_mput_vara_text", "ncmpi_mput_vara_schar",
    "ncmpi_mput_vara_uchar", "ncmpi_mput_vara_short", "ncmpi_mput_vara_ushort",
    "ncmpi_mput_vara_int", "ncmpi_mput_vara_uint", "ncmpi_mput_vara_long",
    "ncmpi_mput_vara_float", "ncmpi_mput_vara_double", "ncmpi_mput_vara_longlong",
    "ncmpi_mput_vara_ulonglong", "ncmpi_mput_vara_text_all", "ncmpi_mput_vara_schar_all",
    "ncmpi_mput_vara_uchar_all", "ncmpi_mput_vara_short_all", "ncmpi_mput_vara_ushort_all",
    "ncmpi_mput_vara_int_all", "ncmpi_mput_vara_uint_all", "ncmpi_mput_vara_long_all",
    "ncmpi_mput_vara_float_all", "ncmpi_mput_vara_double_all", "ncmpi_mput_vara_longlong_all",
    "ncmpi_mput_vara_ulonglong_all", "ncmpi_mput_vars", "ncmpi_mput_vars_all",
    "ncmpi_mput_vars_text", "ncmpi_mput_vars_schar", "ncmpi_mput_vars_uchar",
    "ncmpi_mput_vars_short", "ncmpi_mput_vars_ushort", "ncmpi_mput_vars_int",
    "ncmpi_mput_vars_uint", "ncmpi_mput_vars_long", "ncmpi_mput_vars_float",
    "ncmpi_mput_vars_double", "ncmpi_mput_vars_longlong", "ncmpi_mput_vars_ulonglong",
    "ncmpi_mput_vars_text_all", "ncmpi_mput_vars_schar_all", "ncmpi_mput_vars_uchar_all",
    "ncmpi_mput_vars_short_all", "ncmpi_mput_vars_ushort_all", "ncmpi_mput_vars_int_all",
    "ncmpi_mput_vars_uint_all", "ncmpi_mput_vars_long_all", "ncmpi_mput_vars_float_all",
    "ncmpi_mput_vars_double_all", "ncmpi_mput_vars_longlong_all", "ncmpi_mput_vars_ulonglong_all",
    "ncmpi_mput_varm", "ncmpi_mput_varm_all", "ncmpi_mput_varm_text",
    "ncmpi_mput_varm_schar", "ncmpi_mput_varm_uchar", "ncmpi_mput_varm_short",
    "ncmpi_mput_varm_ushort", "ncmpi_mput_varm_int", "ncmpi_mput_varm_uint",
    "ncmpi_mput_varm_long", "ncmpi_mput_varm_float", "ncmpi_mput_varm_double",
    "ncmpi_mput_varm_longlong", "ncmpi_mput_varm_ulonglong", "ncmpi_mput_varm_text_all",
    "ncmpi_mput_varm_schar_all", "ncmpi_mput_varm_uchar_all", "ncmpi_mput_varm_short_all",
    "ncmpi_mput_varm_ushort_all", "ncmpi_mput_varm_int_all", "ncmpi_mput_varm_uint_all",
    "ncmpi_mput_varm_long_all", "ncmpi_mput_varm_float_all", "ncmpi_mput_varm_double_all",
    "ncmpi_mput_varm_longlong_all", "ncmpi_mput_varm_ulonglong_all", "ncmpi_mget_var",
    "ncmpi_mget_var_all", "ncmpi_mget_var_text", "ncmpi_mget_var_schar",
    "ncmpi_mget_var_uchar", "ncmpi_mget_var_short", "ncmpi_mget_var_ushort",
    "ncmpi_mget_var_int", "ncmpi_mget_var_uint", "ncmpi_mget_var_long",
    "ncmpi_mget_var_float", "ncmpi_mget_var_double", "ncmpi_mget_var_longlong",
    "ncmpi_mget_var_ulonglong", "ncmpi_mget_var_text_all", "ncmpi_mget_var_schar_all",
    "ncmpi_mget_var_uchar_all", "ncmpi_mget_var_short_all", "ncmpi_mget_var_ushort_all",
    "ncmpi_mget_var_int_all", "ncmpi_mget_var_uint_all", "ncmpi_mget_var_long_all",
    "ncmpi_mget_var_float_all", "ncmpi_mget_var_double_all", "ncmpi_mget_var_longlong_all",
    "ncmpi_mget_var_ulonglong_all", "ncmpi_mget_var1", "ncmpi_mget_var1_all",
    "ncmpi_mget_var1_text", "ncmpi_mget_var1_schar", "ncmpi_mget_var1_uchar",
    "ncmpi_mget_var1_short", "ncmpi_mget_var1_ushort", "ncmpi_mget_var1_int",
    "ncmpi_mget_var1_uint", "ncmpi_mget_var1_long", "ncmpi_mget_var1_float",
    "ncmpi_mget_var1_double", "ncmpi_mget_var1_longlong", "ncmpi_mget_var1_ulonglong",
    "ncmpi_mget_var1_text_all", "ncmpi_mget_var1_schar_all", "ncmpi_mget_var1_uchar_all",
    "ncmpi_mget_var1_short_all", "ncmpi_mget_var1_ushort_all", "ncmpi_mget_var1_int_all",
    "ncmpi_mget_var1_uint_all", "ncmpi_mget_var1_long_all", "ncmpi_mget_var1_float_all",
    "ncmpi_mget_var1_double_all", "ncmpi_mget_var1_longlong_all", "ncmpi_mget_var1_ulonglong_all",
    "ncmpi_mget_vara", "ncmpi_mget_vara_all", "ncmpi_mget_vara_text",
    "ncmpi_mget_vara_schar", "ncmpi_mget_vara_uchar", "ncmpi_mget_vara_short",
    "ncmpi_mget_vara_ushort", "ncmpi_mget_vara_int", "ncmpi_mget_vara_uint",
    "ncmpi_mget_vara_long", "ncmpi_mget_vara_float", "ncmpi_mget_vara_double",
    "ncmpi_mget_vara_longlong", "ncmpi_mget_vara_ulonglong", "ncmpi_mget_vara_text_all",
    "ncmpi_mget_vara_schar_all", "ncmpi_mget_vara_uchar_all", "ncmpi_mget_vara_short_all",
    "ncmpi_mget_vara_ushort_all", "ncmpi_mget_vara_int_all", "ncmpi_mget_vara_uint_all",
    "ncmpi_mget_vara_long_all", "ncmpi_mget_vara_float_all", "ncmpi_mget_vara_double_all",
    "ncmpi_mget_vara_longlong_all", "ncmpi_mget_vara_ulonglong_all", "ncmpi_mget_vars",
    "ncmpi_mget_vars_all", "ncmpi_mget_vars_text", "ncmpi_mget_vars_schar",
    "ncmpi_mget_vars_uchar", "ncmpi_mget_vars_short", "ncmpi_mget_vars_ushort",
    "ncmpi_mget_vars_int", "ncmpi_mget_vars_uint", "ncmpi_mget_vars_long",
    "ncmpi_mget_vars_float", "ncmpi_mget_vars_double", "ncmpi_mget_vars_longlong",
    "ncmpi_mget_vars_ulonglong", "ncmpi_mget_vars_text_all", "ncmpi_mget_vars_schar_all",
    "ncmpi_mget_vars_uchar_all", "ncmpi_mget_vars_short_all", "ncmpi_mget_vars_ushort_all",
    "ncmpi_mget_vars_int_all", "ncmpi_mget_vars_uint_all", "ncmpi_mget_vars_long_all",
    "ncmpi_mget_vars_float_all", "ncmpi_mget_vars_double_all", "ncmpi_mget_vars_longlong_all",
    "ncmpi_mget_vars_ulonglong_all", "ncmpi_mget_varm", "ncmpi_mget_varm_all",
    "ncmpi_mget_varm_text", "ncmpi_mget_varm_schar", "ncmpi_mget_varm_uchar",
    "ncmpi_mget_varm_short", "ncmpi_mget_varm_ushort", "ncmpi_mget_varm_int",
    "ncmpi_mget_varm_uint", "ncmpi_mget_varm_long", "ncmpi_mget_varm_float",
    "ncmpi_mget_varm_double", "ncmpi_mget_varm_longlong", "ncmpi_mget_varm_ulonglong",
    "ncmpi_mget_varm_text_all", "ncmpi_mget_varm_schar_all", "ncmpi_mget_varm_uchar_all",
    "ncmpi_mget_varm_short_all", "ncmpi_mget_varm_ushort_all", "ncmpi_mget_varm_int_all",
    "ncmpi_mget_varm_uint_all", "ncmpi_mget_varm_long_all", "ncmpi_mget_varm_float_all",
    "ncmpi_mget_varm_double_all", "ncmpi_mget_varm_longlong_all", "ncmpi_mget_varm_ulonglong_all",

    // NetCDF 4.9.2
    "nc__create", "nc__enddef", "nc__open",
    "nc_abort", "nc_close", "nc_close_memio",
    "nc_create", "nc_create_mem", "nc_create_par",
    "nc_create_par_fortran", "nc_def_user_format", "nc_enddef",
    "nc_get_alignment", "nc_get_chunk_cache", "nc_inq",
    "nc_inq_format", "nc_inq_format_extended", "nc_inq_path",
    "nc_inq_type", "nc_inq_user_format", "nc_open",
    "nc_open_mem", "nc_open_memio", "nc_open_par",
    "nc_open_par_fortran", "nc_redef", "nc_set_alignment",
    "nc_set_chunk_cache", "nc_set_fill", "nc_sync",
    "nc_var_par_access", "nc_def_dim", "nc_inq_dim",
    "nc_inq_dimid", "nc_inq_dimlen", "nc_inq_dimname",
    "nc_inq_ndims", "nc_inq_unlimdim", "nc_rename_dim",
    "NC_atomictypelen", "NC_atomictypename", "nc_def_var_blosc",
    "nc_inq_var_filter", "nc_inq_var_filter_ids", "nc_inq_var_filter_info",
    "nctypelen", "nc_def_var", "nc_def_var_fill",
    "nc_def_var_deflate", "nc_def_var_quantize", "nc_def_var_fletcher32",
    "nc_def_var_chunking", "nc_def_var_endian", "nc_def_var_szip",
    "nc_rename_var", "nc_free_string", "nc_set_var_chunk_cache",
    "nc_get_var_chunk_cache", "nc_get_vara", "nc_get_vara_text",
    "nc_get_vara_schar", "nc_get_vara_uchar", "nc_get_vara_short",
    "nc_get_vara_int", "nc_get_vara_long", "nc_get_vara_float",
    "nc_get_vara_double", "nc_get_vara_ubyte", "nc_get_vara_ushort",
    "nc_get_vara_uint", "nc_get_vara_longlong", "nc_get_vara_ulonglong",
    "nc_get_vara_string", "nc_get_var1", "nc_get_var1_text",
    "nc_get_var1_schar", "nc_get_var1_uchar", "nc_get_var1_short",
    "nc_get_var1_int", "nc_get_var1_long", "nc_get_var1_float",
    "nc_get_var1_double", "nc_get_var1_ubyte", "nc_get_var1_ushort",
    "nc_get_var1_uint", "nc_get_var1_longlong", "nc_get_var1_ulonglong",
    "nc_get_var1_string", "nc_get_var", "nc_get_var_text",
    "nc_get_var_schar", "nc_get_var_uchar", "nc_get_var_short",
    "nc_get_var_int", "nc_get_var_long", "nc_get_var_float",
    "nc_get_var_double", "nc_get_var_ubyte", "nc_get_var_ushort",
    "nc_get_var_uint", "nc_get_var_longlong", "nc_get_var_ulonglong",
    "nc_get_var_string", "nc_get_vars", "nc_get_vars_text",
    "nc_get_vars_schar", "nc_get_vars_uchar", "nc_get_vars_short",
    "nc_get_vars_int", "nc_get_vars_long", "nc_get_vars_float",
    "nc_get_vars_double", "nc_get_vars_ubyte", "nc_get_vars_ushort",
    "nc_get_vars_uint", "nc_get_vars_longlong", "nc_get_vars_ulonglong",
    "nc_get_vars_string", "nc_get_varm", "nc_get_varm_schar",
    "nc_get_varm_uchar", "nc_get_varm_short", "nc_get_varm_int",
    "nc_get_varm_long", "nc_get_varm_float", "nc_get_varm_double",
    "nc_get_varm_ubyte", "nc_get_varm_ushort", "nc_get_varm_uint",
    "nc_get_varm_longlong", "nc_get_varm_ulonglong", "nc_get_varm_text",
    "nc_get_varm_string", "nc_inq_varid", "nc_inq_var",
    "nc_inq_varname", "nc_inq_vartype", "nc_inq_varndims",
    "nc_inq_vardimid", "nc_inq_varnatts", "nc_inq_var_deflate",
    "nc_inq_var_fletcher32", "nc_inq_var_chunking", "nc_inq_var_fill",
    "nc_inq_var_quantize", "nc_inq_var_endian", "nc_inq_var_szip",
    "nc_inq_unlimdims", "nc_put_vara", "nc_put_vara_text",
    "nc_put_vara_schar", "nc_put_vara_uchar", "nc_put_vara_short",
    "nc_put_vara_int", "nc_put_vara_long", "nc_put_vara_float",
    "nc_put_vara_double", "nc_put_vara_ubyte", "nc_put_vara_ushort",
    "nc_put_vara_uint", "nc_put_vara_longlong", "nc_put_vara_ulonglong",
    "nc_put_vara_string", "nc_put_var1", "nc_put_var1_text",
    "nc_put_var1_schar", "nc_put_var1_uchar", "nc_put_var1_short",
    "nc_put_var1_int", "nc_put_var1_long", "nc_put_var1_float",
    "nc_put_var1_double", "nc_put_var1_ubyte", "nc_put_var1_ushort",
    "nc_put_var1_uint", "nc_put_var1_longlong", "nc_put_var1_ulonglong",
    "nc_put_var1_string", "nc_put_var", "nc_put_var_text",
    "nc_put_var_schar", "nc_put_var_uchar", "nc_put_var_short",
    "nc_put_var_int", "nc_put_var_long", "nc_put_var_float",
    "nc_put_var_double", "nc_put_var_ubyte", "nc_put_var_ushort",
    "nc_put_var_uint", "nc_put_var_longlong", "nc_put_var_ulonglong",
    "nc_put_var_string", "nc_put_vars", "nc_put_vars_text",
    "nc_put_vars_schar", "nc_put_vars_uchar", "nc_put_vars_short",
    "nc_put_vars_int", "nc_put_vars_long", "nc_put_vars_float",
    "nc_put_vars_double", "nc_put_vars_ubyte", "nc_put_vars_ushort",
    "nc_put_vars_uint", "nc_put_vars_longlong", "nc_put_vars_ulonglong",
    "nc_put_vars_string", "nc_put_varm", "nc_put_varm_text",
    "nc_put_varm_schar", "nc_put_varm_uchar", "nc_put_varm_short",
    "nc_put_varm_int", "nc_put_varm_long", "nc_put_varm_float",
    "nc_put_varm_double", "nc_put_varm_ubyte", "nc_put_varm_ushort",
    "nc_put_varm_uint", "nc_put_varm_longlong", "nc_put_varm_ulonglong",
    "nc_put_varm_string", "nc_inq_att", "nc_inq_attid",
    "nc_inq_attname", "nc_inq_natts", "nc_inq_atttype",
    "nc_inq_attlen", "nc_def_grp", "nc_get_att",
    "nc_get_att_text", "nc_get_att_schar", "nc_get_att_uchar",
    "nc_get_att_short", "nc_get_att_int", "nc_get_att_long",
    "nc_get_att_float", "nc_get_att_double", "nc_get_att_ubyte",
    "nc_get_att_ushort", "nc_get_att_uint", "nc_get_att_longlong",
    "nc_get_att_ulonglong", "nc_get_att_string", "nc_put_att_string",
    "nc_put_att_text", "nc_put_att", "nc_put_att_schar",
    "nc_put_att_uchar", "nc_put_att_short", "nc_put_att_int",
    "nc_put_att_long", "nc_put_att_float", "nc_put_att_double",
    "nc_put_att_ubyte", "nc_put_att_ushort", "nc_put_att_uint",
    "nc_put_att_longlong", "nc_put_att_ulonglong", "nc_rename_att",
    "nc_del_att", "nc_inq_dimids", "nc_inq_grp_full_ncid",
    "nc_inq_grp_ncid", "nc_inq_grp_parent", "nc_inq_grpname",
    "nc_inq_grpname_full", "nc_inq_grpname_len", "nc_inq_grps",
    "nc_inq_ncid", "nc_inq_typeids", "nc_inq_varids",
    "nc_rename_grp", "nc_show_metadata", "nc_inq_type_equal",
    "nc_inq_typeid", "nc_inq_user_type", "nc_def_compound",
    "nc_insert_compound", "nc_insert_array_compound", "nc_inq_compound",
    "nc_inq_compound_name", "nc_inq_compound_size", "nc_inq_compound_nfields",
    "nc_inq_compound_field", "nc_inq_compound_fieldname", "nc_inq_compound_fieldoffset",
    "nc_inq_compound_fieldtype", "nc_inq_compound_fieldndims", "nc_inq_compound_fielddim_sizes",
    "nc_inq_compound_fieldindex", "nc_def_enum", "nc_insert_enum",
    "nc_inq_enum", "nc_inq_enum_member", "nc_inq_enum_ident",
    "nc_free_vlen", "nc_free_vlens", "nc_def_vlen",
    "nc_inq_vlen", "nc_def_opaque", "nc_inq_opaque",
};

#endif /* __RECORDER_LOGGER_H */
