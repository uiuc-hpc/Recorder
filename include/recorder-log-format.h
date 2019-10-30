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


typedef struct Record_t {
    double tstart, tend;
    unsigned char func_id;      // we have about 200 functions in total
    int arg_count;
    char **args;
} Record;


static const char* func_list[] = {
    // POSIX I/O - 66 functions
    "creat",        "creat64",      "open",         "open64",   "close",
    "write",        "read",         "lseek",        "lseek64",  "pread",
    "pread64",      "pwrite",       "pwrite64",     "readv",    "writev",
    "mmap",         "mmap64",       "fopen",        "fopen64",  "fclose"
    "fwrite",       "fread",        "ftell",        "fseek",    "fsync",
    "fdatasync",    "__xstat",      "__xstat64",    "__lxstat", "__lxstat64",
    "__fxstat",     "__fxstat64",   "getcwd",       "mkdir",    "rmdir",
    "chdir",        "link",         "linkat",       "unlink",   "symlink",
    "symlinkat",    "readlink",     "readlinkat",   "rename",   "chmod",
    "chown",        "lchown",       "utime",        "opendir",  "readdir",
    "closedir",     "rewinddir",    "mknod",        "mknodat",  "fcntl",
    "dup",          "dup2",         "pipe",         "mkfifo",   "umask",
    "fdopen",       "fileno",       "access",       "faccessat","tmpfile",
    "remove",

    // MPI I/O  - 60 functions
    "PMPI_File_close",              "PMPI_File_set_size",       "PMPI_File_iread_at",
    "PMPI_File_iread",              "PMPI_File_iread_shared",   "PMPI_File_iwrite_at",
    "PMPI_File_iwrite",             "PMPI_File_iwrite_shared",  "PMPI_File_open",
    "PMPI_File_read_all_begin",     "PMPI_File_read_all",       "PMPI_File_read_at_all",
    "PMPI_File_read_at_all_begin",  "PMPI_File_read_at",        "PMPI_File_read",
    "PMPI_File_read_ordered_begin", "PMPI_File_read_ordered",   "PMPI_File_read_shared",
    "PMPI_File_set_view",           "PMPI_File_sync",           "PMPI_File_write_all_begin",
    "PMPI_File_write_all",          "PMPI_File_write_at_all_begin", "PMPI_File_write_at_all",
    "PMPI_File_write_at",           "PMPI_File_write",          "PMPI_File_write_ordered_begin",
    "PMPI_File_write_ordered",      "PMPI_File_write_shared",   "PMPI_Finalize",
    "PMPI_Finalized",               "PMPI_Init",                "PMPI_Init_thread",
    "PMPI_Wtime",                   "PMPI_Comm_rank",           "PMPI_Comm_size",
    "PMPI_Get_processor_name",      "PMPI_Get_processor_name",  "PMPI_Comm_set_errhandler",
    "PMPI_Barrier",                 "PMPI_Bcast",               "PMPI_Gather",
    "PMPI_Gatherv",                 "PMPI_Scatter",             "PMPI_Scatterv",
    "PMPI_Allgather",               "PMPI_Allgatherv",          "PMPI_Alltoall",
    "PMPI_Reduce",                  "PMPI_Allreduce",           "PMPI_Reduce_scatter",
    "PMPI_Scan",                    "PMPI_Type_commit",         "PMPI_Type_contiguous",
    "PMPI_Type_extent",             "PMPI_Type_free",           "PMPI_Type_hindexed",
    "PMPI_Op_create",               "PMPI_Op_free",             "PMPI_Type_get_envelope",
    "PMPI_Type_size",


    // HDF5 I/O
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

/*
static inline const char* get_function_name_by_id(int id) {
    if (id < 0 || id > 255) return "WRONG_FUNCTION_ID";
    return func_list[id];
}
static unsigned char get_function_id_by_name(const char* name) {
    size_t len = sizeof(func_list) / sizeof(char *);
    unsigned char i;
    for(i = 0; i < len; i++) {
        if (strcmp(func_list[i], name) == 0)
            return i;
    }
    return 255;
}
*/


#endif /* __RECORDER_LOG_FORMAT_H */
