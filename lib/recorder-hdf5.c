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

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include "mpi.h"
#include "recorder.h"
#include "hdf5.h"

#ifdef RECORDER_PRELOAD

#define __USE_GNU
#include <dlfcn.h>
#include <stdlib.h>

#define RECORDER_FORWARD_DECL(name, ret, args) ret(*__real_##name) args = NULL;

#define RECORDER_DECL(__name) __name

#define MAP_OR_FAIL(func)                                                      \
  if (!(__real_##func)) {                                                      \
    __real_##func = dlsym(RTLD_NEXT, #func);                                   \
    if (!(__real_##func)) {                                                    \
      fprintf(stderr, "Recorder failed to map symbol: %s\n", #func);            \
      exit(1);                                                                 \
    }                                                                          \
  }

#else

#define RECORDER_FORWARD_DECL(name, ret, args) extern ret __real_##name args;

#define RECORDER_DECL(__name) __wrap_##__name

#define MAP_OR_FAIL(func)

#endif

#define SMALL_BUF_SIZE 128
#define LARGE_BUF_SIZE 1024

char *func_list[] = {
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
    "H5Oopen",                      "ENDLIST"};

int get_func_id(const char *func_name) {
  int i = 0;
  while (func_list[i] != "ENDLIST") {
    if (func_list[i] == func_name)
      return i;
    else
      i++;
  }
  return -1;
}

void change_char(const char *str, char oldChar, char *newStr, char newChar) {
  int len = strlen(str);
  strncpy(newStr, str, len);
  newStr[len] = '\0';
  char *newStrPtr = newStr;
  while ((newStrPtr = strchr(newStr, oldChar)) != NULL)
    *newStrPtr++ = newChar;
}

void print_arr(const hsize_t *array, int array_size, char *string) {
  char *tmp = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
  int i;

  char open_bracket[] = "{";
  char close_bracket[] = "}";
  char zero_str[] = "0";
  strcpy(string, open_bracket);
  for (i = 0; i < array_size - 1; i++) {
    strcpy(tmp, zero_str);
    sprintf(tmp, "%lld;", array[i]);
    strcat(string, tmp);
  }
  strcpy(tmp, zero_str);
  sprintf(tmp, "%lld", array[array_size - 1]);
  strcat(string, tmp);
  strcat(string, close_bracket);

  free(tmp);
}

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
/* NOTE: using HDF5 1.8 version */

/* File Interface */
RECORDER_FORWARD_DECL(H5Fcreate, hid_t,
                      (const char *filename, unsigned flags, hid_t create_plist,
                       hid_t access_plist));
RECORDER_FORWARD_DECL(H5Fopen, hid_t, (const char *filename, unsigned flags,
                                       hid_t access_plist));
RECORDER_FORWARD_DECL(H5Fclose, herr_t, (hid_t file_id));

/* Group Interface */
RECORDER_FORWARD_DECL(H5Gclose, herr_t, (hid_t group_id));
RECORDER_FORWARD_DECL(H5Gcreate1, hid_t,
                      (hid_t loc_id, const char *name, size_t size_hint));
RECORDER_FORWARD_DECL(H5Gcreate2, hid_t,
                      (hid_t loc_id, const char *name, hid_t lcpl_id,
                       hid_t gcpl_id, hid_t gapl_id));
RECORDER_FORWARD_DECL(H5Gget_objinfo, herr_t,
                      (hid_t loc_id, const char *name, hbool_t follow_link,
                       H5G_stat_t *statbuf));
RECORDER_FORWARD_DECL(H5Giterate, int,
                      (hid_t loc_id, const char *name, int *idx,
                       H5G_iterate_t operator, void *operator_data));
RECORDER_FORWARD_DECL(H5Gopen1, hid_t, (hid_t loc_id, const char *name));
RECORDER_FORWARD_DECL(H5Gopen2, hid_t,
                      (hid_t loc_id, const char *name, hid_t gapl_id));

/* Dataset Interface  */
RECORDER_FORWARD_DECL(H5Dclose, herr_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dcreate1, hid_t,
                      (hid_t loc_id, const char *name, hid_t type_id,
                       hid_t space_id, hid_t dcpl_id));
RECORDER_FORWARD_DECL(H5Dcreate2, hid_t,
                      (hid_t loc_id, const char *name, hid_t dtype_id,
                       hid_t space_id, hid_t lcpl_id, hid_t dcpl_id,
                       hid_t dapl_id));
RECORDER_FORWARD_DECL(H5Dget_create_plist, hid_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dget_space, hid_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dget_type, hid_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dopen1, hid_t, (hid_t loc_id, const char *name));
RECORDER_FORWARD_DECL(H5Dopen2, hid_t,
                      (hid_t loc_id, const char *name, hid_t dapl_id));
RECORDER_FORWARD_DECL(H5Dread, herr_t,
                      (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id,
                       hid_t file_space_id, hid_t xfer_plist_id, void *buf));
RECORDER_FORWARD_DECL(H5Dwrite, herr_t,
                      (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id,
                       hid_t file_space_id, hid_t xfer_plist_id,
                       const void *buf));

/* Dataspace Interface */
RECORDER_FORWARD_DECL(H5Sclose, herr_t, (hid_t space_id));

RECORDER_FORWARD_DECL(H5Screate, hid_t, (H5S_class_t type));

RECORDER_FORWARD_DECL(H5Screate_simple, hid_t,
                      (int rank, const hsize_t *current_dims,
                       const hsize_t *maximum_dims));
RECORDER_FORWARD_DECL(H5Sget_select_npoints, hssize_t, (hid_t space_id));
RECORDER_FORWARD_DECL(H5Sget_simple_extent_dims, int,
                      (hid_t space_id, hsize_t * dims, hsize_t * maxdims));
RECORDER_FORWARD_DECL(H5Sget_simple_extent_npoints, hssize_t, (hid_t space_id));
RECORDER_FORWARD_DECL(H5Sselect_elements, herr_t,
                      (hid_t space_id, H5S_seloper_t op, size_t num_elements,
                       const hsize_t *coord));
RECORDER_FORWARD_DECL(H5Sselect_hyperslab, herr_t,
                      (hid_t space_id, H5S_seloper_t op, const hsize_t *start,
                       const hsize_t *stride, const hsize_t *count,
                       const hsize_t *block));
RECORDER_FORWARD_DECL(H5Sselect_none, herr_t, (hid_t space_id));

/* Datatype Interface */

RECORDER_FORWARD_DECL(H5Tclose, herr_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tcopy, hid_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tget_class, H5T_class_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tget_size, size_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tset_size, herr_t, (hid_t dtype_id, size_t size));
RECORDER_FORWARD_DECL(H5Tcreate, hid_t, (H5T_class_t class, size_t size));
RECORDER_FORWARD_DECL(H5Tinsert, herr_t, (hid_t dtype_id, const char *name,
                                          size_t offset, hid_t field_id));

/* Attribute Interface */

RECORDER_FORWARD_DECL(H5Aclose, herr_t, (hid_t attr_id));
RECORDER_FORWARD_DECL(H5Acreate1, hid_t,
                      (hid_t loc_id, const char *attr_name, hid_t type_id,
                       hid_t space_id, hid_t acpl_id));
RECORDER_FORWARD_DECL(H5Acreate2, hid_t,
                      (hid_t loc_id, const char *attr_name, hid_t type_id,
                       hid_t space_id, hid_t acpl_id, hid_t aapl_id));
RECORDER_FORWARD_DECL(H5Aget_name, ssize_t,
                      (hid_t attr_id, size_t buf_size, char *buf));
RECORDER_FORWARD_DECL(H5Aget_num_attrs, int, (hid_t loc_id));
RECORDER_FORWARD_DECL(H5Aget_space, hid_t, (hid_t attr_id));
RECORDER_FORWARD_DECL(H5Aget_type, hid_t, (hid_t attr_id));
RECORDER_FORWARD_DECL(H5Aopen, hid_t,
                      (hid_t obj_id, const char *attr_name, hid_t aapl_id));
RECORDER_FORWARD_DECL(H5Aopen_idx, hid_t, (hid_t loc_id, unsigned int idx));
RECORDER_FORWARD_DECL(H5Aopen_name, hid_t, (hid_t loc_id, const char *name));
RECORDER_FORWARD_DECL(H5Aread, herr_t,
                      (hid_t attr_id, hid_t mem_type_id, void *buf));
RECORDER_FORWARD_DECL(H5Awrite, herr_t,
                      (hid_t attr_id, hid_t mem_type_id, const void *buf));

/* Property List Interface */
RECORDER_FORWARD_DECL(H5Pclose, herr_t, (hid_t plist));
RECORDER_FORWARD_DECL(H5Pcreate, hid_t, (hid_t cls_id));
RECORDER_FORWARD_DECL(H5Pget_chunk, int,
                      (hid_t plist, int max_ndims, hsize_t *dims));
RECORDER_FORWARD_DECL(H5Pget_mdc_config, herr_t,
                      (hid_t plist_id, H5AC_cache_config_t * config_ptr));
RECORDER_FORWARD_DECL(H5Pset_alignment, herr_t,
                      (hid_t plist, hsize_t threshold, hsize_t alignment));
RECORDER_FORWARD_DECL(H5Pset_chunk, herr_t,
                      (hid_t plist, int ndims, const hsize_t *dim));
RECORDER_FORWARD_DECL(H5Pset_dxpl_mpio, herr_t,
                      (hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode));
RECORDER_FORWARD_DECL(H5Pset_fapl_core, herr_t,
                      (hid_t fapl_id, size_t increment, hbool_t backing_store));
RECORDER_FORWARD_DECL(H5Pset_fapl_mpio, herr_t,
                      (hid_t fapl_id, MPI_Comm comm, MPI_Info info));
RECORDER_FORWARD_DECL(H5Pset_fapl_mpiposix, herr_t,
                      (hid_t fapl_id, MPI_Comm comm, hbool_t use_gpfs_hints));
RECORDER_FORWARD_DECL(H5Pset_istore_k, herr_t, (hid_t plist, unsigned ik));
RECORDER_FORWARD_DECL(H5Pset_mdc_config, herr_t,
                      (hid_t plist_id, H5AC_cache_config_t * config_ptr));
RECORDER_FORWARD_DECL(H5Pset_meta_block_size, herr_t,
                      (hid_t fapl_id, hsize_t size));

/* Link Interface */
RECORDER_FORWARD_DECL(H5Lexists, htri_t,
                      (hid_t loc_id, const char *name, hid_t lapl_id));
RECORDER_FORWARD_DECL(H5Lget_val, herr_t,
                      (hid_t link_loc_id, const char *link_name,
                       void *linkval_buff, size_t size, hid_t lapl_id));
RECORDER_FORWARD_DECL(H5Literate, herr_t,
                      (hid_t group_id, H5_index_t index_type,
                       H5_iter_order_t order, hsize_t * idx, H5L_iterate_t op,
                       void *op_data));

/* Object Interface */
RECORDER_FORWARD_DECL(H5Oclose, herr_t, (hid_t object_id));
RECORDER_FORWARD_DECL(H5Oget_info, herr_t,
                      (hid_t object_id, H5O_info_t * object_info));
RECORDER_FORWARD_DECL(H5Oget_info_by_name, herr_t,
                      (hid_t loc_id, const char *object_name,
                       H5O_info_t *object_info, hid_t lapl_id));
RECORDER_FORWARD_DECL(H5Oopen, hid_t,
                      (hid_t loc_id, const char *name, hid_t lapl_id));

static struct recorder_file_runtime *recorder_file_by_hid(int hid);

hid_t RECORDER_DECL(H5Fcreate)(const char *filename, unsigned flags,
                               hid_t create_plist, hid_t access_plist) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Fcreate);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    if (flags == H5F_ACC_TRUNC)
      fprintf(__recorderfh, "%.5f H5Fcreate (%s,H5F_ACC_TRUNC,%d,%d)", tm1,
              filename, create_plist, access_plist);
    else if (flags == H5F_ACC_EXCL)
      fprintf(__recorderfh, "%.5f H5Fcreate (%s,H5F_ACC_EXCL,%d,%d)", tm1,
              filename, create_plist, access_plist);
    else
      fprintf(__recorderfh, "%.5f H5Fcreate (%s,%d,%d,%d)", tm1, filename,
              flags, create_plist, access_plist);
  }
#endif

  ret = __real_H5Fcreate(filename, flags, create_plist, access_plist);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Fopen)(const char *filename, unsigned flags,
                             hid_t access_plist) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Fopen);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Fopen (%s,%d,%d)", tm1, filename, flags,
            access_plist);
  }
