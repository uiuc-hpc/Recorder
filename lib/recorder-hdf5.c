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

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE         //  Need to be on top to use RTLD_NEXT

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include "mpi.h"
#include "recorder.h"
#include "hdf5.h"


#define SMALL_BUF_SIZE 128
#define LARGE_BUF_SIZE 1024


#define TRACE_LEN 256

#ifdef RECORDER_PRELOAD
    #ifndef DISABLE_HDF5_TRACE
        #define RECORDER_IMP_CHEN(func, ret, args, attr1, attr2, log_text)      \
            MAP_OR_FAIL(func)                                                   \
            depth++;                                                            \
            double tm1 = recorder_wtime();                                      \
            ret res = RECORDER_MPI_CALL(func) args ;                            \
            double tm2 = recorder_wtime();                                      \
            write_data_operation(#func, "", tm1, tm2, attr1, attr2, log_text);  \
            depth--;                                                            \
            return res;
    #else
        #define RECORDER_IMP_CHEN(func, ret, args, attr1, attr2, log_text)  \
            MAP_OR_FAIL(func)                                               \
            return RECORDER_MPI_CALL(func) args ;
    #endif
#endif

void get_datatype_name(int class_id, int type, char *string) {
  char *tmp = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
  switch (class_id) {
  case H5T_INTEGER:
    if (H5Tequal(type, H5T_STD_I8BE) == 1) {
      sprintf(tmp, "H5T_STD_I8BE");
    } else if (H5Tequal(type, H5T_STD_I8LE) == 1) {
      sprintf(tmp, "H5T_STD_I8LE");
    } else if (H5Tequal(type, H5T_STD_I16BE) == 1) {
      sprintf(tmp, "H5T_STD_I16BE");
    } else if (H5Tequal(type, H5T_STD_I16LE) == 1) {
      sprintf(tmp, "H5T_STD_I16LE");
    } else if (H5Tequal(type, H5T_STD_I32BE) == 1) {
      sprintf(tmp, "H5T_STD_I32BE");
    } else if (H5Tequal(type, H5T_STD_I32LE) == 1) {
      sprintf(tmp, "H5T_STD_I32LE");
    } else if (H5Tequal(type, H5T_STD_I64BE) == 1) {
      sprintf(tmp, "H5T_STD_I64BE");
    } else if (H5Tequal(type, H5T_STD_I64LE) == 1) {
      sprintf(tmp, "H5T_STD_I64LE");
    } else if (H5Tequal(type, H5T_STD_U8BE) == 1) {
      sprintf(tmp, "H5T_STD_U8BE");
    } else if (H5Tequal(type, H5T_STD_U8LE) == 1) {
      sprintf(tmp, "H5T_STD_U8LE");
    } else if (H5Tequal(type, H5T_STD_U16BE) == 1) {
      sprintf(tmp, "H5T_STD_U16BE");
    } else if (H5Tequal(type, H5T_STD_U16LE) == 1) {
      sprintf(tmp, "H5T_STD_U16LE");
    } else if (H5Tequal(type, H5T_STD_U32BE) == 1) {
      sprintf(tmp, "H5T_STD_U32BE");
    } else if (H5Tequal(type, H5T_STD_U32LE) == 1) {
      sprintf(tmp, "H5T_STD_U32LE");
    } else if (H5Tequal(type, H5T_STD_U64BE) == 1) {
      sprintf(tmp, "H5T_STD_U64BE");
    } else if (H5Tequal(type, H5T_STD_U64LE) == 1) {
      sprintf(tmp, "H5T_STD_U64LE");
    } else if (H5Tequal(type, H5T_NATIVE_SCHAR) == 1) {
      sprintf(tmp, "H5T_NATIVE_SCHAR");
    } else if (H5Tequal(type, H5T_NATIVE_UCHAR) == 1) {
      sprintf(tmp, "H5T_NATIVE_UCHAR");
    } else if (H5Tequal(type, H5T_NATIVE_SHORT) == 1) {
      sprintf(tmp, "H5T_NATIVE_SHORT");
    } else if (H5Tequal(type, H5T_NATIVE_USHORT) == 1) {
      sprintf(tmp, "H5T_NATIVE_USHORT");
    } else if (H5Tequal(type, H5T_NATIVE_INT) == 1) {
      sprintf(tmp, "H5T_NATIVE_INT");
    } else if (H5Tequal(type, H5T_NATIVE_UINT) == 1) {
      sprintf(tmp, "H5T_NATIVE_UINT");
    } else if (H5Tequal(type, H5T_NATIVE_LONG) == 1) {
      sprintf(tmp, "H5T_NATIVE_LONG");
    } else if (H5Tequal(type, H5T_NATIVE_ULONG) == 1) {
      sprintf(tmp, "H5T_NATIVE_ULONG");
    } else if (H5Tequal(type, H5T_NATIVE_LLONG) == 1) {
      sprintf(tmp, "H5T_NATIVE_LLONG");
    } else if (H5Tequal(type, H5T_NATIVE_ULLONG) == 1) {
      sprintf(tmp, "H5T_NATIVE_ULLONG");
    } else {
      sprintf(tmp, "%d", type);
    }
    break;

  case H5T_FLOAT:
    if (H5Tequal(type, H5T_IEEE_F32BE) == 1) {
      sprintf(tmp, "H5T_IEEE_F32BE");
    } else if (H5Tequal(type, H5T_IEEE_F32LE) == 1) {
      sprintf(tmp, "H5T_IEEE_F32LE");
    } else if (H5Tequal(type, H5T_IEEE_F64BE) == 1) {
      sprintf(tmp, "H5T_IEEE_F64BE");
    } else if (H5Tequal(type, H5T_IEEE_F64LE) == 1) {
      sprintf(tmp, "H5T_IEEE_F64LE");
    } else if (H5Tequal(type, H5T_VAX_F32) == 1) {
      sprintf(tmp, "H5T_VAX_F32");
    } else if (H5Tequal(type, H5T_VAX_F64) == 1) {
      sprintf(tmp, "H5T_VAX_F64");
    } else if (H5Tequal(type, H5T_NATIVE_FLOAT) == 1) {
      sprintf(tmp, "H5T_NATIVE_FLOAT");
    } else if (H5Tequal(type, H5T_NATIVE_DOUBLE) == 1) {
      sprintf(tmp, "H5T_NATIVE_DOUBLE");
#if H5_SIZEOF_LONG_DOUBLE != 0
    } else if (H5Tequal(type, H5T_NATIVE_LDOUBLE) == 1) {
      sprintf(tmp, "H5T_NATIVE_LDOUBLE");
#endif
    } else {
      sprintf(tmp, "%d", type);
    }
    break;

  case H5T_TIME:
    sprintf(tmp, "H5T_TIME");
    break;
  /*
      case 3:
              sprintf(tmp, "H5T_STRING");
              break;
      case 4:
              sprintf(tmp, "H5T_BITFIELD");
              break;
      case 5:
              sprintf(tmp, "H5T_OPAQUE");
              break;
      case 6:
              sprintf(tmp, "H5T_COMPOUND");
              break;
      case 7:
              sprintf(tmp, "H5T_REFERENCE");
              break;
      case 8:
              sprintf(tmp, "H5T_ENUM");
              break;
      case 9:
              sprintf(tmp, "H5T_VLEN");
              break;
      case 10:
              sprintf(tmp, "H5T_ARRAY");
              break;
   */
  default:
    sprintf(tmp, "%d", type);
  }

  strcpy(string, tmp);
  free(tmp);
}

void get_prop_list_cls_name(int class_id, char *string) {
  char *tmp = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);

  if (class_id == H5P_OBJECT_CREATE)
    sprintf(tmp, "H5P_OBJECT_CREATE");
  else if (class_id == H5P_FILE_CREATE)
    sprintf(tmp, "H5P_FILE_CREATE");
  else if (class_id == H5P_FILE_ACCESS)
    sprintf(tmp, "H5P_FILE_ACCESS");
  else if (class_id == H5P_DATASET_CREATE)
    sprintf(tmp, "H5P_DATASET_CREATE");
  else if (class_id == H5P_DATASET_ACCESS)
    sprintf(tmp, "H5P_DATASET_ACCESS");
  else if (class_id == H5P_DATASET_XFER)
    sprintf(tmp, "H5P_DATASET_XFER");
  else if (class_id == H5P_FILE_MOUNT)
    sprintf(tmp, "H5P_FILE_MOUNT");
  else if (class_id == H5P_GROUP_CREATE)
    sprintf(tmp, "H5P_GROUP_CREATE");
  else if (class_id == H5P_GROUP_ACCESS)
    sprintf(tmp, "H5P_GROUP_ACCESS");
  else if (class_id == H5P_DATATYPE_CREATE)
    sprintf(tmp, "H5P_DATATYPE_CREATE");
  else if (class_id == H5P_DATATYPE_ACCESS)
    sprintf(tmp, "H5P_DATATYPE_ACCESS");
  else if (class_id == H5P_STRING_CREATE)
    sprintf(tmp, "H5P_STRING_CREATE");
  else if (class_id == H5P_ATTRIBUTE_CREATE)
    sprintf(tmp, "H5P_ATTRIBUTE_CREATE");
  else if (class_id == H5P_OBJECT_COPY)
    sprintf(tmp, "H5P_OBJECT_COPY");
  else if (class_id == H5P_LINK_CREATE)
    sprintf(tmp, "H5P_LINK_CREATE");
  else if (class_id == H5P_LINK_ACCESS)
    sprintf(tmp, "H5P_LINK_ACCESS");
  else
    sprintf(tmp, "%d", class_id);

  strcpy(string, tmp);
  free(tmp);
}

