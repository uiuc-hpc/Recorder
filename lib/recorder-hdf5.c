/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

static inline char *comm2name(MPI_Comm comm) {
    char *tmp = malloc(128);
    int len;
    PMPI_Comm_get_name(comm, tmp, &len);
    tmp[len] = 0;
    if(len == 0) strcpy(tmp, "MPI_COMM_UNKNOWN");
    return tmp;
}

hid_t RECORDER_HDF5_DECL(H5Fcreate)(const char *filename, unsigned flags, hid_t create_plist, hid_t access_plist) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fcreate, (filename, flags, create_plist, access_plist));
    char **args = assemble_args_list(4, realrealpath(filename), itoa(flags), itoa(create_plist), itoa(access_plist));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t RECORDER_HDF5_DECL(H5Fopen)(const char *filename, unsigned flags, hid_t access_plist) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fopen, (filename, flags, access_plist));
    char **args = assemble_args_list(3, realrealpath(filename), itoa(flags), itoa(access_plist));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Fclose)(hid_t file_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fclose, (file_id));
    char **args = assemble_args_list(1, itoa(file_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t RECORDER_HDF5_DECL(H5Fflush)(hid_t object_id, H5F_scope_t scope) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fflush, (object_id, scope));
    char **args = assemble_args_list(2, itoa(object_id), itoa(scope));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}



// Group Interface
herr_t RECORDER_HDF5_DECL(H5Gclose)(hid_t group_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gclose, (group_id));
    char **args = assemble_args_list(1, itoa(group_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Gcreate1)(hid_t loc_id, const char *name, size_t size_hint) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gcreate1, (loc_id, name, size_hint));
    char **args = assemble_args_list(3, itoa(loc_id), strdup(name), itoa(size_hint));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t RECORDER_HDF5_DECL(H5Gcreate2)(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gcreate2, (loc_id, name, lcpl_id, gcpl_id, gapl_id));
    char **args = assemble_args_list(5, itoa(loc_id), strdup(name), itoa(lcpl_id), itoa(gcpl_id), itoa(gapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t RECORDER_HDF5_DECL(H5Gget_objinfo)(hid_t loc_id, const char *name, hbool_t follow_link, H5G_stat_t *statbuf) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_objinfo, (loc_id, name, follow_link, statbuf));
    char **args = assemble_args_list(4, itoa(loc_id), strdup(name), itoa(follow_link), ptoa(statbuf));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_HDF5_DECL(H5Giterate)(hid_t loc_id, const char *name, int *idx, H5G_iterate_t operator, void *operator_data) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, H5Giterate, (loc_id, name, idx, operator, operator_data));
    char **args = assemble_args_list(4, itoa(loc_id), strdup(name), ptoa(&operator), ptoa(operator_data));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t RECORDER_HDF5_DECL(H5Gopen1)(hid_t loc_id, const char *name) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gopen1, (loc_id, name));
    char **args = assemble_args_list(2, itoa(loc_id), strdup(name));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}