#endif

  ret = __real_H5Fopen(filename, flags, access_plist);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Fclose)(hid_t file_id) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Fclose);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Fclose (%d)", tm1, file_id);
  }
#endif

  ret = __real_H5Fclose(file_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

// Group Interface
herr_t RECORDER_DECL(H5Gclose)(hid_t group_id) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Gclose);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Gclose (%d)", tm1, group_id);
  }
#endif

  ret = __real_H5Gclose(group_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Gcreate1)(hid_t loc_id, const char *name,
                                size_t size_hint) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Gcreate1);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Gcreate1 (%d,%s,%d)", tm1, loc_id, new_name,
            size_hint);
    free(new_name);
  }
#endif

  ret = __real_H5Gcreate1(loc_id, name, size_hint);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Gcreate2)(hid_t loc_id, const char *name, hid_t lcpl_id,
                                hid_t gcpl_id, hid_t gapl_id) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Gcreate2);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Gcreate2 (%d,%s,%d,%d,%d)", tm1, loc_id,
            new_name, lcpl_id, gcpl_id, gapl_id);
    free(new_name);
  }
#endif

  ret = __real_H5Gcreate2(loc_id, name, lcpl_id, gcpl_id, gapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Gget_objinfo)(hid_t loc_id, const char *name,
                                     hbool_t follow_link, H5G_stat_t *statbuf) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Gget_objinfo);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Gget_objinfo (%d,%s,%d,statbuf)", tm1, loc_id,
            new_name, follow_link);
    free(new_name);
  }
