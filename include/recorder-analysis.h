#ifndef __RECORDER_ANALYSIS_H
#define __RECORDER_ANALYSIS_H

#include "recorder.h"
#include "uthash.h"

typedef struct RuleConfidence_t {
    int rule;
    int count;
    int probability;
    UT_hash_handle hh;
} RuleConfidence;

int recorder_analysis(RecorderLogger* logger, Record* record, CallSignature* entry, Knowledge *knowledge);

static const char* collective_func_list[] = {
    // MPI 16 functions
    "MPI_Barrier",                  "MPI_Bcast",                        "MPI_Gather",                   "MPI_Scan",
    "MPI_Gatherv",                  "MPI_Scatter",                      "MPI_Scatterv",                 "MPI_Reduce",
    "MPI_Allreduce",                "MPI_Reduce_scatter",               "MPI_File_write_all",           "MPI_File_write_at_all_begin",
    "MPI_File_write_at_all",        "MPI_File_read_all_begin",          "MPI_File_read_all",            "MPI_File_read_at_all",


    // HDF5 I/O - 54 functions
    "H5Acreate",                "H5Acreate1",           "H5Acreate2",           "H5Acreate_by_name",
    "H5Adelete",                "H5Adelete_by_idx",     "H5Adelete_by_name",    "H5Arename", 
    "H5Arename_by_name",        "H5Awrite",             "H5Dcreate",            "H5Dcreate1",
    "H5Dcreate2",               "H5Dcreate_anon",       "H5Dextend",            "H5Dset_extent",
    "H5Fclose",                 "H5Fcreate",            "H5Fflush",             "H5Fmount",
    "H5Fopen",                  "H5Freopen",            "H5Funmount",           "H5Gcreate",
    "H5Gcreate1",               "H5Gcreate2",           "H5Gcreate_anon",       "H5Glink",
    "H5Glink2",                 "H5Gmove",              "H5Gmove2",             "H5Gset_comment",
    "H5Gunlink",                "H5Idec_ref",           "H5LIinc_ref",          "H5Lcopy",
    "H5Lcreate_external",       "H5Lcreate_hard",       "H5Lcreate_soft",       "H5Lcreate_ud",
    "H5Ldelete",                "H5Ldelete_by_idx",     "H5Lmove",              "H5Ocopy",
    "H5Odecr_refcount",         "H5Oincr_refcount",     "H5Olink",              "H5Oset_comment",
    "H5Oset_comment_by_name",   "H5Rcreate",            "H5Tcommit",            "H5Tcommit1",
    "H5Tcommit2",               "H5Tcommit_anon"
};
#endif 