void get_op_name(H5S_seloper_t op, char *string) {
  char *tmp = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
  if (op == H5S_SELECT_SET)
    sprintf(tmp, "H5S_SELECT_SET");
  else if (op == H5S_SELECT_OR)
    sprintf(tmp, "H5S_SELECT_OR");
  else if (op == H5S_SELECT_AND)
    sprintf(tmp, "H5S_SELECT_AND");
  else if (op == H5S_SELECT_XOR)
    sprintf(tmp, "H5S_SELECT_XOR");
  else if (op == H5S_SELECT_NOTB)
    sprintf(tmp, "H5S_SELECT_NOTB");
  else if (op == H5S_SELECT_NOTA)
    sprintf(tmp, "H5S_SELECT_NOTA");
  else
    sprintf(tmp, "%d", op);

  strcpy(string, tmp);
  free(tmp);
}

// TODO: not used?
//static struct recorder_file_runtime *recorder_file_by_hid(int hid);


hid_t RECORDER_DECL(H5Fcreate)(const char *filename, unsigned flags, hid_t create_plist, hid_t access_plist) {
  char log_text[TRACE_LEN];
  sprintf(log_text, "H5Fcreate (%s, %d, %d, %d)", filename, flags, create_plist, access_plist);
  RECORDER_IMP_CHEN(H5Fcreate, hid_t, (filename, flags, create_plist, access_plist), flags, 0, log_text);
}