#endif

  ret = __real_H5Gget_objinfo(loc_id, name, follow_link, statbuf);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

int RECORDER_DECL(H5Giterate)(hid_t loc_id, const char *name, int *idx,
                              H5G_iterate_t operator, void *operator_data) {
  int ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Giterate);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Giterate (%d,%s,%d,%d,%d)", tm1, loc_id,
            new_name, idx, operator, operator_data);
    free(new_name);
  }
#endif

  ret = __real_H5Giterate(loc_id, name, idx, operator, operator_data);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Gopen1)(hid_t loc_id, const char *name) {
  hid_t ret;
  double tm1, tm2, duration;

  MAP_OR_FAIL(H5Gopen1);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Gopen1 (%d,%s)", tm1, loc_id, new_name);
    free(new_name);
  }
#endif

  ret = __real_H5Gopen1(loc_id, name);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Gopen2)(hid_t loc_id, const char *name, hid_t gapl_id) {
  hid_t ret;
  double tm1, tm2, duration;
  MAP_OR_FAIL(H5Gopen2);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Gopen2 (%d,%s,%d)", tm1, loc_id, new_name,
            gapl_id);
    free(new_name);
  }
#endif

  ret = __real_H5Gopen2(loc_id, name, gapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

// Dataset interface
herr_t RECORDER_DECL(H5Dclose)(hid_t dataset_id) {
  herr_t ret;
  double tm1, tm2, duration;

  MAP_OR_FAIL(H5Dclose);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Dclose (%d)", tm1, dataset_id);
  }
#endif

  ret = __real_H5Dclose(dataset_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Dcreate1)(hid_t loc_id, const char *name, hid_t type_id,
                                hid_t space_id, hid_t dcpl_id) {
  hid_t ret;
  double tm1, tm2, duration;

  MAP_OR_FAIL(H5Dcreate1);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(type_id), type_id, datatype_name);
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Dcreate1 (%d,%s,%s,%d,%d)", tm1, loc_id,
            new_name, datatype_name, space_id, dcpl_id);
    free(new_name);
    free(datatype_name);
  }