hid_t RECORDER_HDF5_DECL(H5Gopen2)(hid_t loc_id, const char *name, hid_t gapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gopen2, (loc_id, name, gapl_id));
    char **args = assemble_args_list(3, itoa(loc_id), strdup(name), itoa(gapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

// Dataset interface
herr_t RECORDER_HDF5_DECL(H5Dclose)(hid_t dataset_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dclose, (dataset_id));
    char **args = assemble_args_list(1, itoa(dataset_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Dcreate1)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dcreate1, (loc_id, name, type_id, space_id, dcpl_id));
    char **args = assemble_args_list(5, itoa(loc_id), strdup(name), itoa(type_id), itoa(space_id), itoa(dcpl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t RECORDER_HDF5_DECL(H5Dcreate2)(hid_t loc_id, const char *name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dcreate2, (loc_id, name, dtype_id, space_id, lcpl_id, dcpl_id, dapl_id));
    char **args = assemble_args_list(7, itoa(loc_id), strdup(name), itoa(dtype_id), itoa(space_id), itoa(lcpl_id), itoa(dcpl_id), itoa(dapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t RECORDER_HDF5_DECL(H5Dget_create_plist)(hid_t dataset_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_create_plist, (dataset_id));
    char **args = assemble_args_list(1, itoa(dataset_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Dget_space)(hid_t dataset_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_space, (dataset_id));
    char **args = assemble_args_list(1, itoa(dataset_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args)
}

hid_t RECORDER_HDF5_DECL(H5Dget_type)(hid_t dataset_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_type, (dataset_id));
    char **args = assemble_args_list(1, itoa(dataset_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Dopen1)(hid_t loc_id, const char *name) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dopen1, (loc_id, name));
    char **args = assemble_args_list(2, itoa(loc_id), strdup(name));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t RECORDER_HDF5_DECL(H5Dopen2)(hid_t loc_id, const char *name, hid_t dapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dopen2, (loc_id, name, dapl_id));
    char **args = assemble_args_list(3, itoa(loc_id), strdup(name), itoa(dapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Dread)(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, void *buf) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dread, (dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf));
    char **args = assemble_args_list(6, itoa(dataset_id), itoa(mem_type_id), itoa(mem_space_id), itoa(file_space_id), itoa(xfer_plist_id), ptoa(buf));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t RECORDER_HDF5_DECL(H5Dwrite)(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void *buf) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dwrite, (dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf));
    char **args = assemble_args_list(6, itoa(dataset_id), itoa(mem_type_id), itoa(mem_space_id), itoa(file_space_id), itoa(xfer_plist_id), ptoa(buf));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t RECORDER_HDF5_DECL(H5Dset_extent)(hid_t dset_id, const hsize_t size[]) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dset_extent, (dset_id, size));
    char **args = assemble_args_list(2, itoa(dset_id), ptoa(size));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}


herr_t RECORDER_HDF5_DECL(H5Sclose)(hid_t space_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sclose, (space_id));
    char **args = assemble_args_list(1, itoa(space_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Screate)(H5S_class_t type) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Screate, (type));
    char **args = assemble_args_list(1, itoa(type));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Screate_simple)(int rank, const hsize_t *current_dims, const hsize_t *maximum_dims) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Screate_simple, (rank, current_dims, maximum_dims));
    char **args = assemble_args_list(3, itoa(rank), ptoa(current_dims), ptoa(maximum_dims));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hssize_t RECORDER_HDF5_DECL(H5Sget_select_npoints)(hid_t space_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Sget_select_npoints, (space_id));
    char **args = assemble_args_list(1, itoa(space_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_HDF5_DECL(H5Sget_simple_extent_dims)(hid_t space_id, hsize_t *dims, hsize_t *maxdims) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, H5Sget_simple_extent_dims, (space_id, dims, maxdims));
    char **args = assemble_args_list(3, itoa(space_id), ptoa(dims), ptoa(maxdims));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hssize_t RECORDER_HDF5_DECL(H5Sget_simple_extent_npoints)(hid_t space_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Sget_simple_extent_npoints, (space_id));
    char **args = assemble_args_list(1, itoa(space_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t RECORDER_HDF5_DECL(H5Sselect_elements)(hid_t space_id, H5S_seloper_t op, size_t num_elements, const hsize_t *coord) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_elements, (space_id, op, num_elements, coord));
    char **args = assemble_args_list(4, itoa(space_id), itoa(op), itoa(num_elements), ptoa(coord));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t RECORDER_HDF5_DECL(H5Sselect_hyperslab)(hid_t space_id, H5S_seloper_t op, const hsize_t *start, const hsize_t *stride, const hsize_t *count, const hsize_t *block) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_hyperslab, (space_id, op, start, stride, count, block));
    char **args = assemble_args_list(6, itoa(space_id), itoa(op), ptoa(start), ptoa(stride), ptoa(count), ptoa(block));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t RECORDER_HDF5_DECL(H5Sselect_none)(hid_t space_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_none, (space_id));
    char **args = assemble_args_list(1, itoa(space_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t RECORDER_HDF5_DECL(H5Tclose)(hid_t dtype_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tclose, (dtype_id));
    char **args = assemble_args_list(1, itoa(dtype_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Tcopy)(hid_t dtype_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tcopy, (dtype_id));
    char **args = assemble_args_list(1, itoa(dtype_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args)
}

H5T_class_t RECORDER_HDF5_DECL(H5Tget_class)(hid_t dtype_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(H5T_class_t, H5Tget_class, (dtype_id));
    char **args = assemble_args_list(1, itoa(dtype_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

size_t RECORDER_HDF5_DECL(H5Tget_size)(hid_t dtype_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(size_t, H5Tget_size, (dtype_id));
    char **args = assemble_args_list(1, itoa(dtype_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t RECORDER_HDF5_DECL(H5Tset_size)(hid_t dtype_id, size_t size) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tset_size, (dtype_id, size));
    char **args = assemble_args_list(2, itoa(dtype_id), itoa(size));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t RECORDER_HDF5_DECL(H5Tcreate)(H5T_class_t class, size_t size) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tcreate, (class, size));
    char **args = assemble_args_list(2, itoa(class), itoa(size));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Tinsert)(hid_t dtype_id, const char *name, size_t offset, hid_t field_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tinsert, (dtype_id, name, offset, field_id));
    char **args = assemble_args_list(4, itoa(dtype_id), strdup(name), itoa(offset), itoa(field_id));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t RECORDER_HDF5_DECL(H5Aclose)(hid_t attr_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aclose, (attr_id));
    char **args = assemble_args_list(1, itoa(attr_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Acreate1)(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate1, (loc_id, attr_name, type_id, space_id, acpl_id));
    char **args = assemble_args_list(5, itoa(loc_id), strdup(attr_name), itoa(type_id), itoa(space_id), itoa(acpl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t RECORDER_HDF5_DECL(H5Acreate2)(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate2, (loc_id, attr_name, type_id, space_id, acpl_id, aapl_id));
    char **args = assemble_args_list(6, itoa(loc_id), strdup(attr_name), itoa(type_id), itoa(space_id), itoa(acpl_id), itoa(aapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

ssize_t RECORDER_HDF5_DECL(H5Aget_name)(hid_t attr_id, size_t buf_size, char *buf) {
    RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Aget_name, (attr_id, buf_size, buf));
    char **args = assemble_args_list(3, itoa(attr_id), itoa(buf_size), ptoa(buf));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int RECORDER_HDF5_DECL(H5Aget_num_attrs)(hid_t loc_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, H5Aget_num_attrs, (loc_id));
    char **args = assemble_args_list(1, itoa(loc_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Aget_space)(hid_t attr_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aget_space, (attr_id));
    char **args = assemble_args_list(1, itoa(attr_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Aget_type)(hid_t attr_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aget_type, (attr_id));
    char **args = assemble_args_list(1, itoa(attr_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Aopen)(hid_t obj_id, const char *attr_name, hid_t aapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen, (obj_id, attr_name, aapl_id));
    char **args = assemble_args_list(3, itoa(obj_id), strdup(attr_name), itoa(aapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t RECORDER_HDF5_DECL(H5Aopen_idx)(hid_t loc_id, unsigned int idx) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_idx, (loc_id,idx));
    char **args = assemble_args_list(2, itoa(loc_id), itoa(idx));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t RECORDER_HDF5_DECL(H5Aopen_name)(hid_t loc_id, const char *name) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_name, (loc_id, name));
    char **args = assemble_args_list(2, itoa(loc_id), strdup(name));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Aread)(hid_t attr_id, hid_t mem_type_id, void *buf) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aread, (attr_id, mem_type_id, buf));
    char **args = assemble_args_list(3, itoa(attr_id), itoa(mem_type_id), ptoa(buf));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Awrite)(hid_t attr_id, hid_t mem_type_id, const void *buf) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Awrite, (attr_id, mem_type_id, buf));
    char **args = assemble_args_list(3, itoa(attr_id), itoa(mem_type_id), ptoa(buf));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pclose)(hid_t plist) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pclose, (plist));
    char **args = assemble_args_list(1, itoa(plist));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t RECORDER_HDF5_DECL(H5Pcreate)(hid_t cls_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pcreate, (cls_id));
    char **args = assemble_args_list(1, itoa(cls_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_HDF5_DECL(H5Pget_chunk)(hid_t plist, int max_ndims, hsize_t *dims) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, H5Pget_chunk, (plist, max_ndims, dims));
    char **args = assemble_args_list(3, itoa(plist), itoa(max_ndims), ptoa(dims));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pget_mdc_config)(hid_t plist_id, H5AC_cache_config_t *config_ptr) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mdc_config, (plist_id, config_ptr));
    char **args = assemble_args_list(2, itoa(plist_id), ptoa(config_ptr));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_alignment)(hid_t plist, hsize_t threshold, hsize_t alignment) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_alignment, (plist, threshold, alignment));
    char **args = assemble_args_list(3, itoa(plist), itoa(threshold), itoa(alignment));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_chunk)(hid_t plist, int ndims, const hsize_t *dim) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_chunk, (plist, ndims, dim));
    char **args = assemble_args_list(3, itoa(plist), itoa(ndims), ptoa(dim));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_dxpl_mpio)(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_dxpl_mpio, (dxpl_id, xfer_mode));
    char **args = assemble_args_list(2, itoa(dxpl_id), itoa(xfer_mode));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_fapl_core)(hid_t fapl_id, size_t increment, hbool_t backing_store) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_core, (fapl_id, increment, backing_store));
    char **args = assemble_args_list(3, itoa(fapl_id), itoa(increment), itoa(backing_store));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_fapl_mpio)(hid_t fapl_id, MPI_Comm comm, MPI_Info info) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_mpio, (fapl_id, comm, info));
    char **args = assemble_args_list(3, itoa(fapl_id), comm2name(comm), ptoa(&info));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_fapl_mpiposix)(hid_t fapl_id, MPI_Comm comm, hbool_t use_gpfs_hints) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_mpiposix, (fapl_id, comm, use_gpfs_hints));
    char **args = assemble_args_list(3, itoa(fapl_id), comm2name(comm), itoa(use_gpfs_hints));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_istore_k)(hid_t plist, unsigned ik) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_istore_k, (plist, ik));
    char **args = assemble_args_list(2, itoa(plist), itoa(ik));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_mdc_config)(hid_t plist_id, H5AC_cache_config_t *config_ptr) {
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
    /*
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
    */
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_mdc_config, (plist_id, config_ptr));
    char **args = assemble_args_list(2, itoa(plist_id), ptoa(config_ptr));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_meta_block_size)(hid_t fapl_id, hsize_t size) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_meta_block_size, (fapl_id, size));
    char **args = assemble_args_list(2, itoa(fapl_id), itoa(size));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t RECORDER_HDF5_DECL(H5Lexists)(hid_t loc_id, const char *name, hid_t lapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Lexists, (loc_id, name, lapl_id));
    char **args = assemble_args_list(3, itoa(loc_id), strdup(name), itoa(lapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t RECORDER_HDF5_DECL(H5Lget_val)(hid_t link_loc_id, const char *link_name, void *linkval_buff, size_t size, hid_t lapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_val, (link_loc_id, link_name, linkval_buff, size, lapl_id));
    char **args = assemble_args_list(5, itoa(link_loc_id), strdup(link_name), ptoa(linkval_buff), itoa(size), itoa(lapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t RECORDER_HDF5_DECL(H5Literate)(hid_t group_id, H5_index_t index_type, H5_iter_order_t order, hsize_t *idx, H5L_iterate_t op, void *op_data) {
    RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Literate, (group_id, index_type, order, idx, op, op_data));
    char **args = assemble_args_list(6, itoa(group_id), itoa(index_type), itoa(order), ptoa(idx), ptoa(op), ptoa(op_data));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t RECORDER_HDF5_DECL(H5Oclose)(hid_t object_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oclose, (object_id));
    char **args = assemble_args_list(1, itoa(object_id));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

/*
 * Exists in 1.8 not in 1.10, in 1.10 they are H5Oget_info1; H5Oget_info2
herr_t RECORDER_HDF5_DECL(H5Oget_info)(hid_t object_id, H5O_info_t *object_info) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Oget_info (%d, %p)", object_id, object_info);
    RECORDER_INTERCEPTOR_EPILOGUE(H5Oget_info, herr_t, (object_id, object_info), 0, 0, log_text);

}

herr_t RECORDER_HDF5_DECL(H5Oget_info_by_name)(hid_t loc_id, const char *object_name, H5O_info_t *object_info, hid_t lapl_id) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "H5Oget_info_by_name (%d, %s, %p, %d)", loc_id, object_name, object_info, lapl_id);
    RECORDER_INTERCEPTOR_EPILOGUE(H5Oget_info_by_name, herr_t, (loc_id, object_name, object_info, lapl_id), 0, 0, log_text);

}
*/

hid_t RECORDER_HDF5_DECL(H5Oopen)(hid_t loc_id, const char *name, hid_t lapl_id) {
    RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen, (loc_id, name, lapl_id));
    char **args = assemble_args_list(3, itoa(loc_id), strdup(name), itoa(lapl_id));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}


herr_t RECORDER_HDF5_DECL(H5Pset_coll_metadata_write)(hid_t fapl_id, hbool_t is_collective) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_coll_metadata_write, (fapl_id, is_collective));
    char **args = assemble_args_list(2, itoa(fapl_id), itoa(is_collective));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args)
}

herr_t RECORDER_HDF5_DECL(H5Pget_coll_metadata_write)(hid_t fapl_id, hbool_t* is_collective) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_coll_metadata_write, (fapl_id, is_collective));
    char **args = assemble_args_list(2, itoa(fapl_id), ptoa(is_collective));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Pset_all_coll_metadata_ops)(hid_t fapl_id, hbool_t is_collective) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_all_coll_metadata_ops, (fapl_id, is_collective));
    char **args = assemble_args_list(2, itoa(fapl_id), itoa(is_collective));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t RECORDER_HDF5_DECL(H5Pget_all_coll_metadata_ops)(hid_t fapl_id, hbool_t* is_collective) {
    RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_all_coll_metadata_ops, (fapl_id, is_collective));
    char **args = assemble_args_list(2, itoa(fapl_id), ptoa(is_collective));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

