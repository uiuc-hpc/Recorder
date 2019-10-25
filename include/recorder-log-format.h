/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2012 by The HDF Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 * 4. All publications or advertising materials mentioning features or use of
 *    this software are asked, but not required, to acknowledge that it was
 *    developed by The HDF Group and by the National Center for Supercomputing
 *    Applications at the University of Illinois at Urbana-Champaign and
 *    credit the contributors.
 *
 * 5. Neither the name of The HDF Group, the name of the University, nor the
 *    name of any Contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission from
 *    The HDF Group, the University, or the Contributor, respectively.
 *
 * DISCLAIMER:
 * THIS SOFTWARE IS PROVIDED BY THE HDF GROUP AND THE CONTRIBUTORS
 * "AS IS" WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED. In no
 * event shall The HDF Group or the Contributors be liable for any damages
 * suffered by the users arising out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * Portions of Recorder were developed with support from the Lawrence Berkeley
 * National Laboratory (LBNL) and the United States Department of Energy under
 * Prime Contract No. DE-AC02-05CH11231.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __RECORDER_LOG_FORMAT_H
#define __RECORDER_LOG_FORMAT_H

#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if !defined PRId64 || defined(PRI_MACROS_BROKEN)
#ifndef __WORDSIZE
#error failed to detect PRId64 or word size
#endif
# undef PRId64
#if __WORDSIZE == 64
# define PRId64 "ld"
#else
# define PRId64 "lld"
#endif
#endif
#if !defined PRIu64 || defined(PRI_MACROS_BROKEN)
#ifndef __WORDSIZE
#error failed to detect PRId64 or word size
#endif
# undef PRIu64
#if __WORDSIZE == 64
# define PRIu64 "lu"
#else
# define PRIu64 "llu"
#endif
#endif


/**
 * attr1 = offset for read/write/seek
 * attr1 = mode for open
 *
 * attr2 = counts for read/write
 * attr2 = whence for seek
 */
typedef struct IoOperation {
    unsigned char func_id;
    unsigned char filename_id;
    double start_time;
    double end_time;
    size_t attr1;
    size_t attr2;
} IoOperation_t;


typedef struct Record_t {
    int tstart, tdur;
    char *func_id;
    int arg_count;
    char **args;
} Record;

static const char* my_func_list[] = {
    // POSIX I/O
    "creat", /* 0 */        "creat64",  /* 1 */
    "open",  /* 2 */        "open64",   /* 3 */     "close",    /* ... */
    "write",                "read",
    "lseek",                "lseek64",
    "pread",                "pread64",
    "pwrite",               "pwrite64",
    "readv",                "writev",
    "mmap",                 "mmap64",
    "fopen",                "fopen64",              "fclose"
    "fwrite",               "fread",
    "fseek",                "fsync",                "fdatasync",

    // MPI I/O
    "MPI_File_close",                       /* 25 */
    "MPI_File_set_size",
    "MPI_File_iread_at",
    "MPI_File_iread",
    "MPI_File_iread_shared",
    "MPI_File_iwrite_at",
    "MPI_File_iwrite",
    "MPI_File_iwrite_shared",
    "MPI_File_open",
    "MPI_File_read_all_begin",
    "MPI_File_read_all",
    "MPI_File_read_at_all",
    "MPI_File_read_at_all_begin",
    "MPI_File_read_at",
    "MPI_File_read",
    "MPI_File_read_ordered_begin",
    "MPI_File_read_ordered",
    "MPI_File_read_shared",
    "MPI_File_set_view",
    "MPI_File_sync",
    "MPI_File_write_all_begin",
    "MPI_File_write_all",
    "MPI_File_write_at_all_begin",
    "MPI_File_write_at_all",
    "MPI_File_write_at",
    "MPI_File_write",
    "MPI_File_write_ordered_begin",
    "MPI_File_write_ordered",
    "MPI_File_write_shared"
    "MPI_Finalize",
    "MPI_Init",
    "MPI_Init_thread",
    "MPI_Wtime",
    "MPI_Comm_rank",
    "MPI_Comm_size",
    "MPI_Barrier",
    "MPI_Bcast",
    "MPI_Gather",
    "MPI_Gatherv",
    "MPI_Scatter",
    "MPI_Scatterv",
    "MPI_Allgather",
    "MPI_Allgatherv",
    "MPI_Alltoall",
    "MPI_Reduce",
    "MPI_Allreduce",
    "MPI_Reduce_scatter",
    "MPI_Scan",
    "MPI_Type_commit",
    "MPI_Type_contiguous",
    "MPI_Type_extent",
    "MPI_Type_free",
    "MPI_Type_hindexed",
    "MPI_Op_create",
    "MPI_Op_free",
    "MPI_Type_get_envelope",
    "MPI_Type_size",                        /* 81 */

    "H5Fcreate", // File interface
    "H5Fopen",                      "H5Fclose",
    "H5Gclose", // Group interface
    "H5Gcreate1",                   "H5Gcreate2",
    "H5Gget_objinfo",               "H5Giterate",
    "H5Gopen1",                     "H5Gopen2",
    "H5Dclose", // Dataset interface
    "H5Dcreate1",                   "H5Dcreate2",
    "H5Dget_create_plist",          "H5Dget_space",
    "H5Dget_type",                  "H5Dopen1",
    "H5Dopen2",                     "H5Dread",
    "H5Dwrite",
    "H5Sclose", // Dataspace interface
    "H5Screate",                    "H5Screate_simple",
    "H5Sget_select_npoints",        "H5Sget_simple_extent_dims",
    "H5Sget_simple_extent_npoints", "H5Sselect_elements",
    "H5Sselect_hyperslab",          "H5Sselect_none",
    "H5Tclose", // Datatype interface
    "H5Tcopy",                      "H5Tget_class",
    "H5Tget_size",                  "H5Tset_size",
    "H5Tcreate",                    "H5Tinsert",
    "H5Aclose", // Attribute interface
    "H5Acreate1",                   "H5Acreate2",
    "H5Aget_name",                  "H5Aget_num_attrs",
    "H5Aget_space",                 "H5Aget_type",
    "H5Aopen",                      "H5Aopen_idx",
    "H5Aopen_name",                 "H5Aread",
    "H5Awrite",
    "H5Pclose", // Property List interface
    "H5Pcreate",                    "H5Pget_chunk",
    "H5Pget_mdc_config",            "H5Pset_alignment",
    "H5Pset_chunk",                 "H5Pset_dxpl_mpio",
    "H5Pset_fapl_core",             "H5Pset_fapl_mpio",
    "H5Pset_fapl_mpiposix",         "H5Pset_istore_k",
    "H5Pset_mdc_config",            "H5Pset_meta_block_size",
    "H5Lexists", // Link interface
    "H5Lget_val",                   "H5Literate",
    "H5Oclose", // Object interface
    "H5Oget_info",                  "H5Oget_info_by_name",
    "H5Oopen"
};

static inline const char* get_function_name_by_id(int id) {
    if (id < 0 || id > 90) return "WRONG_FUNCTION_ID";
    return my_func_list[id];
}
static inline int get_function_id_by_name(const char* name) {
    size_t len = sizeof(my_func_list) / sizeof(char *);
    int i;
    for(i = 0; i < len; i++) {
        if (strcmp(my_func_list[i], name) == 0)
            return i;
    }
    return -1;
}


#endif /* __RECORDER_LOG_FORMAT_H */