#endif

  ret = __real_H5Dcreate1(loc_id, name, type_id, space_id, dcpl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return ret;
}

hid_t RECORDER_DECL(H5Dcreate2)(hid_t loc_id, const char *name, hid_t dtype_id,
                                hid_t space_id, hid_t lcpl_id, hid_t dcpl_id,
                                hid_t dapl_id) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Dcreate2);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();

  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(dtype_id), dtype_id, datatype_name);
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Dcreate2 (%d,%s,%s,%d,%d,%d,%d)", tm1, loc_id,
            new_name, datatype_name, space_id, lcpl_id, dcpl_id, dapl_id);
    free(new_name);
    free(datatype_name);
  }
#endif

  ret = __real_H5Dcreate2(loc_id, name, dtype_id, space_id, lcpl_id, dcpl_id,
                          dapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return ret;
}

hid_t RECORDER_DECL(H5Dget_create_plist)(hid_t dataset_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Dget_create_plist);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Dget_create_plist (%d)", tm1, dataset_id);
  }
#endif

  ret = __real_H5Dget_create_plist(dataset_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return ret;
}

hid_t RECORDER_DECL(H5Dget_space)(hid_t dataset_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Dget_space);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Dget_space (%d)", tm1, dataset_id);
  }
#endif

  ret = __real_H5Dget_space(dataset_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Dget_type)(hid_t dataset_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Dget_type);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Dget_type (%d)", tm1, dataset_id);
  }
#endif

  ret = __real_H5Dget_type(dataset_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Dopen1)(hid_t loc_id, const char *name) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Dopen1);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Dopen1 (%d,%s)", tm1, loc_id, new_name);
    free(new_name);
  }
#endif

  ret = __real_H5Dopen1(loc_id, name);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Dopen2)(hid_t loc_id, const char *name, hid_t dapl_id) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Dopen2);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Dopen2 (%d,%s,%d)", tm1, loc_id, new_name,
            dapl_id);
    free(new_name);
  }
#endif

  ret = __real_H5Dopen2(loc_id, name, dapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Dread)(hid_t dataset_id, hid_t mem_type_id,
                              hid_t mem_space_id, hid_t file_space_id,
                              hid_t xfer_plist_id, void *buf) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Dread);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  hssize_t npoints = H5Sget_select_npoints(mem_space_id);
  size_t size_of_data_type = H5Tget_size(mem_type_id);
  long long total_size_read = npoints * size_of_data_type;
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(mem_type_id), mem_type_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Dread (%d,%s,%d,%d,%d,%lld)", tm1, dataset_id,
            datatype_name, mem_space_id, file_space_id, xfer_plist_id,
            total_size_read);
    free(datatype_name);
  }