hid_t RECORDER_DECL(H5Fopen)(const char *filename, unsigned flags, hid_t access_plist) {
  char log_text[TRACE_LEN];
  sprintf(log_text, "H5Fopen (%s, %d, %d)", filename, flags, access_plist);
  RECORDER_IMP_CHEN(H5Fopen, hid_t, (filename, flags, access_plist), flags, 0, log_text);
}

herr_t RECORDER_DECL(H5Fclose)(hid_t file_id) {
  char log_text[TRACE_LEN];
  sprintf(log_text, "H5Fclose (%d)", file_id);
  RECORDER_IMP_CHEN(H5Fclose, herr_t, (file_id), 0, 0, log_text);
}

// Group Interface
herr_t RECORDER_DECL(H5Gclose)(hid_t group_id) {
  char log_text[TRACE_LEN];
  sprintf(log_text, "H5Gclose (%d)", group_id);
  RECORDER_IMP_CHEN(H5Gclose, herr_t, (group_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Gcreate1)(hid_t loc_id, const char *name, size_t size_hint) {
  char log_text[TRACE_LEN];
  sprintf(log_text, "H5Gcreate1 (%d, %s, %ld)", loc_id, name, size_hint);
  RECORDER_IMP_CHEN(H5Gcreate1, hid_t, (loc_id, name, size_hint), size_hint, 0, log_text);
}

hid_t RECORDER_DECL(H5Gcreate2)(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id) {
  char log_text[TRACE_LEN];
  sprintf(log_text, "H5Gcreate2 (%d, %s, %d, %d, %d)", loc_id, name, lcpl_id, gcpl_id, gapl_id);
  RECORDER_IMP_CHEN(H5Gcreate2, hid_t, (loc_id, name, lcpl_id, gcpl_id, gapl_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Gget_objinfo)(hid_t loc_id, const char *name, hbool_t follow_link, H5G_stat_t *statbuf) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Gget_objinfo (%d, %s, %d, %p)", loc_id, name, follow_link, statbuf);
    RECORDER_IMP_CHEN(H5Gget_objinfo, herr_t, (loc_id, name, follow_link, statbuf), 0, 0, log_text);
}

int RECORDER_DECL(H5Giterate)(hid_t loc_id, const char *name, int *idx, H5G_iterate_t operator, void *operator_data) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Giterate (%d, %s, %p, %d, %p)", loc_id, name, idx, operator, operator_data);
    RECORDER_IMP_CHEN(H5Giterate, int, (loc_id, name, idx, operator, operator_data), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Gopen1)(hid_t loc_id, const char *name) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Gopen1 (%d, %s)", loc_id, name);
    RECORDER_IMP_CHEN(H5Gopen1, hid_t, (loc_id, name), 0, 0, log_text);
}


hid_t RECORDER_DECL(H5Gopen2)(hid_t loc_id, const char *name, hid_t gapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Gopen2 (%d, %s, %d)", loc_id, name, gapl_id);
    RECORDER_IMP_CHEN(H5Gopen2, hid_t, (loc_id, name, gapl_id), 0, 0, log_text);
}

// Dataset interface
herr_t RECORDER_DECL(H5Dclose)(hid_t dataset_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dclose (%d)", dataset_id);
    RECORDER_IMP_CHEN(H5Dclose, herr_t, (dataset_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dcreate1)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id) {
    char log_text[TRACE_LEN];
    char datatype_name[64];
    get_datatype_name(H5Tget_class(type_id), type_id, datatype_name);
    sprintf(log_text, "H5Dcreate1 (%d, %s, %s, %d, %d)", loc_id, name, datatype_name, space_id, dcpl_id);
    RECORDER_IMP_CHEN(H5Dcreate1, hid_t, (loc_id, name, type_id, space_id, dcpl_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dcreate2)(hid_t loc_id, const char *name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dcreate2 (%d, %s, %d, %d, %d, %d, %d)", loc_id, name, dtype_id, space_id, lcpl_id, dcpl_id, dapl_id);
    RECORDER_IMP_CHEN(H5Dcreate2, hid_t, (loc_id, name, dtype_id, space_id, lcpl_id, dcpl_id, dapl_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dget_create_plist)(hid_t dataset_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dget_create_plist (%d)", dataset_id);
    RECORDER_IMP_CHEN(H5Dget_create_plist, hid_t, (dataset_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dget_space)(hid_t dataset_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dget_space (%d)", dataset_id);
    RECORDER_IMP_CHEN(H5Dget_space, hid_t, (dataset_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dget_type)(hid_t dataset_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dget_type (%d)", dataset_id);
    RECORDER_IMP_CHEN(H5Dget_type, hid_t, (dataset_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dopen1)(hid_t loc_id, const char *name) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dopen1 (%d, %s)", loc_id, name);
    RECORDER_IMP_CHEN(H5Dopen1, hid_t, (loc_id, name), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Dopen2)(hid_t loc_id, const char *name, hid_t dapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dopen2 (%d, %s, %d)", loc_id, name, dapl_id);
    RECORDER_IMP_CHEN(H5Dopen2, hid_t, (loc_id, name, dapl_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Dread)(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, void *buf) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dread (%d, %d, %d, %d, %d, %p)", dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf);
    RECORDER_IMP_CHEN(H5Dread, hid_t, (dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Dwrite)(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void *buf) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Dwrite (%d, %d, %d, %d, %d, %p)", dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf);
    RECORDER_IMP_CHEN(H5Dwrite, hid_t, (dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Sclose)(hid_t space_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Sclose (%d)", space_id);
    RECORDER_IMP_CHEN(H5Sclose, herr_t, (space_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Screate)(H5S_class_t type) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Screate (%d)", type);
    RECORDER_IMP_CHEN(H5Screate, hid_t, (type), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Screate_simple)(int rank, const hsize_t *current_dims, const hsize_t *maximum_dims) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Screate_simple (%d, %p, %p)", rank, current_dims, maximum_dims);
    RECORDER_IMP_CHEN(H5Screate_simple, hid_t, (rank, current_dims, maximum_dims), 0, 0, log_text);
}

hssize_t RECORDER_DECL(H5Sget_select_npoints)(hid_t space_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Sget_select_npoints (%d)", space_id);
    RECORDER_IMP_CHEN(H5Sget_select_npoints, hssize_t, (space_id), 0, 0, log_text);
}

int RECORDER_DECL(H5Sget_simple_extent_dims)(hid_t space_id, hsize_t *dims, hsize_t *maxdims) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Sget_simple_extent_dims (%d, %p, %p)", space_id, dims, maxdims);
    RECORDER_IMP_CHEN(H5Sget_simple_extent_dims, int, (space_id, dims, maxdims), 0, 0, log_text);
}

hssize_t RECORDER_DECL(H5Sget_simple_extent_npoints)(hid_t space_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Sget_simple_extent_npoints (%d)", space_id);
    RECORDER_IMP_CHEN(H5Sget_simple_extent_npoints, hssize_t, (space_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Sselect_elements)(hid_t space_id, H5S_seloper_t op, size_t num_elements, const hsize_t *coord) {
    char op_name[32];
    char log_text[TRACE_LEN];
    get_op_name(op, op_name);
    sprintf(log_text, "H5Sselect_elements (%d, %s, %ld, %p)", space_id, op_name, num_elements, coord);
    RECORDER_IMP_CHEN(H5Sselect_elements, herr_t, (space_id, op, num_elements, coord), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Sselect_hyperslab)(hid_t space_id, H5S_seloper_t op, const hsize_t *start, const hsize_t *stride, const hsize_t *count, const hsize_t *block) {
    char op_name[32];
    char log_text[TRACE_LEN];
    get_op_name(op, op_name);
    sprintf(log_text, "H5Sselect_hyperslab (%d, %s, %p, %p, %p, %p)", space_id, op_name, start, stride, count, block);
    RECORDER_IMP_CHEN(H5Sselect_hyperslab, herr_t, (space_id, op, start, stride, count, block), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Sselect_none)(hid_t space_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Sselect_none (%d)", space_id);
    RECORDER_IMP_CHEN(H5Sselect_none, herr_t, (space_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Tclose)(hid_t dtype_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tclose (%d)", dtype_id);
    RECORDER_IMP_CHEN(H5Tclose, herr_t, (dtype_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Tcopy)(hid_t dtype_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tcopy (%d)", dtype_id);
    RECORDER_IMP_CHEN(H5Tcopy, hid_t , (dtype_id), 0, 0, log_text);
}

H5T_class_t RECORDER_DECL(H5Tget_class)(hid_t dtype_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tget_class (%d)", dtype_id);
    RECORDER_IMP_CHEN(H5Tget_class, H5T_class_t, (dtype_id), 0, 0, log_text);
}

size_t RECORDER_DECL(H5Tget_size)(hid_t dtype_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tget_size (%d)", dtype_id);
    RECORDER_IMP_CHEN(H5Tget_size, size_t, (dtype_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Tset_size)(hid_t dtype_id, size_t size) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tset_size (%d, %ld)", dtype_id, size);
    RECORDER_IMP_CHEN(H5Tset_size, herr_t, (dtype_id, size), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Tcreate)(H5T_class_t class, size_t size) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tcreate (%d, %ld)", class, size);
    RECORDER_IMP_CHEN(H5Tcreate, hid_t, (class, size), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Tinsert)(hid_t dtype_id, const char *name, size_t offset, hid_t field_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Tinsert (%d, %s, %ld, %d)", dtype_id, name, offset, field_id);
    RECORDER_IMP_CHEN(H5Tinsert, herr_t, (dtype_id, name, offset, field_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Aclose)(hid_t attr_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aclose (%d)", attr_id);
    RECORDER_IMP_CHEN(H5Aclose, herr_t, (attr_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Acreate1)(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id) {
    char log_text[TRACE_LEN];
    char datatype_name[64];
    get_datatype_name(H5Tget_class(type_id), type_id, datatype_name);
    sprintf(log_text, "H5Acreate1 (%d, %s, %s, %d, %d)", loc_id, attr_name, datatype_name, space_id, acpl_id);
    RECORDER_IMP_CHEN(H5Acreate1, hid_t, (loc_id, attr_name, type_id, space_id, acpl_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Acreate2)(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id) {
    char log_text[TRACE_LEN];
    char datatype_name[64];
    get_datatype_name(H5Tget_class(type_id), type_id, datatype_name);
    sprintf(log_text, "H5Acreate2 (%d, %s, %s, %d, %d, %d)", loc_id, attr_name, datatype_name, space_id, acpl_id, aapl_id);
    RECORDER_IMP_CHEN(H5Acreate2, hid_t, (loc_id, attr_name, type_id, space_id, acpl_id, aapl_id), 0, 0, log_text);
}

ssize_t RECORDER_DECL(H5Aget_name)(hid_t attr_id, size_t buf_size, char *buf) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aget_name (%d, %ld, %p)", attr_id, buf_size, buf);
    RECORDER_IMP_CHEN(H5Aget_name, ssize_t, (attr_id, buf_size, buf), 0, 0, log_text);
}

int RECORDER_DECL(H5Aget_num_attrs)(hid_t loc_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aget_num_attrs (%d)", loc_id);
    RECORDER_IMP_CHEN(H5Aget_num_attrs, int, (loc_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Aget_space)(hid_t attr_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aget_space (%d)", attr_id);
    RECORDER_IMP_CHEN(H5Aget_space, hid_t, (attr_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Aget_type)(hid_t attr_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aget_type (%d)", attr_id);
    RECORDER_IMP_CHEN(H5Aget_type, hid_t, (attr_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Aopen)(hid_t obj_id, const char *attr_name, hid_t aapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aopen (%d, %s, %d)", obj_id, attr_name, aapl_id);
    RECORDER_IMP_CHEN(H5Aopen, hid_t, (obj_id, attr_name, aapl_id), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Aopen_idx)(hid_t loc_id, unsigned int idx) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aopen_idx (%d, %d)", loc_id, idx);
    RECORDER_IMP_CHEN(H5Aopen_idx, hid_t, (loc_id,idx), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Aopen_name)(hid_t loc_id, const char *name) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Aopen_name (%d, %s)", loc_id, name);
    RECORDER_IMP_CHEN(H5Aopen_name, hid_t, (loc_id, name), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Aread)(hid_t attr_id, hid_t mem_type_id, void *buf) {
    char log_text[TRACE_LEN];
    char datatype_name[64];
    get_datatype_name(H5Tget_class(mem_type_id), mem_type_id, datatype_name);
    sprintf(log_text, "H5Aread (%d, %s, %p", attr_id, datatype_name, buf);
    RECORDER_IMP_CHEN(H5Aread, herr_t, (attr_id, mem_type_id, buf), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Awrite)(hid_t attr_id, hid_t mem_type_id, const void *buf) {
    char log_text[TRACE_LEN];
    char datatype_name[64];
    get_datatype_name(H5Tget_class(mem_type_id), mem_type_id, datatype_name);
    sprintf(log_text, "H5Awrite (%d, %s, %p", attr_id, datatype_name, buf);
    RECORDER_IMP_CHEN(H5Awrite, herr_t, (attr_id, mem_type_id, buf), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pclose)(hid_t plist) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pclose (%d)", plist);
    RECORDER_IMP_CHEN(H5Pclose, herr_t, (plist), 0, 0, log_text);
}

hid_t RECORDER_DECL(H5Pcreate)(hid_t cls_id) {
    char prop_list_cls_name[SMALL_BUF_SIZE];
    char log_text[TRACE_LEN];
    get_prop_list_cls_name(cls_id, prop_list_cls_name);
    sprintf(log_text, "H5Pcreate (%s)", prop_list_cls_name);
    RECORDER_IMP_CHEN(H5Pcreate, hid_t, (cls_id), 0, 0, log_text);
}

int RECORDER_DECL(H5Pget_chunk)(hid_t plist, int max_ndims, hsize_t *dims) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pget_chunk (%d, %d, %p)", plist, max_ndims, dims);
    RECORDER_IMP_CHEN(H5Pget_chunk, int, (plist, max_ndims, dims), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pget_mdc_config)(hid_t plist_id, H5AC_cache_config_t *config_ptr) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pget_mdc_config (%d, %p)", plist_id, config_ptr);
    RECORDER_IMP_CHEN(H5Pget_mdc_config, herr_t, (plist_id, config_ptr), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_alignment)(hid_t plist, hsize_t threshold, hsize_t alignment) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_alignment (%d, %d, %d)", plist, threshold, alignment);
    RECORDER_IMP_CHEN(H5Pset_alignment, herr_t, (plist, threshold, alignment), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_chunk)(hid_t plist, int ndims, const hsize_t *dim) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_chunk (%d, %d, %p)", plist, ndims, dim);
    RECORDER_IMP_CHEN(H5Pset_chunk, herr_t, (plist, ndims, dim), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_dxpl_mpio)(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode) {

    char log_text[TRACE_LEN];
    if (xfer_mode == H5FD_MPIO_INDEPENDENT)
        sprintf(log_text, "H5Pset_dxpl_mpio (%d, H5FD_MPIO_INDEPENDENT)", dxpl_id);
    else if (xfer_mode == H5FD_MPIO_COLLECTIVE)
        sprintf(log_text, "H5Pset_dxpl_mpio (%d, H5FD_MPIO_COLLECTIVE)", dxpl_id);
    else
        sprintf(log_text, "H5Pset_dxpl_mpio (%d, %d)", dxpl_id, xfer_mode);
    RECORDER_IMP_CHEN(H5Pset_dxpl_mpio, herr_t, (dxpl_id, xfer_mode), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_fapl_core)(hid_t fapl_id, size_t increment, hbool_t backing_store) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_fapl_core (%d, %d, %p)", fapl_id, increment, backing_store);
    RECORDER_IMP_CHEN(H5Pset_fapl_core, herr_t, (fapl_id, increment, backing_store), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_fapl_mpio)(hid_t fapl_id, MPI_Comm comm, MPI_Info info) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_fapl_mpio (%d, %p, %p)", fapl_id, comm, info);
    RECORDER_IMP_CHEN(H5Pset_fapl_mpio, herr_t, (fapl_id, comm, info), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_fapl_mpiposix)(hid_t fapl_id, MPI_Comm comm, hbool_t use_gpfs_hints) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_fapl_mpiposix (%d, %p, %d)", fapl_id, comm, use_gpfs_hints);
    RECORDER_IMP_CHEN(H5Pset_fapl_mpiposix, herr_t, (fapl_id, comm, use_gpfs_hints), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_istore_k)(hid_t plist, unsigned ik) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_istore_k (%d, %d)", plist, ik);
    RECORDER_IMP_CHEN(H5Pset_istore_k, herr_t, (plist, ik), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_mdc_config)(hid_t plist_id, H5AC_cache_config_t *config_ptr) {
    // int version, hbool_t rpt_fcn_enabled, hbool_t open_trace_file, hbool_t
    // close_trace_file,
    // char trace_file_name [H5AC__MAX_TRACE_FILE_NAME_LEN+1], hbool_t
    // evictions_enabled
    // hbool_t set_initial_size, size_t initial_size, double min_clean_fraction,
    // size_t max_size
    // size_t min_size, long int epoch_length, enum H5C_cache_incr_mode
    // incr_mode, double lower_hr_threshold
    // double increment, hbool_t apply_max_increment, size_t max_increment,
    // enum H5C_cache_flash_incr_mode flash_incr_mode, double flash_multiple,
    // double flash_threshold
    // enum H5C_cache_decr_mode decr_mode, double upper_hr_threshold, double
    // decrement, hbool_t apply_max_decrement
    // size_t max_decrement, int epochs_before_eviction, hbool_t
    // apply_empty_reserve, double empty_reserve
    // int dirty_bytes_threshold, int metadata_write_strategy

    char log_text[1024];
    sprintf(log_text, "H5Pset_mdc_config (%d, [%d;%d;%d;%d;%s;%d;%d;%d;%f;%d;%d;%ld;%d;%f;%f;% d;%d;%d;%f;%f;%d;%f;%f;%d;%d;%d;%d;%f;%d;%d])",
            plist_id, config_ptr->version, config_ptr->rpt_fcn_enabled,
            config_ptr->open_trace_file, config_ptr->close_trace_file,
            config_ptr->trace_file_name, config_ptr->evictions_enabled,
            config_ptr->set_initial_size, config_ptr->initial_size,
            config_ptr->min_clean_fraction, config_ptr->max_size,
            config_ptr->min_size, config_ptr->epoch_length,
            config_ptr->incr_mode, config_ptr->lower_hr_threshold,
            config_ptr->increment, config_ptr->apply_max_increment,
            config_ptr->max_increment, config_ptr->flash_incr_mode,
            config_ptr->flash_multiple, config_ptr->flash_threshold,
            config_ptr->decr_mode, config_ptr->upper_hr_threshold,
            config_ptr->decr_mode, config_ptr->apply_max_decrement,
            config_ptr->max_decrement, config_ptr->epochs_before_eviction,
            config_ptr->apply_empty_reserve, config_ptr->empty_reserve,
            config_ptr->dirty_bytes_threshold,
            config_ptr->metadata_write_strategy);
    RECORDER_IMP_CHEN(H5Pset_mdc_config, herr_t, (plist_id, config_ptr), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Pset_meta_block_size)(hid_t fapl_id, hsize_t size) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Pset_meta_block_size (%d, %d)", fapl_id, size);
    RECORDER_IMP_CHEN(H5Pset_meta_block_size, herr_t, (fapl_id, size), 0, 0, log_text);
}

htri_t RECORDER_DECL(H5Lexists)(hid_t loc_id, const char *name, hid_t lapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Lexists (%d, %s, %d)", loc_id, name, lapl_id);
    RECORDER_IMP_CHEN(H5Lexists, htri_t, (loc_id, name, lapl_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Lget_val)(hid_t link_loc_id, const char *link_name, void *linkval_buff, size_t size, hid_t lapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Lget_val (%d, %s, %p, %d, %d)", link_loc_id, link_name, linkval_buff, size, lapl_id);
    RECORDER_IMP_CHEN(H5Lget_val, herr_t, (link_loc_id, link_name, linkval_buff, size, lapl_id), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Literate)(hid_t group_id, H5_index_t index_type, H5_iter_order_t order, hsize_t *idx, H5L_iterate_t op, void *op_data) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Literate (%d, %d, %d, %p, %d, %p)", group_id, index_type, order, idx, op, op_data);
    RECORDER_IMP_CHEN(H5Literate, htri_t, (group_id, index_type, order, idx, op, op_data), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Oclose)(hid_t object_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Oclose (%d)", object_id);
    RECORDER_IMP_CHEN(H5Oclose, herr_t, (object_id), 0, 0, log_text);
}

/*
 * Exists in 1.8 not in 1.10, in 1.10 they are H5Oget_info1; H5Oget_info2
herr_t RECORDER_DECL(H5Oget_info)(hid_t object_id, H5O_info_t *object_info) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Oget_info (%d, %p)", object_id, object_info);
    RECORDER_IMP_CHEN(H5Oget_info, herr_t, (object_id, object_info), 0, 0, log_text);
}

herr_t RECORDER_DECL(H5Oget_info_by_name)(hid_t loc_id, const char *object_name, H5O_info_t *object_info, hid_t lapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Oget_info_by_name (%d, %s, %p, %d)", loc_id, object_name, object_info, lapl_id);
    RECORDER_IMP_CHEN(H5Oget_info_by_name, herr_t, (loc_id, object_name, object_info, lapl_id), 0, 0, log_text);
}
*/

hid_t RECORDER_DECL(H5Oopen)(hid_t loc_id, const char *name, hid_t lapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Oopen (%d, %s, %d)", loc_id, name, lapl_id);
    RECORDER_IMP_CHEN(H5Oopen, hid_t, (loc_id, name, lapl_id), 0, 0, log_text);
}