#endif

  ret = __real_H5Dread(dataset_id, mem_type_id, mem_space_id, file_space_id,
                       xfer_plist_id, buf);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Dwrite)(hid_t dataset_id, hid_t mem_type_id,
                               hid_t mem_space_id, hid_t file_space_id,
                               hid_t xfer_plist_id, const void *buf) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Dwrite);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  size_t size_of_data_type = H5Tget_size(mem_type_id);
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(mem_type_id), mem_type_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Dwrite (%d,%s,%d,%d,%d)", tm1, dataset_id,
            datatype_name, mem_space_id, file_space_id, xfer_plist_id);
    free(datatype_name);
  }
#endif

  ret = __real_H5Dwrite(dataset_id, mem_type_id, mem_space_id, file_space_id,
                        xfer_plist_id, buf);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

// Dataspace interface
herr_t RECORDER_DECL(H5Sclose)(hid_t space_id) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sclose);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Sclose (%d)", tm1, space_id);
  }
#endif

  ret = __real_H5Sclose(space_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Screate)(H5S_class_t type) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Screate);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();

  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Screate (%d)", tm1, type);
  }
#endif

  ret = __real_H5Screate(type);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Screate_simple)(int rank, const hsize_t *current_dims,
                                      const hsize_t *maximum_dims) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Screate_simple);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();

  if (__recorderfh != NULL && depth == 1) {
    char *current_dims_str = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    if (current_dims != NULL)
      print_arr(current_dims, rank, current_dims_str);
    else
      strcpy(current_dims_str, "NULL");

    char *maximum_dims_str = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    if (maximum_dims != NULL)
      print_arr(maximum_dims, rank, maximum_dims_str);
    else
      strcpy(maximum_dims_str, "NULL");

    // fprintf(__recorderfh,"%.5f H5Screate_simple
    // (%d,current_dims,maximum_dims)",tm1, rank);
    fprintf(__recorderfh, "%.5f H5Screate_simple (%d,%s,%s)", tm1, rank,
            current_dims_str, maximum_dims_str);

    free(current_dims_str);
    free(maximum_dims_str);
  }
#endif

  ret = __real_H5Screate_simple(rank, current_dims, maximum_dims);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hssize_t RECORDER_DECL(H5Sget_select_npoints)(hid_t space_id) {
  hssize_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sget_select_npoints);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Sget_select_npoints (%d)", tm1, space_id);
  }
#endif

  ret = __real_H5Sget_select_npoints(space_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return ret;
}

int RECORDER_DECL(H5Sget_simple_extent_dims)(hid_t space_id, hsize_t *dims,
                                             hsize_t *maxdims) {
  int ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sget_simple_extent_dims);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Sget_simple_extent_dims (%d,%d,%d)", tm1,
            space_id, dims, maxdims);
  }
#endif

  ret = __real_H5Sget_simple_extent_dims(space_id, dims, maxdims);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hssize_t RECORDER_DECL(H5Sget_simple_extent_npoints)(hid_t space_id) {
  hssize_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sget_simple_extent_npoints);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Sget_simple_extent_npoints (%d)", tm1,
            space_id);
  }
#endif

  ret = __real_H5Sget_simple_extent_npoints(space_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return ret;
}

herr_t RECORDER_DECL(H5Sselect_elements)(hid_t space_id, H5S_seloper_t op,
                                         size_t num_elements,
                                         const hsize_t *coord) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sselect_elements);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Sselect_elements (%d,%d,%d,%d)", tm1,
            space_id, op, num_elements, coord);
  }
#endif

  ret = __real_H5Sselect_elements(space_id, op, num_elements, coord);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Sselect_hyperslab)(hid_t space_id, H5S_seloper_t op,
                                          const hsize_t *start,
                                          const hsize_t *stride,
                                          const hsize_t *count,
                                          const hsize_t *block) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sselect_hyperslab);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();

  if (__recorderfh != NULL && depth == 1) {
    // The start, stride, count, and block arrays must be the same size as the
    // rank of the dataspace.
    int rank = H5Sget_simple_extent_ndims(space_id);

    char *start_str = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    if (start != NULL)
      print_arr(start, rank, start_str);
    else
      strcpy(start_str, "NULL");

    char *stride_str = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    if (stride != NULL)
      print_arr(stride, rank, stride_str);
    else
      strcpy(stride_str, "NULL");

    char *count_str = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    if (count != NULL)
      print_arr(count, rank, count_str);
    else
      strcpy(count_str, "NULL");

    char *block_str = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    if (block != NULL)
      print_arr(block, rank, block_str);
    else
      strcpy(block_str, "NULL");

    char *op_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_op_name(op, op_name);
    fprintf(__recorderfh, "%.5f H5Sselect_hyperslab (%d,%s,%s,%s,%s,%s)", tm1,
            space_id, op_name, start_str, stride_str, count_str, block_str);

    free(op_name);
    free(start_str);
    free(stride_str);
    free(count_str);
    free(block_str);
  }
#endif

  ret = __real_H5Sselect_hyperslab(space_id, op, start, stride, count, block);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Sselect_none)(hid_t space_id) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Sselect_none);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Sselect_none (%d)", tm1, space_id);
  }
#endif

  ret = __real_H5Sselect_none(space_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Tclose)(hid_t dtype_id) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Tclose);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Tclose (%d)", tm1, dtype_id);
  }
#endif

  ret = __real_H5Tclose(dtype_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Tcopy)(hid_t dtype_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Tcopy);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(dtype_id), dtype_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Tcopy (%s)", tm1, datatype_name);
    free(datatype_name);
  }
#endif

  ret = __real_H5Tcopy(dtype_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

H5T_class_t RECORDER_DECL(H5Tget_class)(hid_t dtype_id) {
  H5T_class_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Tget_class);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Tget_class (%d)", tm1, dtype_id);
  }
#endif

  ret = __real_H5Tget_class(dtype_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

size_t RECORDER_DECL(H5Tget_size)(hid_t dtype_id) {
  size_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Tget_size);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Tget_size (%d)", tm1, dtype_id);
  }
#endif

  ret = __real_H5Tget_size(dtype_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return ret;
}

herr_t RECORDER_DECL(H5Tset_size)(hid_t dtype_id, size_t size) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Tset_size);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Tset_size (%d,%d)", tm1, dtype_id, size);
  }
#endif

  ret = __real_H5Tset_size(dtype_id, size);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Tcreate)(H5T_class_t class, size_t size) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Tcreate);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    if (class == H5T_COMPOUND)
      fprintf(__recorderfh, "%.5f H5Tcreate (H5T_COMPOUND,%d)", tm1, size);
    else if (class == H5T_OPAQUE)
      fprintf(__recorderfh, "%.5f H5Tcreate (H5T_OPAQUE,%d)", tm1, size);
    else if (class == H5T_ENUM)
      fprintf(__recorderfh, "%.5f H5Tcreate (H5T_ENUM,%d)", tm1, size);
    else if (class == H5T_STRING)
      fprintf(__recorderfh, "%.5f H5Tcreate (H5T_STRING,%d)", tm1, size);
    else
      fprintf(__recorderfh, "%.5f H5Tcreate (%d,%d)", tm1, class, size);
  }
#endif

  ret = __real_H5Tcreate(class, size);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Tinsert)(hid_t dtype_id, const char *name, size_t offset,
                                hid_t field_id) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Tinsert);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Tinsert (%d,%s,%d,%d)", tm1, dtype_id, name,
            offset, field_id);
  }
#endif

  ret = __real_H5Tinsert(dtype_id, name, offset, field_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Aclose)(hid_t attr_id) {
  herr_t ret;
  double tm1, tm2, duration;
  MAP_OR_FAIL(H5Aclose);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aclose (%d)", tm1, attr_id);
  }
#endif

  ret = __real_H5Aclose(attr_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Acreate1)(hid_t loc_id, const char *attr_name,
                                hid_t type_id, hid_t space_id, hid_t acpl_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Acreate1);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(type_id), type_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Acreate1 (%d,%s,%s,%d,%d)", tm1, loc_id,
            attr_name, datatype_name, space_id, acpl_id);
    free(datatype_name);
  }
#endif

  ret = __real_H5Acreate1(loc_id, attr_name, type_id, space_id, acpl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Acreate2)(hid_t loc_id, const char *attr_name,
                                hid_t type_id, hid_t space_id, hid_t acpl_id,
                                hid_t aapl_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Acreate2);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(type_id), type_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Acreate2 (%d,%s,%s,%d,%d,%d)", tm1, loc_id,
            attr_name, datatype_name, space_id, acpl_id, aapl_id);
    free(datatype_name);
  }
#endif

  ret =
      __real_H5Acreate2(loc_id, attr_name, type_id, space_id, acpl_id, aapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

ssize_t RECORDER_DECL(H5Aget_name)(hid_t attr_id, size_t buf_size, char *buf) {
  ssize_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aget_name);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aget_name (%d,%d,%s)", tm1, attr_id, buf_size,
            buf);
  }
#endif

  ret = __real_H5Aget_name(attr_id, buf_size, buf);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d <%.5f>\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

int RECORDER_DECL(H5Aget_num_attrs)(hid_t loc_id) {
  int ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aget_num_attrs);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aget_num_attrs (%d)", tm1, loc_id);
  }
#endif

  ret = __real_H5Aget_num_attrs(loc_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Aget_space)(hid_t attr_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aget_space);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aget_space (%d)", tm1, attr_id);
  }
#endif

  ret = __real_H5Aget_space(attr_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Aget_type)(hid_t attr_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aget_type);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aget_type (%d)", tm1, attr_id);
  }
#endif

  ret = __real_H5Aget_type(attr_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Aopen)(hid_t obj_id, const char *attr_name,
                             hid_t aapl_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aopen);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aopen (%d,%s,%d)", tm1, obj_id, attr_name,
            aapl_id);
  }
#endif

  ret = __real_H5Aopen(obj_id, attr_name, aapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Aopen_idx)(hid_t loc_id, unsigned int idx) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aopen_idx);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aopen_idx (%d,%d)", tm1, loc_id, idx);
  }
#endif

  ret = __real_H5Aopen_idx(loc_id, idx);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Aopen_name)(hid_t loc_id, const char *name) {
  hid_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Aopen_name);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Aopen_name (%d,%s)", tm1, loc_id, name);
  }
#endif

  ret = __real_H5Aopen_name(loc_id, name);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Aread)(hid_t attr_id, hid_t mem_type_id, void *buf) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Aread);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(mem_type_id), mem_type_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Aread (%d,%d)", tm1, attr_id, datatype_name);
    free(datatype_name);
  }
#endif

  ret = __real_H5Aread(attr_id, mem_type_id, buf);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Awrite)(hid_t attr_id, hid_t mem_type_id,
                               const void *buf) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Awrite);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *datatype_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_datatype_name(H5Tget_class(mem_type_id), mem_type_id, datatype_name);
    fprintf(__recorderfh, "%.5f H5Awrite (%d,%s)", tm1, attr_id, datatype_name);
    free(datatype_name);
  }
#endif

  ret = __real_H5Awrite(attr_id, mem_type_id, buf);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pclose)(hid_t plist) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pclose);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pclose (%d)", tm1, plist);
  }
#endif

  ret = __real_H5Pclose(plist);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Pcreate)(hid_t cls_id) {
  hid_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pcreate);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *prop_list_cls_name = (char *)malloc(sizeof(char) * SMALL_BUF_SIZE);
    get_prop_list_cls_name(cls_id, prop_list_cls_name);
    fprintf(__recorderfh, "%.5f H5Pcreate (%s)", tm1, prop_list_cls_name);
    free(prop_list_cls_name);
  }
#endif

  ret = __real_H5Pcreate(cls_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

int RECORDER_DECL(H5Pget_chunk)(hid_t plist, int max_ndims, hsize_t *dims) {
  int ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pget_chunk);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pget_chunk (%d,%d,%d)", tm1, plist, max_ndims,
            dims);
  }
#endif

  ret = __real_H5Pget_chunk(plist, max_ndims, dims);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pget_mdc_config)(hid_t plist_id,
                                        H5AC_cache_config_t *config_ptr) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pget_mdc_config);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pget_mdc_config (%d,config_ptr)", tm1,
            plist_id);
  }
#endif

  ret = __real_H5Pget_mdc_config(plist_id, config_ptr);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_alignment)(hid_t plist, hsize_t threshold,
                                       hsize_t alignment) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_alignment);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();

  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pset_alignment (%d,%d,%d)", tm1, plist,
            threshold, alignment);
  }
#endif

  ret = __real_H5Pset_alignment(plist, threshold, alignment);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_chunk)(hid_t plist, int ndims, const hsize_t *dim) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_chunk);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pset_chunk (%d,%d,%d)", tm1, plist, ndims,
            ndims * sizeof(hsize_t));
  }
#endif

  ret = __real_H5Pset_chunk(plist, ndims, dim);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_dxpl_mpio)(hid_t dxpl_id,
                                       H5FD_mpio_xfer_t xfer_mode) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_dxpl_mpio);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    if (xfer_mode == H5FD_MPIO_INDEPENDENT)
      fprintf(__recorderfh, "%.5f H5Pset_dxpl_mpio (%d,H5FD_MPIO_INDEPENDENT)",
              tm1, dxpl_id);
    else if (xfer_mode == H5FD_MPIO_COLLECTIVE)
      fprintf(__recorderfh, "%.5f H5Pset_dxpl_mpio (%d,H5FD_MPIO_COLLECTIVE)",
              tm1, dxpl_id);
    else
      fprintf(__recorderfh, "%.5f H5Pset_dxpl_mpio (%d,%d)", tm1, dxpl_id,
              xfer_mode);
  }
#endif

  ret = __real_H5Pset_dxpl_mpio(dxpl_id, xfer_mode);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_fapl_core)(hid_t fapl_id, size_t increment,
                                       hbool_t backing_store) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_fapl_core);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pset_fapl_core (%d,%d,%d)", tm1, fapl_id,
            increment, backing_store);
  }
#endif

  ret = __real_H5Pset_fapl_core(fapl_id, increment, backing_store);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_fapl_mpio)(hid_t fapl_id, MPI_Comm comm,
                                       MPI_Info info) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_fapl_mpio);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *comm_name = comm2name(comm);
    fprintf(__recorderfh, "%.5f H5Pset_fapl_mpio (%d,%s,%d)", tm1, fapl_id,
            comm_name, info);
  }
#endif

  ret = __real_H5Pset_fapl_mpio(fapl_id, comm, info);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_fapl_mpiposix)(hid_t fapl_id, MPI_Comm comm,
                                           hbool_t use_gpfs_hints) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_fapl_mpiposix);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pset_fapl_mpiposix (%d,%d,%d)", tm1, fapl_id,
            comm, use_gpfs_hints);
  }
#endif

  ret = __real_H5Pset_fapl_mpiposix(fapl_id, comm, use_gpfs_hints);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_istore_k)(hid_t plist, unsigned ik) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_istore_k);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pset_istore_k (%d,%d)", tm1, plist, ik);
  }
#endif

  ret = __real_H5Pset_istore_k(plist, ik);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_mdc_config)(hid_t plist_id,
                                        H5AC_cache_config_t *config_ptr) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_mdc_config);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
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

    fprintf(__recorderfh, "%.5f H5Pset_mdc_config "
                          "(%d,[%d;%d;%d;%d;%s;%d;%d;%d;%f;%d;%d;%ld;%d;%f;%f;%"
                          "d;%d;%d;%f;%f;%d;%f;%f;%d;%d;%d;%d;%f;%d;%d]);",
            tm1, plist_id, config_ptr->version, config_ptr->rpt_fcn_enabled,
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
  }
#endif

  ret = __real_H5Pset_mdc_config(plist_id, config_ptr);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Pset_meta_block_size)(hid_t fapl_id, hsize_t size) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Pset_meta_block_size);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Pset_meta_block_size (%d,%d)", tm1, fapl_id,
            size);
  }
#endif

  ret = __real_H5Pset_meta_block_size(fapl_id, size);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

htri_t RECORDER_DECL(H5Lexists)(hid_t loc_id, const char *name, hid_t lapl_id) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Lexists);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Lexists (%d,%s,%d)", tm1, loc_id, new_name,
            lapl_id);
    free(new_name);
  }
#endif

  ret = __real_H5Lexists(loc_id, name, lapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Lget_val)(hid_t link_loc_id, const char *link_name,
                                 void *linkval_buff, size_t size,
                                 hid_t lapl_id) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Lget_val);
#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Lget_val (%d,%s,%d,%d,%d)", tm1, link_loc_id,
            link_name, linkval_buff, size, lapl_id);
  }
#endif

  ret = __real_H5Lget_val(link_loc_id, link_name, linkval_buff, size, lapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Literate)(hid_t group_id, H5_index_t index_type,
                                 H5_iter_order_t order, hsize_t *idx,
                                 H5L_iterate_t op, void *op_data) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Literate);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Literate (%d,%d,%d,%d,%d,%d)", tm1, group_id,
            index_type, order, idx, op, op_data);
  }
#endif

  ret = __real_H5Literate(group_id, index_type, order, idx, op, op_data);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Oclose)(hid_t object_id) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Oclose);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Oclose (%d)", tm1, object_id);
  }
#endif

  ret = __real_H5Oclose(object_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

herr_t RECORDER_DECL(H5Oget_info)(hid_t object_id, H5O_info_t *object_info) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Oget_info);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Oget_info (%d,%d)", tm1, object_id,
            object_info);
  }
#endif

  ret = __real_H5Oget_info(object_id, object_info);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif
  return (ret);
}

herr_t RECORDER_DECL(H5Oget_info_by_name)(hid_t loc_id, const char *object_name,
                                          H5O_info_t *object_info,
                                          hid_t lapl_id) {
  herr_t ret;
  double tm1, tm2;
  MAP_OR_FAIL(H5Oget_info_by_name);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, "%.5f H5Oget_info_by_name (%d,%s,%d,%d)", tm1, loc_id,
            object_name, object_info, lapl_id);
  }
#endif

  ret = __real_H5Oget_info_by_name(loc_id, object_name, object_info, lapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}

hid_t RECORDER_DECL(H5Oopen)(hid_t loc_id, const char *name, hid_t lapl_id) {
  herr_t ret;
  double tm1, tm2;

  MAP_OR_FAIL(H5Oopen);

#ifndef DISABLE_HDF5_TRACE
  depth++;
  tm1 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    char *new_name = (char *)malloc(sizeof(char) * strlen(name));
    change_char(name, ' ', new_name, '_');
    fprintf(__recorderfh, "%.5f H5Oopen (%d,%s,%d)", tm1, loc_id, new_name,
            lapl_id);
    free(new_name);
  }
#endif

  ret = __real_H5Oopen(loc_id, name, lapl_id);

#ifndef DISABLE_HDF5_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1) {
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  }
  depth--;
#endif

  return (ret);
}
