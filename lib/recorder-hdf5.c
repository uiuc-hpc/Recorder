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
static int count = 0;

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

herr_t WRAPPER_NAME(H5Dread)(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dread, (dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf));
	char **args = assemble_args_list(6, itoa(dset_id),itoa(mem_type_id),itoa(mem_space_id),itoa(file_space_id),itoa(dxpl_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Adelete_by_idx)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Adelete_by_idx, (loc_id, obj_name, idx_type, order, n, lapl_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),itoa(n),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Fflush)(hid_t object_id, H5F_scope_t scope) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fflush, (object_id, scope));
	char **args = assemble_args_list(2, itoa(object_id),itoa(scope));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int WRAPPER_NAME(H5Pget_chunk)(hid_t plist_id, int max_ndims, hsize_t dim[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Pget_chunk, (plist_id, max_ndims, dim));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(max_ndims),ptoa(dim));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_buffer)(hid_t plist_id, size_t size, void *tconv, void *bkg) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_buffer, (plist_id, size, tconv, bkg));
	char **args = assemble_args_list(4, itoa(plist_id),itoa(size),ptoa(tconv),ptoa(bkg));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute)(hid_t loc_id, const char *obj_name, const char *attr_name, hid_t mem_type_id, void *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute, (loc_id, obj_name, attr_name, mem_type_id, data));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa(mem_type_id),ptoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_all_coll_metadata_ops)(hid_t plist_id, hbool_t is_collective) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_all_coll_metadata_ops, (plist_id, is_collective));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(is_collective));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pmodify_filter)(hid_t plist_id, H5Z_filter_t filter, unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pmodify_filter, (plist_id, filter, flags, cd_nelmts, cd_values));
	char **args = assemble_args_list(5, itoa(plist_id),ptoa(&filter),itoa(flags),itoa(cd_nelmts),ptoa(cd_values));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Fmount)(hid_t loc, const char *name, hid_t child, hid_t plist) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fmount, (loc, name, child, plist));
	char **args = assemble_args_list(4, itoa(loc),strtoa(name),itoa(child),itoa(plist));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

H5G_obj_t WRAPPER_NAME(H5Rget_obj_type1)(hid_t id, H5R_type_t ref_type, const void *ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5G_obj_t, H5Rget_obj_type1, (id, ref_type, ref));
	char **args = assemble_args_list(3, itoa(id),ptoa(&ref_type),ptoa(ref));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5IMmake_palette)(hid_t loc_id, const char *pal_name, const hsize_t *pal_dims, const unsigned char *pal_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMmake_palette, (loc_id, pal_name, pal_dims, pal_data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(pal_name),itoa((pal_dims==NULL)?-1:*pal_dims),strtoa(pal_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

ssize_t WRAPPER_NAME(H5Pget_driver_config_str)(hid_t fapl_id, char *config_buf, size_t buf_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Pget_driver_config_str, (fapl_id, config_buf, buf_size));
	char **args = assemble_args_list(3, itoa(fapl_id),strtoa(config_buf),itoa(buf_size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Ewalk2)(hid_t err_stack, H5E_direction_t direction, H5E_walk2_t func, void *client_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ewalk2, (err_stack, direction, func, client_data));
	char **args = assemble_args_list(4, itoa(err_stack),ptoa(&direction),ptoa(&func),ptoa(client_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Oget_info3)(hid_t loc_id, H5O_info2_t *oinfo, unsigned fields) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info3, (loc_id, oinfo, fields));
	char **args = assemble_args_list(3, itoa(loc_id),ptoa(oinfo),itoa(fields));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Eclose_stack)(hid_t stack_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eclose_stack, (stack_id));
	char **args = assemble_args_list(1, itoa(stack_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5PTfree_vlen_buff)(hid_t table_id, size_t bufflen, void *buff) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTfree_vlen_buff, (table_id, bufflen, buff));
	char **args = assemble_args_list(3, itoa(table_id),itoa(bufflen),ptoa(buff));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Ecreate_stack)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ecreate_stack, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

hid_t WRAPPER_NAME(H5VLget_connector_id_by_name)(const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5VLget_connector_id_by_name, (name));
	char **args = assemble_args_list(1, strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5TBinsert_record)(hid_t loc_id, const char *dset_name, hsize_t start, hsize_t nrecords, size_t dst_size, const size_t *dst_offset, const size_t *dst_sizes, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBinsert_record, (loc_id, dset_name, start, nrecords, dst_size, dst_offset, dst_sizes, buf));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(dset_name),itoa(start),itoa(nrecords),itoa(dst_size),itoa((dst_offset==NULL)?-1:*dst_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Otoken_cmp)(hid_t loc_id, const H5O_token_t *token1, const H5O_token_t *token2, int *cmp_value) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Otoken_cmp, (loc_id, token1, token2, cmp_value));
	char **args = assemble_args_list(4, itoa(loc_id),ptoa(token1),ptoa(token2),itoa((cmp_value==NULL)?-1:*cmp_value));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5PLremove)(unsigned int index) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLremove, (index));
	char **args = assemble_args_list(1, itoa(index));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_hyper_vector_size)(hid_t plist_id, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_hyper_vector_size, (plist_id, size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hssize_t WRAPPER_NAME(H5Sget_select_hyper_nblocks)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Sget_select_hyper_nblocks, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Aget_info_by_idx)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5A_info_t *ainfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aget_info_by_idx, (loc_id, obj_name, idx_type, order, n, ainfo, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),itoa(n),ptoa(ainfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Adelete_by_name)(hid_t loc_id, const char *obj_name, const char *attr_name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Adelete_by_name, (loc_id, obj_name, attr_name, lapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(H5Iinc_ref)(hid_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Iinc_ref, (id));
	char **args = assemble_args_list(1, itoa(id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Ovisit_by_name1)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate1_t op, void *op_data, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ovisit_by_name1, (loc_id, obj_name, idx_type, order, op, op_data, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),ptoa(&op),ptoa(op_data),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5PLreplace)(const char *search_path, unsigned int index) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLreplace, (search_path, index));
	char **args = assemble_args_list(2, strtoa(search_path),itoa(index));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Dwrite_chunk)(hid_t dset_id, hid_t dxpl_id, uint32_t filters, const hsize_t *offset, size_t data_size, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dwrite_chunk, (dset_id, dxpl_id, filters, offset, data_size, buf));
	char **args = assemble_args_list(6, itoa(dset_id),itoa(dxpl_id),itoa(filters),itoa((offset==NULL)?-1:*offset),itoa(data_size),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Sselect_copy)(hid_t dst_id, hid_t src_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_copy, (dst_id, src_id));
	char **args = assemble_args_list(2, itoa(dst_id),itoa(src_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_dset_no_attrs_hint)(hid_t dcpl_id, hbool_t minimize) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_dset_no_attrs_hint, (dcpl_id, minimize));
	char **args = assemble_args_list(2, itoa(dcpl_id),itoa(minimize));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Ssel_iter_create)(hid_t spaceid, size_t elmt_size, unsigned flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ssel_iter_create, (spaceid, elmt_size, flags));
	char **args = assemble_args_list(3, itoa(spaceid),itoa(elmt_size),itoa(flags));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_name3)(hid_t loc_id, const char *name, H5O_info2_t *oinfo, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_name3, (loc_id, name, oinfo, fields, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),ptoa(oinfo),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_obj_track_times)(hid_t plist_id, hbool_t *track_times) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_obj_track_times, (plist_id, track_times));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((track_times==NULL)?-1:*track_times));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Sselect_none)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_none, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Lcopy)(hid_t src_loc, const char *src_name, hid_t dst_loc, const char *dst_name, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcopy, (src_loc, src_name, dst_loc, dst_name, lcpl_id, lapl_id));
	char **args = assemble_args_list(6, itoa(src_loc),strtoa(src_name),itoa(dst_loc),strtoa(dst_name),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Fset_libver_bounds)(hid_t file_id, H5F_libver_t low, H5F_libver_t high) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fset_libver_bounds, (file_id, low, high));
	char **args = assemble_args_list(3, itoa(file_id),ptoa(&low),ptoa(&high));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Dwrite_multi)(size_t count, hid_t dset_id[], hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[], hid_t dxpl_id, const void *buf[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dwrite_multi, (count, dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf));
	char **args = assemble_args_list(7, itoa(count),ptoa(dset_id),ptoa(mem_type_id),ptoa(mem_space_id),ptoa(file_space_id),itoa(dxpl_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Fset_latest_format)(hid_t file_id, hbool_t latest_format) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fset_latest_format, (file_id, latest_format));
	char **args = assemble_args_list(2, itoa(file_id),itoa(latest_format));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eset_current_stack)(hid_t err_stack_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eset_current_stack, (err_stack_id));
	char **args = assemble_args_list(1, itoa(err_stack_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hssize_t WRAPPER_NAME(H5Sget_select_elem_npoints)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Sget_select_elem_npoints, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_type_conv_cb)(hid_t dxpl_id, H5T_conv_except_func_t op, void *operate_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_type_conv_cb, (dxpl_id, op, operate_data));
	char **args = assemble_args_list(3, itoa(dxpl_id),ptoa(&op),ptoa(operate_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_log)(hid_t fapl_id, const char *logfile, unsigned long long flags, size_t buf_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_log, (fapl_id, logfile, flags, buf_size));
	char **args = assemble_args_list(4, itoa(fapl_id),strtoa(logfile),itoa(flags),itoa(buf_size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Orefresh)(hid_t oid) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Orefresh, (oid));
	char **args = assemble_args_list(1, itoa(oid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Acreate2)(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate2, (loc_id, attr_name, type_id, space_id, acpl_id, aapl_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(attr_name),itoa(type_id),itoa(space_id),itoa(acpl_id),itoa(aapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Dextend)(hid_t dset_id, const hsize_t size[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dextend, (dset_id, size));
	char **args = assemble_args_list(2, itoa(dset_id),ptoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBread_table)(hid_t loc_id, const char *dset_name, size_t dst_size, const size_t *dst_offset, const size_t *dst_sizes, void *dst_buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBread_table, (loc_id, dset_name, dst_size, dst_offset, dst_sizes, dst_buf));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(dset_name),itoa(dst_size),itoa((dst_offset==NULL)?-1:*dst_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(dst_buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

htri_t WRAPPER_NAME(H5Sis_regular_hyperslab)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sis_regular_hyperslab, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_modify_write_buf)(hid_t plist_id, hbool_t modify_write_buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_modify_write_buf, (plist_id, modify_write_buf));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(modify_write_buf));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Scopy)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Scopy, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Sdecode)(const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Sdecode, (buf));
	char **args = assemble_args_list(1, ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_string)(hid_t loc_id, const char *obj_name, const char *attr_name, const char *attr_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_string, (loc_id, obj_name, attr_name, attr_data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),strtoa(attr_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_sieve_buf_size)(hid_t fapl_id, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_sieve_buf_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Pget_class_parent)(hid_t pclass_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pget_class_parent, (pclass_id));
	char **args = assemble_args_list(1, itoa(pclass_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

htri_t WRAPPER_NAME(H5Sselect_shape_same)(hid_t space1_id, hid_t space2_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sselect_shape_same, (space1_id, space2_id));
	char **args = assemble_args_list(2, itoa(space1_id),itoa(space2_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBwrite_records)(hid_t loc_id, const char *dset_name, hsize_t start, hsize_t nrecords, size_t type_size, const size_t *field_offset, const size_t *dst_sizes, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBwrite_records, (loc_id, dset_name, start, nrecords, type_size, field_offset, dst_sizes, buf));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(dset_name),itoa(start),itoa(nrecords),itoa(type_size),itoa((field_offset==NULL)?-1:*field_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Sget_select_elem_pointlist)(hid_t spaceid, hsize_t startpoint, hsize_t numpoints, hsize_t buf[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sget_select_elem_pointlist, (spaceid, startpoint, numpoints, buf));
	char **args = assemble_args_list(4, itoa(spaceid),itoa(startpoint),itoa(numpoints),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTread_bitfield_value)(hid_t dset_id, int num_values, const unsigned *offset, const unsigned *lengths, hid_t space, int *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_bitfield_value, (dset_id, num_values, offset, lengths, space, buf));
	char **args = assemble_args_list(6, itoa(dset_id),itoa(num_values),itoa((offset==NULL)?-1:*offset),itoa((lengths==NULL)?-1:*lengths),itoa(space),itoa((buf==NULL)?-1:*buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5IMis_palette)(hid_t loc_id, const char *dset_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMis_palette, (loc_id, dset_name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(dset_name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Freopen)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Freopen, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Punregister)(hid_t pclass_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Punregister, (pclass_id, name));
	char **args = assemble_args_list(2, itoa(pclass_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_szip)(hid_t plist_id, unsigned options_mask, unsigned pixels_per_block) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_szip, (plist_id, options_mask, pixels_per_block));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(options_mask),itoa(pixels_per_block));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Fget_create_plist)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fget_create_plist, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

H5T_class_t WRAPPER_NAME(H5Tget_class)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5T_class_t, H5Tget_class, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5EScancel)(hid_t es_id, size_t *num_not_canceled, hbool_t *err_occurred) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5EScancel, (es_id, num_not_canceled, err_occurred));
	char **args = assemble_args_list(3, itoa(es_id),itoa((num_not_canceled==NULL)?-1:*num_not_canceled),itoa((err_occurred==NULL)?-1:*err_occurred));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Tcommit1)(hid_t loc_id, const char *name, hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tcommit1, (loc_id, name, type_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Sselect_adjust)(hid_t spaceid, const hssize_t *offset) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_adjust, (spaceid, offset));
	char **args = assemble_args_list(2, itoa(spaceid),itoa((offset==NULL)?-1:*offset));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_filter_callback)(hid_t plist_id, H5Z_filter_func_t func, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_filter_callback, (plist_id, func, op_data));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(&func),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Tdecode)(const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tdecode, (buf));
	char **args = assemble_args_list(1, ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Rget_obj_type2)(hid_t id, H5R_type_t ref_type, const void *ref, H5O_type_t *obj_type) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rget_obj_type2, (id, ref_type, ref, obj_type));
	char **args = assemble_args_list(4, itoa(id),ptoa(&ref_type),ptoa(ref),ptoa(obj_type));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Pcopy)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pcopy, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Dget_num_chunks)(hid_t dset_id, hid_t fspace_id, hsize_t *nchunks) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dget_num_chunks, (dset_id, fspace_id, nchunks));
	char **args = assemble_args_list(3, itoa(dset_id),itoa(fspace_id),itoa((nchunks==NULL)?-1:*nchunks));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_coll_metadata_write)(hid_t plist_id, hbool_t *is_collective) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_coll_metadata_write, (plist_id, is_collective));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((is_collective==NULL)?-1:*is_collective));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5IMmake_image_24bit)(hid_t loc_id, const char *dset_name, hsize_t width, hsize_t height, const char *interlace, const unsigned char *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMmake_image_24bit, (loc_id, dset_name, width, height, interlace, buffer));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(dset_name),itoa(width),itoa(height),strtoa(interlace),strtoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5set_free_list_limits)(int reg_global_lim, int reg_list_lim, int arr_global_lim, int arr_list_lim, int blk_global_lim, int blk_list_lim) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5set_free_list_limits, (reg_global_lim, reg_list_lim, arr_global_lim, arr_list_lim, blk_global_lim, blk_list_lim));
	char **args = assemble_args_list(6, itoa(reg_global_lim),itoa(reg_list_lim),itoa(arr_global_lim),itoa(arr_list_lim),itoa(blk_global_lim),itoa(blk_list_lim));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

ssize_t WRAPPER_NAME(H5Iget_name)(hid_t id, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Iget_name, (id, name, size));
	char **args = assemble_args_list(3, itoa(id),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Scombine_hyperslab)(hid_t space_id, H5S_seloper_t op, const hsize_t start[], const hsize_t stride[], const hsize_t count[], const hsize_t block[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Scombine_hyperslab, (space_id, op, start, stride, count, block));
	char **args = assemble_args_list(6, itoa(space_id),ptoa(&op),ptoa(start),ptoa(stride),ptoa(count),ptoa(block));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

ssize_t WRAPPER_NAME(H5Aget_name)(hid_t attr_id, size_t buf_size, char *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Aget_name, (attr_id, buf_size, buf));
	char **args = assemble_args_list(3, itoa(attr_id),itoa(buf_size),strtoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5TBget_table_info)(hid_t loc_id, const char *dset_name, hsize_t *nfields, hsize_t *nrecords) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBget_table_info, (loc_id, dset_name, nfields, nrecords));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(dset_name),itoa((nfields==NULL)?-1:*nfields),itoa((nrecords==NULL)?-1:*nrecords));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Aread)(hid_t attr_id, hid_t type_id, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aread, (attr_id, type_id, buf));
	char **args = assemble_args_list(3, itoa(attr_id),itoa(type_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_deflate)(hid_t plist_id, unsigned level) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_deflate, (plist_id, level));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(level));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_mcdt_search_cb)(hid_t plist_id, H5O_mcdt_search_cb_t *func, void **op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mcdt_search_cb, (plist_id, func, op_data));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(func),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_double)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, const double *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_double, (loc_id, dset_name, rank, dims, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),ftoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5LTtext_to_dtype)(const char *text, H5LT_lang_t lang_type) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5LTtext_to_dtype, (text, lang_type));
	char **args = assemble_args_list(2, strtoa(text),ptoa(&lang_type));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5Eget_num)(hid_t error_stack_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Eget_num, (error_stack_id));
	char **args = assemble_args_list(1, itoa(error_stack_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_fill_time)(hid_t plist_id, H5D_fill_time_t *fill_time) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fill_time, (plist_id, fill_time));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(fill_time));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_double)(hid_t loc_id, const char *obj_name, const char *attr_name, const double *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_double, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),ftoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_cache)(hid_t plist_id, int *mdc_nelmts, size_t *rdcc_nslots, size_t *rdcc_nbytes, double *rdcc_w0) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_cache, (plist_id, mdc_nelmts, rdcc_nslots, rdcc_nbytes, rdcc_w0));
	char **args = assemble_args_list(5, itoa(plist_id),itoa((mdc_nelmts==NULL)?-1:*mdc_nelmts),itoa((rdcc_nslots==NULL)?-1:*rdcc_nslots),itoa((rdcc_nbytes==NULL)?-1:*rdcc_nbytes),ftoa((rdcc_w0==NULL)?-1:*rdcc_w0));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5ESwait)(hid_t es_id, uint64_t timeout, size_t *num_in_progress, hbool_t *err_occurred) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESwait, (es_id, timeout, num_in_progress, err_occurred));
	char **args = assemble_args_list(4, itoa(es_id),itoa(timeout),itoa((num_in_progress==NULL)?-1:*num_in_progress),itoa((err_occurred==NULL)?-1:*err_occurred));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5VLclose)(hid_t connector_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5VLclose, (connector_id));
	char **args = assemble_args_list(1, itoa(connector_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

ssize_t WRAPPER_NAME(H5Eget_class_name)(hid_t class_id, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Eget_class_name, (class_id, name, size));
	char **args = assemble_args_list(3, itoa(class_id),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_dxpl_mpio_chunk_opt_ratio)(hid_t dxpl_id, unsigned percent_num_proc_per_chunk) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_dxpl_mpio_chunk_opt_ratio, (dxpl_id, percent_num_proc_per_chunk));
	char **args = assemble_args_list(2, itoa(dxpl_id),itoa(percent_num_proc_per_chunk));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_int)(hid_t loc_id, const char *obj_name, const char *attr_name, int *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_int, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_obj_track_times)(hid_t plist_id, hbool_t track_times) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_obj_track_times, (plist_id, track_times));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(track_times));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBwrite_fields_name)(hid_t loc_id, const char *dset_name, const char *field_names, hsize_t start, hsize_t nrecords, size_t type_size, const size_t *field_offset, const size_t *dst_sizes, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBwrite_fields_name, (loc_id, dset_name, field_names, start, nrecords, type_size, field_offset, dst_sizes, buf));
	char **args = assemble_args_list(9, itoa(loc_id),strtoa(dset_name),strtoa(field_names),itoa(start),itoa(nrecords),itoa(type_size),itoa((field_offset==NULL)?-1:*field_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

ssize_t WRAPPER_NAME(H5Rget_name)(hid_t loc_id, H5R_type_t ref_type, const void *ref, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Rget_name, (loc_id, ref_type, ref, name, size));
	char **args = assemble_args_list(5, itoa(loc_id),ptoa(&ref_type),ptoa(ref),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_userblock)(hid_t plist_id, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_userblock, (plist_id, size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Gopen1)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gopen1, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Dfill)(const void *fill, hid_t fill_type_id, void *buf, hid_t buf_type_id, hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dfill, (fill, fill_type_id, buf, buf_type_id, space_id));
	char **args = assemble_args_list(5, ptoa(fill),itoa(fill_type_id),ptoa(buf),itoa(buf_type_id),itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_relax_file_integrity_checks)(hid_t plist_id, uint64_t flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_relax_file_integrity_checks, (plist_id, flags));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int WRAPPER_NAME(H5Sget_simple_extent_ndims)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Sget_simple_extent_ndims, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_direct)(hid_t fapl_id, size_t alignment, size_t block_size, size_t cbuf_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_direct, (fapl_id, alignment, block_size, cbuf_size));
	char **args = assemble_args_list(4, itoa(fapl_id),itoa(alignment),itoa(block_size),itoa(cbuf_size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget_fclose_degree)(hid_t fapl_id, H5F_close_degree_t *degree) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fclose_degree, (fapl_id, degree));
	char **args = assemble_args_list(2, itoa(fapl_id),ptoa(degree));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Gmove)(hid_t src_loc_id, const char *src_name, const char *dst_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gmove, (src_loc_id, src_name, dst_name));
	char **args = assemble_args_list(3, itoa(src_loc_id),strtoa(src_name),strtoa(dst_name));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Aopen_idx)(hid_t loc_id, unsigned idx) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_idx, (loc_id, idx));
	char **args = assemble_args_list(2, itoa(loc_id),itoa(idx));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eprint2)(hid_t err_stack, FILE *stream) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eprint2, (err_stack, stream));
	char **args = assemble_args_list(2, itoa(err_stack),ptoa(stream));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Aclose)(hid_t attr_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aclose, (attr_id));
	char **args = assemble_args_list(1, itoa(attr_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

ssize_t WRAPPER_NAME(H5DSget_label)(hid_t did, unsigned int idx, char *label, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5DSget_label, (did, idx, label, size));
	char **args = assemble_args_list(4, itoa(did),itoa(idx),strtoa(label),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget_mpio_actual_io_mode)(hid_t plist_id, H5D_mpio_actual_io_mode_t *actual_io_mode) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mpio_actual_io_mode, (plist_id, actual_io_mode));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(actual_io_mode));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

size_t WRAPPER_NAME(H5LDget_dset_type_size)(hid_t did, const char *fields) {
	RECORDER_INTERCEPTOR_PROLOGUE(size_t, H5LDget_dset_type_size, (did, fields));
	char **args = assemble_args_list(2, itoa(did),strtoa(fields));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_double)(hid_t loc_id, const char *obj_name, const char *attr_name, double *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_double, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),ftoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5IMunlink_palette)(hid_t loc_id, const char *image_name, const char *pal_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMunlink_palette, (loc_id, image_name, pal_name));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(image_name),strtoa(pal_name));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Tcommit_async)(hid_t loc_id, const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tcommit_async, (loc_id, name, type_id, lcpl_id, tcpl_id, tapl_id, es_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(name),itoa(type_id),itoa(lcpl_id),itoa(tcpl_id),itoa(tapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Pset_external)(hid_t plist_id, const char *name, off_t offset, hsize_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_external, (plist_id, name, offset, size));
	char **args = assemble_args_list(4, itoa(plist_id),strtoa(name),ptoa(&offset),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hsize_t WRAPPER_NAME(H5Aget_storage_size)(hid_t attr_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hsize_t, H5Aget_storage_size, (attr_id));
	char **args = assemble_args_list(1, itoa(attr_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_alloc_time)(hid_t plist_id, H5D_alloc_time_t *alloc_time) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_alloc_time, (plist_id, alloc_time));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(alloc_time));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_small_data_block_size)(hid_t fapl_id, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_small_data_block_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t WRAPPER_NAME(H5Zfilter_avail)(H5Z_filter_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Zfilter_avail, (id));
	char **args = assemble_args_list(1, ptoa(&id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

ssize_t WRAPPER_NAME(H5Pget_elink_prefix)(hid_t plist_id, char *prefix, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Pget_elink_prefix, (plist_id, prefix, size));
	char **args = assemble_args_list(3, itoa(plist_id),strtoa(prefix),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

void * WRAPPER_NAME(H5allocate_memory)(size_t size, hbool_t clear) {
	RECORDER_INTERCEPTOR_PROLOGUE(void *, H5allocate_memory, (size, clear));
	char **args = assemble_args_list(2, itoa(size),itoa(clear));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5PLinsert)(const char *search_path, unsigned int index) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLinsert, (search_path, index));
	char **args = assemble_args_list(2, strtoa(search_path),itoa(index));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_no_selection_io_cause)(hid_t plist_id, uint32_t *no_selection_io_cause) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_no_selection_io_cause, (plist_id, no_selection_io_cause));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((no_selection_io_cause==NULL)?-1:*no_selection_io_cause));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_dataset_ndims)(hid_t loc_id, const char *dset_name, int *rank) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_dataset_ndims, (loc_id, dset_name, rank));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),itoa((rank==NULL)?-1:*rank));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Pget_class)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pget_class, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

char * WRAPPER_NAME(H5Eget_minor)(H5E_minor_t min) {
	RECORDER_INTERCEPTOR_PROLOGUE(char *, H5Eget_minor, (min));
	char **args = assemble_args_list(1, ptoa(&min));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int WRAPPER_NAME(H5Pget_preserve)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Pget_preserve, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Ovisit1)(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate1_t op, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ovisit1, (obj_id, idx_type, order, op, op_data));
	char **args = assemble_args_list(5, itoa(obj_id),itoa(idx_type),itoa(order),ptoa(&op),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_gc_references)(hid_t fapl_id, unsigned *gc_ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_gc_references, (fapl_id, gc_ref));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((gc_ref==NULL)?-1:*gc_ref));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Ssel_iter_close)(hid_t sel_iter_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ssel_iter_close, (sel_iter_id));
	char **args = assemble_args_list(1, itoa(sel_iter_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

htri_t WRAPPER_NAME(H5Pall_filters_avail)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Pall_filters_avail, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

htri_t WRAPPER_NAME(H5Pexist)(hid_t plist_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Pexist, (plist_id, name));
	char **args = assemble_args_list(2, itoa(plist_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Fget_vfd_handle)(hid_t file_id, hid_t fapl, void **file_handle) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_vfd_handle, (file_id, fapl, file_handle));
	char **args = assemble_args_list(3, itoa(file_id),itoa(fapl),ptoa(file_handle));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LRcopy_region)(hid_t obj_id, hdset_reg_ref_t *ref, const char *file, const char *path, const hsize_t *block_coord) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRcopy_region, (obj_id, ref, file, path, block_coord));
	char **args = assemble_args_list(5, itoa(obj_id),ptoa(ref),strtoa(file),strtoa(path),itoa((block_coord==NULL)?-1:*block_coord));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Acreate1)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t acpl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate1, (loc_id, name, type_id, space_id, acpl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),itoa(type_id),itoa(space_id),itoa(acpl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

htri_t WRAPPER_NAME(H5Tdetect_class)(hid_t type_id, H5T_class_t cls) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Tdetect_class, (type_id, cls));
	char **args = assemble_args_list(2, itoa(type_id),ptoa(&cls));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eclose_msg)(hid_t err_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eclose_msg, (err_id));
	char **args = assemble_args_list(1, itoa(err_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Olink)(hid_t obj_id, hid_t new_loc_id, const char *new_name, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Olink, (obj_id, new_loc_id, new_name, lcpl_id, lapl_id));
	char **args = assemble_args_list(5, itoa(obj_id),itoa(new_loc_id),strtoa(new_name),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5PTget_next)(hid_t table_id, size_t nrecords, void *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTget_next, (table_id, nrecords, data));
	char **args = assemble_args_list(3, itoa(table_id),itoa(nrecords),ptoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5Lexists)(hid_t loc_id, const char *name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Lexists, (loc_id, name, lapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Fget_info1)(hid_t obj_id, H5F_info1_t *file_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_info1, (obj_id, file_info));
	char **args = assemble_args_list(2, itoa(obj_id),ptoa(file_info));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_windows)(hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_windows, (fapl_id));
	char **args = assemble_args_list(1, itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_mdc_image_config)(hid_t plist_id, H5AC_cache_image_config_t *config_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_mdc_image_config, (plist_id, config_ptr));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(config_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Eregister_class)(const char *cls_name, const char *lib_name, const char *version) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Eregister_class, (cls_name, lib_name, version));
	char **args = assemble_args_list(3, strtoa(cls_name),strtoa(lib_name),strtoa(version));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Dget_chunk_info_by_coord)(hid_t dset_id, const hsize_t *offset, unsigned *filter_mask, haddr_t *addr, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dget_chunk_info_by_coord, (dset_id, offset, filter_mask, addr, size));
	char **args = assemble_args_list(5, itoa(dset_id),itoa((offset==NULL)?-1:*offset),itoa((filter_mask==NULL)?-1:*filter_mask),ptoa(addr),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_size)(hid_t id, const char *name, size_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_size, (id, name, size));
	char **args = assemble_args_list(3, itoa(id),strtoa(name),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_virtual_count)(hid_t dcpl_id, size_t *count) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_virtual_count, (dcpl_id, count));
	char **args = assemble_args_list(2, itoa(dcpl_id),itoa((count==NULL)?-1:*count));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

H5Z_EDC_t WRAPPER_NAME(H5Pget_edc_check)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5Z_EDC_t, H5Pget_edc_check, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5VLquery_optional)(hid_t obj_id, H5VL_subclass_t subcls, int opt_type, uint64_t *flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5VLquery_optional, (obj_id, subcls, opt_type, flags));
	char **args = assemble_args_list(4, itoa(obj_id),ptoa(&subcls),itoa(opt_type),itoa((flags==NULL)?-1:*flags));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5dont_atexit)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5dont_atexit, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5Pget_vol_id)(hid_t plist_id, hid_t *vol_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_vol_id, (plist_id, vol_id));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((vol_id==NULL)?-1:*vol_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5IMis_image)(hid_t loc_id, const char *dset_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMis_image, (loc_id, dset_name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(dset_name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5PLget)(unsigned int index, char *path_buf, size_t buf_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5PLget, (index, path_buf, buf_size));
	char **args = assemble_args_list(3, itoa(index),strtoa(path_buf),itoa(buf_size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5IMlink_palette)(hid_t loc_id, const char *image_name, const char *pal_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMlink_palette, (loc_id, image_name, pal_name));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(image_name),strtoa(pal_name));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pfill_value_defined)(hid_t plist, H5D_fill_value_t *status) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pfill_value_defined, (plist, status));
	char **args = assemble_args_list(2, itoa(plist),ptoa(status));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Oopen_by_token)(hid_t loc_id, H5O_token_t token) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen_by_token, (loc_id, token));
	char **args = assemble_args_list(2, itoa(loc_id),ptoa(&token));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5Pget_virtual_dsetname)(hid_t dcpl_id, size_t index, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Pget_virtual_dsetname, (dcpl_id, index, name, size));
	char **args = assemble_args_list(4, itoa(dcpl_id),itoa(index),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget)(hid_t plist_id, const char *name, void *value) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget, (plist_id, name, value));
	char **args = assemble_args_list(3, itoa(plist_id),strtoa(name),ptoa(value));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Dwrite)(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dwrite, (dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf));
	char **args = assemble_args_list(6, itoa(dset_id),itoa(mem_type_id),itoa(mem_space_id),itoa(file_space_id),itoa(dxpl_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_uchar)(hid_t loc_id, const char *obj_name, const char *attr_name, const unsigned char *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_uchar, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),strtoa(buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_vlen_mem_manager)(hid_t plist_id, H5MM_allocate_t *alloc_func, void **alloc_info, H5MM_free_t *free_func, void **free_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_vlen_mem_manager, (plist_id, alloc_func, alloc_info, free_func, free_info));
	char **args = assemble_args_list(5, itoa(plist_id),ptoa(alloc_func),ptoa(alloc_info),ptoa(free_func),ptoa(free_info));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Dcreate2)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dcreate2, (loc_id, name, type_id, space_id, lcpl_id, dcpl_id, dapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(name),itoa(type_id),itoa(space_id),itoa(lcpl_id),itoa(dcpl_id),itoa(dapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

htri_t WRAPPER_NAME(H5TBAget_fill)(hid_t loc_id, const char *dset_name, hid_t dset_id, unsigned char *dst_buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5TBAget_fill, (loc_id, dset_name, dset_id, dst_buf));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(dset_name),itoa(dset_id),strtoa(dst_buf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_driver_by_name)(hid_t plist_id, const char *driver_name, const char *driver_config) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_driver_by_name, (plist_id, driver_name, driver_config));
	char **args = assemble_args_list(3, itoa(plist_id),strtoa(driver_name),strtoa(driver_config));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5Tcommitted)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Tcommitted, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_attr_creation_order)(hid_t plist_id, unsigned crt_order_flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_attr_creation_order, (plist_id, crt_order_flags));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(crt_order_flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5PLsize)(unsigned int *num_paths) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLsize, (num_paths));
	char **args = assemble_args_list(1, itoa((num_paths==NULL)?-1:*num_paths));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Lunpack_elink_val)(const void *ext_linkval, size_t link_size, unsigned *flags, const char **filename, const char **obj_path) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lunpack_elink_val, (ext_linkval, link_size, flags, filename, obj_path));
	char **args = assemble_args_list(5, ptoa(ext_linkval),itoa(link_size),itoa((flags==NULL)?-1:*flags),ptoa(filename),ptoa(obj_path));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Fget_eoa)(hid_t file_id, haddr_t *eoa) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_eoa, (file_id, eoa));
	char **args = assemble_args_list(2, itoa(file_id),ptoa(eoa));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int WRAPPER_NAME(H5Sget_simple_extent_dims)(hid_t space_id, hsize_t dims[], hsize_t maxdims[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Sget_simple_extent_dims, (space_id, dims, maxdims));
	char **args = assemble_args_list(3, itoa(space_id),ptoa(dims),ptoa(maxdims));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5PTget_dataset)(hid_t table_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5PTget_dataset, (table_id));
	char **args = assemble_args_list(1, itoa(table_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5TBadd_records_from)(hid_t loc_id, const char *dset_name1, hsize_t start1, hsize_t nrecords, const char *dset_name2, hsize_t start2) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBadd_records_from, (loc_id, dset_name1, start1, nrecords, dset_name2, start2));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(dset_name1),itoa(start1),itoa(nrecords),strtoa(dset_name2),itoa(start2));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Eset_auto2)(hid_t estack_id, H5E_auto2_t func, void *client_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eset_auto2, (estack_id, func, client_data));
	char **args = assemble_args_list(3, itoa(estack_id),ptoa(&func),ptoa(client_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5TBcombine_tables)(hid_t loc_id1, const char *dset_name1, hid_t loc_id2, const char *dset_name2, const char *dset_name3) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBcombine_tables, (loc_id1, dset_name1, loc_id2, dset_name2, dset_name3));
	char **args = assemble_args_list(5, itoa(loc_id1),strtoa(dset_name1),itoa(loc_id2),strtoa(dset_name2),strtoa(dset_name3));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

H5Z_filter_t WRAPPER_NAME(H5Pget_filter1)(hid_t plist_id, unsigned filter, unsigned int *flags, size_t *cd_nelmts, unsigned cd_values[], size_t namelen, char name[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5Z_filter_t, H5Pget_filter1, (plist_id, filter, flags, cd_nelmts, cd_values, namelen, name));
	char **args = assemble_args_list(7, itoa(plist_id),itoa(filter),itoa((flags==NULL)?-1:*flags),itoa((cd_nelmts==NULL)?-1:*cd_nelmts),ptoa(cd_values),itoa(namelen),ptoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Lget_val_by_idx)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, void *buf, size_t size, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_val_by_idx, (loc_id, group_name, idx_type, order, n, buf, size, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(buf),itoa(size),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Oclose)(hid_t object_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oclose, (object_id));
	char **args = assemble_args_list(1, itoa(object_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Fget_fileno)(hid_t file_id, unsigned long *fileno) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_fileno, (file_id, fileno));
	char **args = assemble_args_list(2, itoa(file_id),itoa((fileno==NULL)?-1:*fileno));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Fcreate)(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fcreate, (filename, flags, fcpl_id, fapl_id));
	char **args = assemble_args_list(4, strtoa(filename),itoa(flags),itoa(fcpl_id),itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_userblock)(hid_t plist_id, hsize_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_userblock, (plist_id, size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5Oget_comment)(hid_t obj_id, char *comment, size_t bufsize) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Oget_comment, (obj_id, comment, bufsize));
	char **args = assemble_args_list(3, itoa(obj_id),strtoa(comment),itoa(bufsize));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_chunk)(hid_t plist_id, int ndims, const hsize_t dim[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_chunk, (plist_id, ndims, dim));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(ndims),ptoa(dim));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5DSis_attached)(hid_t did, hid_t dsid, unsigned int idx) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5DSis_attached, (did, dsid, idx));
	char **args = assemble_args_list(3, itoa(did),itoa(dsid),itoa(idx));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_string)(hid_t loc_id, const char *dset_name, const char *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_string, (loc_id, dset_name, buf));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),strtoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Dread_chunk)(hid_t dset_id, hid_t dxpl_id, const hsize_t *offset, uint32_t *filters, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dread_chunk, (dset_id, dxpl_id, offset, filters, buf));
	char **args = assemble_args_list(5, itoa(dset_id),itoa(dxpl_id),itoa((offset==NULL)?-1:*offset),itoa((filters==NULL)?-1:*filters),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5VLget_connector_id)(hid_t obj_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5VLget_connector_id, (obj_id));
	char **args = assemble_args_list(1, itoa(obj_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_layout)(hid_t plist_id, H5D_layout_t layout) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_layout, (plist_id, layout));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&layout));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t WRAPPER_NAME(H5Sselect_valid)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sselect_valid, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Tencode)(hid_t obj_id, void *buf, size_t *nalloc) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tencode, (obj_id, buf, nalloc));
	char **args = assemble_args_list(3, itoa(obj_id),ptoa(buf),itoa((nalloc==NULL)?-1:*nalloc));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_dset_no_attrs_hint)(hid_t dcpl_id, hbool_t *minimize) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_dset_no_attrs_hint, (dcpl_id, minimize));
	char **args = assemble_args_list(2, itoa(dcpl_id),itoa((minimize==NULL)?-1:*minimize));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_btree_ratios)(hid_t plist_id, double left, double middle, double right) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_btree_ratios, (plist_id, left, middle, right));
	char **args = assemble_args_list(4, itoa(plist_id),ftoa(left),ftoa(middle),ftoa(right));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

H5I_type_t WRAPPER_NAME(H5Iget_type)(hid_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5I_type_t, H5Iget_type, (id));
	char **args = assemble_args_list(1, itoa(id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LRmake_dataset)(hid_t loc_id, const char *path, hid_t type_id, const size_t buf_size, const hid_t *loc_id_ref, const hdset_reg_ref_t *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRmake_dataset, (loc_id, path, type_id, buf_size, loc_id_ref, buf));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(path),itoa(type_id),itoa(buf_size),itoa((loc_id_ref==NULL)?-1:*loc_id_ref),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Pclose)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pclose, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Fclear_elink_file_cache)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fclear_elink_file_cache, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5TBmake_table)(const char *table_title, hid_t loc_id, const char *dset_name, hsize_t nfields, hsize_t nrecords, size_t type_size, const char *field_names[], const size_t *field_offset, const hid_t *field_types, hsize_t chunk_size, void *fill_data, int compress, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBmake_table, (table_title, loc_id, dset_name, nfields, nrecords, type_size, field_names, field_offset, field_types, chunk_size, fill_data, compress, buf));
	char **args = assemble_args_list(13, strtoa(table_title),itoa(loc_id),strtoa(dset_name),itoa(nfields),itoa(nrecords),itoa(type_size),ptoa(field_names),itoa((field_offset==NULL)?-1:*field_offset),itoa((field_types==NULL)?-1:*field_types),itoa(chunk_size),ptoa(fill_data),itoa(compress),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(13, args);
}

htri_t WRAPPER_NAME(H5Pequal)(hid_t id1, hid_t id2) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Pequal, (id1, id2));
	char **args = assemble_args_list(2, itoa(id1),itoa(id2));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_evict_on_close)(hid_t fapl_id, hbool_t evict_on_close) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_evict_on_close, (fapl_id, evict_on_close));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa(evict_on_close));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Dopen2)(hid_t loc_id, const char *name, hid_t dapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dopen2, (loc_id, name, dapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(dapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Ldelete_by_idx)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ldelete_by_idx, (loc_id, group_name, idx_type, order, n, lapl_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_long_long)(hid_t loc_id, const char *obj_name, const char *attr_name, const long long *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_long_long, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Oget_native_info_by_idx)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_native_info_t *oinfo, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_native_info_by_idx, (loc_id, group_name, idx_type, order, n, oinfo, fields, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(oinfo),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Pget_create_intermediate_group)(hid_t plist_id, unsigned *crt_intmd) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_create_intermediate_group, (plist_id, crt_intmd));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((crt_intmd==NULL)?-1:*crt_intmd));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Epush1)(const char *file, const char *func, unsigned line, H5E_major_t maj, H5E_minor_t min, const char *str) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Epush1, (file, func, line, maj, min, str));
	char **args = assemble_args_list(6, strtoa(file),strtoa(func),itoa(line),ptoa(&maj),ptoa(&min),strtoa(str));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Gset_comment)(hid_t loc_id, const char *name, const char *comment) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gset_comment, (loc_id, name, comment));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),strtoa(comment));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

ssize_t WRAPPER_NAME(H5Pget_data_transform)(hid_t plist_id, char *expression, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Pget_data_transform, (plist_id, expression, size));
	char **args = assemble_args_list(3, itoa(plist_id),strtoa(expression),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_sizes)(hid_t plist_id, size_t *sizeof_addr, size_t *sizeof_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_sizes, (plist_id, sizeof_addr, sizeof_size));
	char **args = assemble_args_list(3, itoa(plist_id),itoa((sizeof_addr==NULL)?-1:*sizeof_addr),itoa((sizeof_size==NULL)?-1:*sizeof_size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

ssize_t WRAPPER_NAME(H5Lget_name_by_idx)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, char *name, size_t size, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Lget_name_by_idx, (loc_id, group_name, idx_type, order, n, name, size, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),strtoa(name),itoa(size),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5PTread_packets)(hid_t table_id, hsize_t start, size_t nrecords, void *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTread_packets, (table_id, start, nrecords, data));
	char **args = assemble_args_list(4, itoa(table_id),itoa(start),itoa(nrecords),ptoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Oget_info1)(hid_t loc_id, H5O_info1_t *oinfo) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info1, (loc_id, oinfo));
	char **args = assemble_args_list(2, itoa(loc_id),ptoa(oinfo));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5DSattach_scale)(hid_t did, hid_t dsid, unsigned int idx) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DSattach_scale, (did, dsid, idx));
	char **args = assemble_args_list(3, itoa(did),itoa(dsid),itoa(idx));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Zunregister)(H5Z_filter_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Zunregister, (id));
	char **args = assemble_args_list(1, ptoa(&id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_cache)(hid_t plist_id, int mdc_nelmts, size_t rdcc_nslots, size_t rdcc_nbytes, double rdcc_w0) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_cache, (plist_id, mdc_nelmts, rdcc_nslots, rdcc_nbytes, rdcc_w0));
	char **args = assemble_args_list(5, itoa(plist_id),itoa(mdc_nelmts),itoa(rdcc_nslots),itoa(rdcc_nbytes),ftoa(rdcc_w0));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

void * WRAPPER_NAME(H5resize_memory)(void *mem, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(void *, H5resize_memory, (mem, size));
	char **args = assemble_args_list(2, ptoa(mem),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBdelete_field)(hid_t loc_id, const char *dset_name, const char *field_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBdelete_field, (loc_id, dset_name, field_name));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),strtoa(field_name));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Padd_merge_committed_dtype_path)(hid_t plist_id, const char *path) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Padd_merge_committed_dtype_path, (plist_id, path));
	char **args = assemble_args_list(2, itoa(plist_id),strtoa(path));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Tget_create_plist)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tget_create_plist, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_edc_check)(hid_t plist_id, H5Z_EDC_t check) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_edc_check, (plist_id, check));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&check));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_vlen_mem_manager)(hid_t plist_id, H5MM_allocate_t alloc_func, void *alloc_info, H5MM_free_t free_func, void *free_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_vlen_mem_manager, (plist_id, alloc_func, alloc_info, free_func, free_info));
	char **args = assemble_args_list(5, itoa(plist_id),ptoa(&alloc_func),ptoa(alloc_info),ptoa(&free_func),ptoa(free_info));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Grefresh)(hid_t group_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Grefresh, (group_id));
	char **args = assemble_args_list(1, itoa(group_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Dvlen_get_buf_size)(hid_t dset_id, hid_t type_id, hid_t space_id, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dvlen_get_buf_size, (dset_id, type_id, space_id, size));
	char **args = assemble_args_list(4, itoa(dset_id),itoa(type_id),itoa(space_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_libver_bounds)(hid_t plist_id, H5F_libver_t low, H5F_libver_t high) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_libver_bounds, (plist_id, low, high));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(&low),ptoa(&high));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Tcommit_anon)(hid_t loc_id, hid_t type_id, hid_t tcpl_id, hid_t tapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tcommit_anon, (loc_id, type_id, tcpl_id, tapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),itoa(type_id),itoa(tcpl_id),itoa(tapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Oset_comment_by_name)(hid_t loc_id, const char *name, const char *comment, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oset_comment_by_name, (loc_id, name, comment, lapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),strtoa(comment),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_short)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, const short *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_short, (loc_id, dset_name, rank, dims, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),itoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Epop)(hid_t err_stack, size_t count) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Epop, (err_stack, count));
	char **args = assemble_args_list(2, itoa(err_stack),itoa(count));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5IMget_palette)(hid_t loc_id, const char *image_name, int pal_number, unsigned char *pal_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMget_palette, (loc_id, image_name, pal_number, pal_data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(image_name),itoa(pal_number),strtoa(pal_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, hid_t type_id, const void *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset, (loc_id, dset_name, rank, dims, type_id, buffer));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),itoa(type_id),ptoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Pset_coll_metadata_write)(hid_t plist_id, hbool_t is_collective) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_coll_metadata_write, (plist_id, is_collective));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(is_collective));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Sencode1)(hid_t obj_id, void *buf, size_t *nalloc) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sencode1, (obj_id, buf, nalloc));
	char **args = assemble_args_list(3, itoa(obj_id),ptoa(buf),itoa((nalloc==NULL)?-1:*nalloc));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_fill_value)(hid_t plist_id, hid_t type_id, const void *value) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fill_value, (plist_id, type_id, value));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(type_id),ptoa(value));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_alignment)(hid_t fapl_id, hsize_t threshold, hsize_t alignment) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_alignment, (fapl_id, threshold, alignment));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa(threshold),itoa(alignment));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Gclose)(hid_t group_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gclose, (group_id));
	char **args = assemble_args_list(1, itoa(group_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

htri_t WRAPPER_NAME(H5Sis_simple)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sis_simple, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5DSdetach_scale)(hid_t did, hid_t dsid, unsigned int idx) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DSdetach_scale, (did, dsid, idx));
	char **args = assemble_args_list(3, itoa(did),itoa(dsid),itoa(idx));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Dcreate1)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dcreate1, (loc_id, name, type_id, space_id, dcpl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),itoa(type_id),itoa(space_id),itoa(dcpl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_copy_object)(hid_t plist_id, unsigned *copy_options) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_copy_object, (plist_id, copy_options));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((copy_options==NULL)?-1:*copy_options));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Ssel_iter_get_seq_list)(hid_t sel_iter_id, size_t maxseq, size_t maxelmts, size_t *nseq, size_t *nelmts, hsize_t *off, size_t *len) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ssel_iter_get_seq_list, (sel_iter_id, maxseq, maxelmts, nseq, nelmts, off, len));
	char **args = assemble_args_list(7, itoa(sel_iter_id),itoa(maxseq),itoa(maxelmts),itoa((nseq==NULL)?-1:*nseq),itoa((nelmts==NULL)?-1:*nelmts),itoa((off==NULL)?-1:*off),itoa((len==NULL)?-1:*len));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Pset_dataset_io_hyperslab_selection)(hid_t plist_id, unsigned rank, H5S_seloper_t op, const hsize_t start[], const hsize_t stride[], const hsize_t count[], const hsize_t block[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_dataset_io_hyperslab_selection, (plist_id, rank, op, start, stride, count, block));
	char **args = assemble_args_list(7, itoa(plist_id),itoa(rank),ptoa(&op),ptoa(start),ptoa(stride),ptoa(count),ptoa(block));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Pset_chunk_opts)(hid_t plist_id, unsigned opts) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_chunk_opts, (plist_id, opts));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(opts));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5PTcreate_fl)(hid_t loc_id, const char *dset_name, hid_t dtype_id, hsize_t chunk_size, int compression) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5PTcreate_fl, (loc_id, dset_name, dtype_id, chunk_size, compression));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(dtype_id),itoa(chunk_size),itoa(compression));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Aiterate1)(hid_t loc_id, unsigned *idx, H5A_operator1_t op, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aiterate1, (loc_id, idx, op, op_data));
	char **args = assemble_args_list(4, itoa(loc_id),itoa((idx==NULL)?-1:*idx),ptoa(&op),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Topen1)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Topen1, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Sselect_project_intersection)(hid_t src_space_id, hid_t dst_space_id, hid_t src_intersect_space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Sselect_project_intersection, (src_space_id, dst_space_id, src_intersect_space_id));
	char **args = assemble_args_list(3, itoa(src_space_id),itoa(dst_space_id),itoa(src_intersect_space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_int)(hid_t loc_id, const char *obj_name, const char *attr_name, const int *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_int, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_elink_acc_flags)(hid_t lapl_id, unsigned *flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_elink_acc_flags, (lapl_id, flags));
	char **args = assemble_args_list(2, itoa(lapl_id),itoa((flags==NULL)?-1:*flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_page_buffer_size)(hid_t plist_id, size_t buf_size, unsigned min_meta_per, unsigned min_raw_per) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_page_buffer_size, (plist_id, buf_size, min_meta_per, min_raw_per));
	char **args = assemble_args_list(4, itoa(plist_id),itoa(buf_size),itoa(min_meta_per),itoa(min_raw_per));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Fclose)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fclose, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Giterate)(hid_t loc_id, const char *name, int *idx, H5G_iterate_t op, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Giterate, (loc_id, name, idx, op, op_data));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),itoa((idx==NULL)?-1:*idx),ptoa(&op),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

htri_t WRAPPER_NAME(H5Tequal)(hid_t type1_id, hid_t type2_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Tequal, (type1_id, type2_id));
	char **args = assemble_args_list(2, itoa(type1_id),itoa(type2_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Oget_native_info_by_name)(hid_t loc_id, const char *name, H5O_native_info_t *oinfo, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_native_info_by_name, (loc_id, name, oinfo, fields, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),ptoa(oinfo),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_file_space)(hid_t plist_id, H5F_file_space_type_t strategy, hsize_t threshold) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_file_space, (plist_id, strategy, threshold));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(&strategy),itoa(threshold));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5DOwrite_chunk)(hid_t dset_id, hid_t dxpl_id, uint32_t filters, const hsize_t *offset, size_t data_size, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DOwrite_chunk, (dset_id, dxpl_id, filters, offset, data_size, buf));
	char **args = assemble_args_list(6, itoa(dset_id),itoa(dxpl_id),itoa(filters),itoa((offset==NULL)?-1:*offset),itoa(data_size),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5PTget_num_packets)(hid_t table_id, hsize_t *nrecords) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTget_num_packets, (table_id, nrecords));
	char **args = assemble_args_list(2, itoa(table_id),itoa((nrecords==NULL)?-1:*nrecords));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Dget_access_plist)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_access_plist, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_mdc_log_options)(hid_t plist_id, hbool_t is_enabled, const char *location, hbool_t start_on_access) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_mdc_log_options, (plist_id, is_enabled, location, start_on_access));
	char **args = assemble_args_list(4, itoa(plist_id),itoa(is_enabled),strtoa(location),itoa(start_on_access));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Adelete)(hid_t loc_id, const char *attr_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Adelete, (loc_id, attr_name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(attr_name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_shuffle)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_shuffle, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_file_space_strategy)(hid_t plist_id, H5F_fspace_strategy_t strategy, hbool_t persist, hsize_t threshold) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_file_space_strategy, (plist_id, strategy, persist, threshold));
	char **args = assemble_args_list(4, itoa(plist_id),ptoa(&strategy),itoa(persist),itoa(threshold));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Eauto_is_v2)(hid_t err_stack, unsigned *is_stack) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eauto_is_v2, (err_stack, is_stack));
	char **args = assemble_args_list(2, itoa(err_stack),itoa((is_stack==NULL)?-1:*is_stack));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Aopen_name)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_name, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Ecreate_msg)(hid_t cls, H5E_type_t msg_type, const char *msg) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ecreate_msg, (cls, msg_type, msg));
	char **args = assemble_args_list(3, itoa(cls),ptoa(&msg_type),strtoa(msg));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Odecr_refcount)(hid_t object_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Odecr_refcount, (object_id));
	char **args = assemble_args_list(1, itoa(object_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Rget_region)(hid_t dataset, H5R_type_t ref_type, const void *ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Rget_region, (dataset, ref_type, ref));
	char **args = assemble_args_list(3, itoa(dataset),ptoa(&ref_type),ptoa(ref));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Scombine_select)(hid_t space1_id, H5S_seloper_t op, hid_t space2_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Scombine_select, (space1_id, op, space2_id));
	char **args = assemble_args_list(3, itoa(space1_id),ptoa(&op),itoa(space2_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_driver)(hid_t plist_id, hid_t driver_id, const void *driver_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_driver, (plist_id, driver_id, driver_info));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(driver_id),ptoa(driver_info));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Dget_create_plist)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_create_plist, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_split)(hid_t fapl, const char *meta_ext, hid_t meta_plist_id, const char *raw_ext, hid_t raw_plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_split, (fapl, meta_ext, meta_plist_id, raw_ext, raw_plist_id));
	char **args = assemble_args_list(5, itoa(fapl),strtoa(meta_ext),itoa(meta_plist_id),strtoa(raw_ext),itoa(raw_plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Gget_num_objs)(hid_t loc_id, hsize_t *num_objs) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_num_objs, (loc_id, num_objs));
	char **args = assemble_args_list(2, itoa(loc_id),itoa((num_objs==NULL)?-1:*num_objs));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Dget_chunk_info)(hid_t dset_id, hid_t fspace_id, hsize_t chk_idx, hsize_t *offset, unsigned *filter_mask, haddr_t *addr, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dget_chunk_info, (dset_id, fspace_id, chk_idx, offset, filter_mask, addr, size));
	char **args = assemble_args_list(7, itoa(dset_id),itoa(fspace_id),itoa(chk_idx),itoa((offset==NULL)?-1:*offset),itoa((filter_mask==NULL)?-1:*filter_mask),ptoa(addr),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5PTcreate_index)(hid_t table_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTcreate_index, (table_id));
	char **args = assemble_args_list(1, itoa(table_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int WRAPPER_NAME(H5Aget_num_attrs)(hid_t loc_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Aget_num_attrs, (loc_id));
	char **args = assemble_args_list(1, itoa(loc_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Oenable_mdc_flushes)(hid_t object_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oenable_mdc_flushes, (object_id));
	char **args = assemble_args_list(1, itoa(object_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTdtype_to_text)(hid_t dtype, char *str, H5LT_lang_t lang_type, size_t *len) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTdtype_to_text, (dtype, str, lang_type, len));
	char **args = assemble_args_list(4, itoa(dtype),strtoa(str),ptoa(&lang_type),itoa((len==NULL)?-1:*len));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LRcopy_reference)(hid_t obj_id, hdset_reg_ref_t *ref, const char *file, const char *path, const hsize_t *block_coord, hdset_reg_ref_t *ref_new) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRcopy_reference, (obj_id, ref, file, path, block_coord, ref_new));
	char **args = assemble_args_list(6, itoa(obj_id),ptoa(ref),strtoa(file),strtoa(path),itoa((block_coord==NULL)?-1:*block_coord),ptoa(ref_new));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Pget_modify_write_buf)(hid_t plist_id, hbool_t *modify_write_buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_modify_write_buf, (plist_id, modify_write_buf));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((modify_write_buf==NULL)?-1:*modify_write_buf));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBget_field_info)(hid_t loc_id, const char *dset_name, char *field_names[], size_t *field_sizes, size_t *field_offsets, size_t *type_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBget_field_info, (loc_id, dset_name, field_names, field_sizes, field_offsets, type_size));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(dset_name),ptoa(field_names),itoa((field_sizes==NULL)?-1:*field_sizes),itoa((field_offsets==NULL)?-1:*field_offsets),itoa((type_size==NULL)?-1:*type_size));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Pget_filter_by_id2)(hid_t plist_id, H5Z_filter_t filter_id, unsigned int *flags, size_t *cd_nelmts, unsigned cd_values[], size_t namelen, char name[], unsigned *filter_config) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_filter_by_id2, (plist_id, filter_id, flags, cd_nelmts, cd_values, namelen, name, filter_config));
	char **args = assemble_args_list(8, itoa(plist_id),ptoa(&filter_id),itoa((flags==NULL)?-1:*flags),itoa((cd_nelmts==NULL)?-1:*cd_nelmts),ptoa(cd_values),itoa(namelen),ptoa(name),itoa((filter_config==NULL)?-1:*filter_config));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Zregister)(const void *cls) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Zregister, (cls));
	char **args = assemble_args_list(1, ptoa(cls));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_uint)(hid_t loc_id, const char *obj_name, const char *attr_name, unsigned int *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_uint, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

ssize_t WRAPPER_NAME(H5Fget_file_image)(hid_t file_id, void *buf_ptr, size_t buf_len) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Fget_file_image, (file_id, buf_ptr, buf_len));
	char **args = assemble_args_list(3, itoa(file_id),ptoa(buf_ptr),itoa(buf_len));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Gget_objinfo)(hid_t loc_id, const char *name, hbool_t follow_link, H5G_stat_t *statbuf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_objinfo, (loc_id, name, follow_link, statbuf));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(follow_link),ptoa(statbuf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Gcreate1)(hid_t loc_id, const char *name, size_t size_hint) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gcreate1, (loc_id, name, size_hint));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(size_hint));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Oopen_by_addr)(hid_t loc_id, haddr_t addr) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen_by_addr, (loc_id, addr));
	char **args = assemble_args_list(2, itoa(loc_id),ptoa(&addr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_mpi_params)(hid_t fapl_id, MPI_Comm *comm, MPI_Info *info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mpi_params, (fapl_id, comm, info));
	char **args = assemble_args_list(3, itoa(fapl_id),ptoa(comm),ptoa(info));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5is_library_threadsafe)(hbool_t *is_ts) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5is_library_threadsafe, (is_ts));
	char **args = assemble_args_list(1, itoa((is_ts==NULL)?-1:*is_ts));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hssize_t WRAPPER_NAME(H5Sget_select_npoints)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Sget_select_npoints, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_ulong)(hid_t loc_id, const char *obj_name, const char *attr_name, const unsigned long *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_ulong, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5VLget_connector_id_by_value)(H5VL_class_value_t connector_value) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5VLget_connector_id_by_value, (connector_value));
	char **args = assemble_args_list(1, ptoa(&connector_value));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_nprops)(hid_t id, size_t *nprops) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_nprops, (id, nprops));
	char **args = assemble_args_list(2, itoa(id),itoa((nprops==NULL)?-1:*nprops));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_shared_mesg_phase_change)(hid_t plist_id, unsigned max_list, unsigned min_btree) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_shared_mesg_phase_change, (plist_id, max_list, min_btree));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(max_list),itoa(min_btree));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_metadata_read_attempts)(hid_t plist_id, unsigned *attempts) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_metadata_read_attempts, (plist_id, attempts));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((attempts==NULL)?-1:*attempts));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Sselect_hyperslab)(hid_t space_id, H5S_seloper_t op, const hsize_t start[], const hsize_t stride[], const hsize_t count[], const hsize_t block[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_hyperslab, (space_id, op, start, stride, count, block));
	char **args = assemble_args_list(6, itoa(space_id),ptoa(&op),ptoa(start),ptoa(stride),ptoa(count),ptoa(block));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

htri_t WRAPPER_NAME(H5Oexists_by_name)(hid_t loc_id, const char *name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Oexists_by_name, (loc_id, name, lapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_alignment)(hid_t fapl_id, hsize_t *threshold, hsize_t *alignment) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_alignment, (fapl_id, threshold, alignment));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa((threshold==NULL)?-1:*threshold),itoa((alignment==NULL)?-1:*alignment));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

ssize_t WRAPPER_NAME(H5Eget_msg)(hid_t msg_id, H5E_type_t *type, char *msg, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Eget_msg, (msg_id, type, msg, size));
	char **args = assemble_args_list(4, itoa(msg_id),ptoa(type),strtoa(msg),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_attr_phase_change)(hid_t plist_id, unsigned max_compact, unsigned min_dense) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_attr_phase_change, (plist_id, max_compact, min_dense));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(max_compact),itoa(min_dense));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int WRAPPER_NAME(H5Iget_ref)(hid_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Iget_ref, (id));
	char **args = assemble_args_list(1, itoa(id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Acreate_by_name)(hid_t loc_id, const char *obj_name, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate_by_name, (loc_id, obj_name, attr_name, type_id, space_id, acpl_id, aapl_id, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa(type_id),itoa(space_id),itoa(acpl_id),itoa(aapl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Dflush)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dflush, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_float)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, const float *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_float, (loc_id, dset_name, rank, dims, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),ftoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_mpio_no_collective_cause)(hid_t plist_id, uint32_t *local_no_collective_cause, uint32_t *global_no_collective_cause) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mpio_no_collective_cause, (plist_id, local_no_collective_cause, global_no_collective_cause));
	char **args = assemble_args_list(3, itoa(plist_id),itoa((local_no_collective_cause==NULL)?-1:*local_no_collective_cause),itoa((global_no_collective_cause==NULL)?-1:*global_no_collective_cause));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Tflush)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tflush, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_mpio_actual_chunk_opt_mode)(hid_t plist_id, H5D_mpio_actual_chunk_opt_mode_t *actual_chunk_opt_mode) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mpio_actual_chunk_opt_mode, (plist_id, actual_chunk_opt_mode));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(actual_chunk_opt_mode));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5PTget_type)(hid_t table_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5PTget_type, (table_id));
	char **args = assemble_args_list(1, itoa(table_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Gmove2)(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id, const char *dst_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gmove2, (src_loc_id, src_name, dst_loc_id, dst_name));
	char **args = assemble_args_list(4, itoa(src_loc_id),strtoa(src_name),itoa(dst_loc_id),strtoa(dst_name));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_ros3_token)(hid_t fapl_id, const char *token) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_ros3_token, (fapl_id, token));
	char **args = assemble_args_list(2, itoa(fapl_id),strtoa(token));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LDget_dset_elmts)(hid_t did, const hsize_t *prev_dims, const hsize_t *cur_dims, const char *fields, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LDget_dset_elmts, (did, prev_dims, cur_dims, fields, buf));
	char **args = assemble_args_list(5, itoa(did),itoa((prev_dims==NULL)?-1:*prev_dims),itoa((cur_dims==NULL)?-1:*cur_dims),strtoa(fields),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_scaleoffset)(hid_t plist_id, H5Z_SO_scale_type_t scale_type, int scale_factor) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_scaleoffset, (plist_id, scale_type, scale_factor));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(&scale_type),itoa(scale_factor));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Premove)(hid_t plist_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Premove, (plist_id, name));
	char **args = assemble_args_list(2, itoa(plist_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t WRAPPER_NAME(H5Fis_hdf5)(const char *file_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Fis_hdf5, (file_name));
	char **args = assemble_args_list(1, strtoa(file_name));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Aopen_by_name)(hid_t loc_id, const char *obj_name, const char *attr_name, hid_t aapl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_by_name, (loc_id, obj_name, attr_name, aapl_id, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa(aapl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Eget_auto2)(hid_t estack_id, H5E_auto2_t *func, void **client_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eget_auto2, (estack_id, func, client_data));
	char **args = assemble_args_list(3, itoa(estack_id),ptoa(func),ptoa(client_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_core_write_tracking)(hid_t fapl_id, hbool_t *is_enabled, size_t *page_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_core_write_tracking, (fapl_id, is_enabled, page_size));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa((is_enabled==NULL)?-1:*is_enabled),itoa((page_size==NULL)?-1:*page_size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5Sselect_intersect_block)(hid_t space_id, const hsize_t *start, const hsize_t *end) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sselect_intersect_block, (space_id, start, end));
	char **args = assemble_args_list(3, itoa(space_id),itoa((start==NULL)?-1:*start),itoa((end==NULL)?-1:*end));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5Sextent_equal)(hid_t space1_id, hid_t space2_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sextent_equal, (space1_id, space2_id));
	char **args = assemble_args_list(2, itoa(space1_id),itoa(space2_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Pget_virtual_srcspace)(hid_t dcpl_id, size_t index) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pget_virtual_srcspace, (dcpl_id, index));
	char **args = assemble_args_list(2, itoa(dcpl_id),itoa(index));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_dataset_info)(hid_t loc_id, const char *dset_name, hsize_t *dims, H5T_class_t *type_class, size_t *type_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_dataset_info, (loc_id, dset_name, dims, type_class, type_size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa((dims==NULL)?-1:*dims),ptoa(type_class),itoa((type_size==NULL)?-1:*type_size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

ssize_t WRAPPER_NAME(H5Fget_free_sections)(hid_t file_id, H5F_mem_t type, size_t nsects, H5F_sect_info_t *sect_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Fget_free_sections, (file_id, type, nsects, sect_info));
	char **args = assemble_args_list(4, itoa(file_id),ptoa(&type),itoa(nsects),ptoa(sect_info));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Tset_size)(hid_t type_id, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tset_size, (type_id, size));
	char **args = assemble_args_list(2, itoa(type_id),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBAget_title)(hid_t loc_id, char *table_title) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBAget_title, (loc_id, table_title));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(table_title));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5garbage_collect)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5garbage_collect, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5Pget_char_encoding)(hid_t plist_id, H5T_cset_t *encoding) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_char_encoding, (plist_id, encoding));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(encoding));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Lget_val)(hid_t loc_id, const char *name, void *buf, size_t size, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_val, (loc_id, name, buf, size, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),ptoa(buf),itoa(size),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pregister1)(hid_t cls_id, const char *name, size_t size, void *def_value, H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_del, H5P_prp_copy_func_t prp_copy, H5P_prp_close_func_t prp_close) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pregister1, (cls_id, name, size, def_value, prp_create, prp_set, prp_get, prp_del, prp_copy, prp_close));
	char **args = assemble_args_list(10, itoa(cls_id),strtoa(name),itoa(size),ptoa(def_value),ptoa(&prp_create),ptoa(&prp_set),ptoa(&prp_get),ptoa(&prp_del),ptoa(&prp_copy),ptoa(&prp_close));
	RECORDER_INTERCEPTOR_EPILOGUE(10, args);
}

herr_t WRAPPER_NAME(H5Aget_info_by_name)(hid_t loc_id, const char *obj_name, const char *attr_name, H5A_info_t *ainfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aget_info_by_name, (loc_id, obj_name, attr_name, ainfo, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),ptoa(ainfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_file_space_page_size)(hid_t plist_id, hsize_t *fsp_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_file_space_page_size, (plist_id, fsp_size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((fsp_size==NULL)?-1:*fsp_size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pregister2)(hid_t cls_id, const char *name, size_t size, void *def_value, H5P_prp_create_func_t create, H5P_prp_set_func_t set, H5P_prp_get_func_t get, H5P_prp_delete_func_t prp_del, H5P_prp_copy_func_t copy, H5P_prp_compare_func_t compare, H5P_prp_close_func_t close) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pregister2, (cls_id, name, size, def_value, create, set, get, prp_del, copy, compare, close));
	char **args = assemble_args_list(11, itoa(cls_id),strtoa(name),itoa(size),ptoa(def_value),ptoa(&create),ptoa(&set),ptoa(&get),ptoa(&prp_del),ptoa(&copy),ptoa(&compare),ptoa(&close));
	RECORDER_INTERCEPTOR_EPILOGUE(11, args);
}

int WRAPPER_NAME(H5Idec_ref)(hid_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Idec_ref, (id));
	char **args = assemble_args_list(1, itoa(id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_vol)(hid_t plist_id, hid_t new_vol_id, const void *new_vol_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_vol, (plist_id, new_vol_id, new_vol_info));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(new_vol_id),ptoa(new_vol_info));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5PLset_loading_state)(unsigned int plugin_control_mask) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLset_loading_state, (plugin_control_mask));
	char **args = assemble_args_list(1, itoa(plugin_control_mask));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

htri_t WRAPPER_NAME(H5Iis_valid)(hid_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Iis_valid, (id));
	char **args = assemble_args_list(1, itoa(id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_string)(hid_t loc_id, const char *obj_name, const char *attr_name, char *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_string, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),strtoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Screate)(H5S_class_t type) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Screate, (type));
	char **args = assemble_args_list(1, ptoa(&type));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int WRAPPER_NAME(H5DSget_num_scales)(hid_t did, unsigned int idx) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5DSget_num_scales, (did, idx));
	char **args = assemble_args_list(2, itoa(did),itoa(idx));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5EScreate)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5EScreate, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_info)(hid_t loc_id, const char *obj_name, const char *attr_name, hsize_t *dims, H5T_class_t *type_class, size_t *type_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_info, (loc_id, obj_name, attr_name, dims, type_class, type_size));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((dims==NULL)?-1:*dims),ptoa(type_class),itoa((type_size==NULL)?-1:*type_size));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Eclear2)(hid_t err_stack) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eclear2, (err_stack));
	char **args = assemble_args_list(1, itoa(err_stack));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_object_flush_cb)(hid_t plist_id, H5F_flush_cb_t *func, void **udata) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_object_flush_cb, (plist_id, func, udata));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(func),ptoa(udata));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_shared_mesg_nindexes)(hid_t plist_id, unsigned nindexes) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_shared_mesg_nindexes, (plist_id, nindexes));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(nindexes));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Pget_driver)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pget_driver, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Gget_linkval)(hid_t loc_id, const char *name, size_t size, char *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_linkval, (loc_id, name, size, buf));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(size),strtoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_float)(hid_t loc_id, const char *dset_name, float *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_float, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),ftoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Oincr_refcount)(hid_t object_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oincr_refcount, (object_id));
	char **args = assemble_args_list(1, itoa(object_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5PTclose)(hid_t table_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTclose, (table_id));
	char **args = assemble_args_list(1, itoa(table_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_elink_file_cache_size)(hid_t plist_id, unsigned *efc_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_elink_file_cache_size, (plist_id, efc_size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((efc_size==NULL)?-1:*efc_size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBinsert_field)(hid_t loc_id, const char *dset_name, const char *field_name, hid_t field_type, hsize_t position, const void *fill_data, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBinsert_field, (loc_id, dset_name, field_name, field_type, position, fill_data, buf));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(dset_name),strtoa(field_name),itoa(field_type),itoa(position),ptoa(fill_data),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Fincrement_filesize)(hid_t file_id, hsize_t increment) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fincrement_filesize, (file_id, increment));
	char **args = assemble_args_list(2, itoa(file_id),itoa(increment));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_btree_ratios)(hid_t plist_id, double *left, double *middle, double *right) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_btree_ratios, (plist_id, left, middle, right));
	char **args = assemble_args_list(4, itoa(plist_id),ftoa((left==NULL)?-1:*left),ftoa((middle==NULL)?-1:*middle),ftoa((right==NULL)?-1:*right));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Fget_dset_no_attrs_hint)(hid_t file_id, hbool_t *minimize) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_dset_no_attrs_hint, (file_id, minimize));
	char **args = assemble_args_list(2, itoa(file_id),itoa((minimize==NULL)?-1:*minimize));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_elink_fapl)(hid_t lapl_id, hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_elink_fapl, (lapl_id, fapl_id));
	char **args = assemble_args_list(2, itoa(lapl_id),itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5VLregister_connector_by_value)(H5VL_class_value_t connector_value, hid_t vipl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5VLregister_connector_by_value, (connector_value, vipl_id));
	char **args = assemble_args_list(2, ptoa(&connector_value),itoa(vipl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pinsert1)(hid_t plist_id, const char *name, size_t size, void *value, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete, H5P_prp_copy_func_t prp_copy, H5P_prp_close_func_t prp_close) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pinsert1, (plist_id, name, size, value, prp_set, prp_get, prp_delete, prp_copy, prp_close));
	char **args = assemble_args_list(9, itoa(plist_id),strtoa(name),itoa(size),ptoa(value),ptoa(&prp_set),ptoa(&prp_get),ptoa(&prp_delete),ptoa(&prp_copy),ptoa(&prp_close));
	RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

herr_t WRAPPER_NAME(H5Pset_file_space_page_size)(hid_t plist_id, hsize_t fsp_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_file_space_page_size, (plist_id, fsp_size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(fsp_size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int WRAPPER_NAME(H5Pget_nfilters)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Pget_nfilters, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

H5S_sel_type WRAPPER_NAME(H5Sget_select_type)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5S_sel_type, H5Sget_select_type, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Gget_create_plist)(hid_t group_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gget_create_plist, (group_id));
	char **args = assemble_args_list(1, itoa(group_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_ullong)(hid_t loc_id, const char *obj_name, const char *attr_name, const unsigned long long *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_ullong, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Gget_info_by_name)(hid_t loc_id, const char *name, H5G_info_t *ginfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_info_by_name, (loc_id, name, ginfo, lapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),ptoa(ginfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Tcreate)(H5T_class_t type, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tcreate, (type, size));
	char **args = assemble_args_list(2, ptoa(&type),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_long)(hid_t loc_id, const char *dset_name, long *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_long, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),itoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5PTcreate)(hid_t loc_id, const char *dset_name, hid_t dtype_id, hsize_t chunk_size, hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5PTcreate, (loc_id, dset_name, dtype_id, chunk_size, plist_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(dtype_id),itoa(chunk_size),itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Awrite)(hid_t attr_id, hid_t type_id, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Awrite, (attr_id, type_id, buf));
	char **args = assemble_args_list(3, itoa(attr_id),itoa(type_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_ulong)(hid_t loc_id, const char *obj_name, const char *attr_name, unsigned long *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_ulong, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_alloc_time)(hid_t plist_id, H5D_alloc_time_t alloc_time) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_alloc_time, (plist_id, alloc_time));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&alloc_time));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_hyper_vector_size)(hid_t fapl_id, size_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_hyper_vector_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t WRAPPER_NAME(H5VLis_connector_registered_by_value)(H5VL_class_value_t connector_value) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5VLis_connector_registered_by_value, (connector_value));
	char **args = assemble_args_list(1, ptoa(&connector_value));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Dget_space)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_space, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Otoken_to_str)(hid_t loc_id, const H5O_token_t *token, char **token_str) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Otoken_to_str, (loc_id, token, token_str));
	char **args = assemble_args_list(3, itoa(loc_id),ptoa(token),ptoa(token_str));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5DSiterate_scales)(hid_t did, unsigned int dim, int *idx, H5DS_iterate_t visitor, void *visitor_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DSiterate_scales, (did, dim, idx, visitor, visitor_data));
	char **args = assemble_args_list(5, itoa(did),itoa(dim),itoa((idx==NULL)?-1:*idx),ptoa(&visitor),ptoa(visitor_data));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset)(hid_t plist_id, const char *name, const void *value) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset, (plist_id, name, value));
	char **args = assemble_args_list(3, itoa(plist_id),strtoa(name),ptoa(value));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Diterate)(void *buf, hid_t type_id, hid_t space_id, H5D_operator_t op, void *operator_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Diterate, (buf, type_id, space_id, op, operator_data));
	char **args = assemble_args_list(5, ptoa(buf),itoa(type_id),itoa(space_id),ptoa(&op),ptoa(operator_data));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_fletcher32)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fletcher32, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_fapl_family)(hid_t fapl_id, hsize_t *memb_size, hid_t *memb_fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fapl_family, (fapl_id, memb_size, memb_fapl_id));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa((memb_size==NULL)?-1:*memb_size),itoa((memb_fapl_id==NULL)?-1:*memb_fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_attr_creation_order)(hid_t plist_id, unsigned *crt_order_flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_attr_creation_order, (plist_id, crt_order_flags));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((crt_order_flags==NULL)?-1:*crt_order_flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pcopy_prop)(hid_t dst_id, hid_t src_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pcopy_prop, (dst_id, src_id, name));
	char **args = assemble_args_list(3, itoa(dst_id),itoa(src_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Lget_info_by_idx1)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5L_info1_t *linfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_info_by_idx1, (loc_id, group_name, idx_type, order, n, linfo, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(linfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t WRAPPER_NAME(H5Aget_space)(hid_t attr_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aget_space, (attr_id));
	char **args = assemble_args_list(1, itoa(attr_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

ssize_t WRAPPER_NAME(H5Aget_name_by_idx)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, char *name, size_t size, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Aget_name_by_idx, (loc_id, obj_name, idx_type, order, n, name, size, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),itoa(n),strtoa(name),itoa(size),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Dget_space_status)(hid_t dset_id, H5D_space_status_t *allocation) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dget_space_status, (dset_id, allocation));
	char **args = assemble_args_list(2, itoa(dset_id),ptoa(allocation));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Aget_create_plist)(hid_t attr_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aget_create_plist, (attr_id));
	char **args = assemble_args_list(1, itoa(attr_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5ESget_op_counter)(hid_t es_id, uint64_t *counter) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESget_op_counter, (es_id, counter));
	char **args = assemble_args_list(2, itoa(es_id),itoa((counter==NULL)?-1:*counter));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_float)(hid_t loc_id, const char *obj_name, const char *attr_name, const float *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_float, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),ftoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Pget_elink_fapl)(hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pget_elink_fapl, (lapl_id));
	char **args = assemble_args_list(1, itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

H5D_layout_t WRAPPER_NAME(H5Pget_layout)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5D_layout_t, H5Pget_layout, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Freset_page_buffering_stats)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Freset_page_buffering_stats, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_fill_value)(hid_t plist_id, hid_t type_id, void *value) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fill_value, (plist_id, type_id, value));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(type_id),ptoa(value));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_small_data_block_size)(hid_t fapl_id, hsize_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_small_data_block_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hssize_t WRAPPER_NAME(H5Sget_simple_extent_npoints)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Sget_simple_extent_npoints, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Lget_info2)(hid_t loc_id, const char *name, H5L_info2_t *linfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_info2, (loc_id, name, linfo, lapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),ptoa(linfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget_sieve_buf_size)(hid_t fapl_id, size_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_sieve_buf_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Fget_filesize)(hid_t file_id, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_filesize, (file_id, size));
	char **args = assemble_args_list(2, itoa(file_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Lget_info1)(hid_t loc_id, const char *name, H5L_info1_t *linfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_info1, (loc_id, name, linfo, lapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),ptoa(linfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget_mdc_image_config)(hid_t plist_id, H5AC_cache_image_config_t *config_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mdc_image_config, (plist_id, config_ptr));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(config_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5Oget_comment_by_name)(hid_t loc_id, const char *name, char *comment, size_t bufsize, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Oget_comment_by_name, (loc_id, name, comment, bufsize, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),strtoa(comment),itoa(bufsize),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

H5S_class_t WRAPPER_NAME(H5Sget_simple_extent_type)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5S_class_t, H5Sget_simple_extent_type, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_nbit)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_nbit, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_meta_block_size)(hid_t fapl_id, hsize_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_meta_block_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Fget_access_plist)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fget_access_plist, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Dset_extent)(hid_t dset_id, const hsize_t size[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dset_extent, (dset_id, size));
	char **args = assemble_args_list(2, itoa(dset_id),ptoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_char)(hid_t loc_id, const char *obj_name, const char *attr_name, const char *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_char, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),strtoa(buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_sym_k)(hid_t plist_id, unsigned *ik, unsigned *lk) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_sym_k, (plist_id, ik, lk));
	char **args = assemble_args_list(3, itoa(plist_id),itoa((ik==NULL)?-1:*ik),itoa((lk==NULL)?-1:*lk));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

haddr_t WRAPPER_NAME(H5Dget_offset)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(haddr_t, H5Dget_offset, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_ushort)(hid_t loc_id, const char *obj_name, const char *attr_name, unsigned short *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_ushort, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

H5Z_filter_t WRAPPER_NAME(H5Pget_filter2)(hid_t plist_id, unsigned idx, unsigned int *flags, size_t *cd_nelmts, unsigned cd_values[], size_t namelen, char name[], unsigned *filter_config) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5Z_filter_t, H5Pget_filter2, (plist_id, idx, flags, cd_nelmts, cd_values, namelen, name, filter_config));
	char **args = assemble_args_list(8, itoa(plist_id),itoa(idx),itoa((flags==NULL)?-1:*flags),itoa((cd_nelmts==NULL)?-1:*cd_nelmts),ptoa(cd_values),itoa(namelen),ptoa(name),itoa((filter_config==NULL)?-1:*filter_config));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Pget_attr_phase_change)(hid_t plist_id, unsigned *max_compact, unsigned *min_dense) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_attr_phase_change, (plist_id, max_compact, min_dense));
	char **args = assemble_args_list(3, itoa(plist_id),itoa((max_compact==NULL)?-1:*max_compact),itoa((min_dense==NULL)?-1:*min_dense));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_evict_on_close)(hid_t fapl_id, hbool_t *evict_on_close) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_evict_on_close, (fapl_id, evict_on_close));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((evict_on_close==NULL)?-1:*evict_on_close));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_short)(hid_t loc_id, const char *obj_name, const char *attr_name, short *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_short, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_object_flush_cb)(hid_t plist_id, H5F_flush_cb_t func, void *udata) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_object_flush_cb, (plist_id, func, udata));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(&func),ptoa(udata));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Screate_simple)(int rank, const hsize_t dims[], const hsize_t maxdims[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Screate_simple, (rank, dims, maxdims));
	char **args = assemble_args_list(3, itoa(rank),ptoa(dims),ptoa(maxdims));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_fill_time)(hid_t plist_id, H5D_fill_time_t fill_time) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fill_time, (plist_id, fill_time));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&fill_time));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t WRAPPER_NAME(H5Sget_regular_hyperslab)(hid_t spaceid, hsize_t start[], hsize_t stride[], hsize_t count[], hsize_t block[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Sget_regular_hyperslab, (spaceid, start, stride, count, block));
	char **args = assemble_args_list(5, itoa(spaceid),ptoa(start),ptoa(stride),ptoa(count),ptoa(block));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_mdc_log_options)(hid_t plist_id, hbool_t *is_enabled, char *location, size_t *location_size, hbool_t *start_on_access) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mdc_log_options, (plist_id, is_enabled, location, location_size, start_on_access));
	char **args = assemble_args_list(5, itoa(plist_id),itoa((is_enabled==NULL)?-1:*is_enabled),strtoa(location),itoa((location_size==NULL)?-1:*location_size),itoa((start_on_access==NULL)?-1:*start_on_access));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

htri_t WRAPPER_NAME(H5Pisa_class)(hid_t plist_id, hid_t pclass_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Pisa_class, (plist_id, pclass_id));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(pclass_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Ssel_iter_reset)(hid_t sel_iter_id, hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ssel_iter_reset, (sel_iter_id, space_id));
	char **args = assemble_args_list(2, itoa(sel_iter_id),itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Pcreate_class)(hid_t parent, const char *name, H5P_cls_create_func_t create, void *create_data, H5P_cls_copy_func_t copy, void *copy_data, H5P_cls_close_func_t close, void *close_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pcreate_class, (parent, name, create, create_data, copy, copy_data, close, close_data));
	char **args = assemble_args_list(8, itoa(parent),strtoa(name),ptoa(&create),ptoa(create_data),ptoa(&copy),ptoa(copy_data),ptoa(&close),ptoa(close_data));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Tclose)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tclose, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_core_write_tracking)(hid_t fapl_id, hbool_t is_enabled, size_t page_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_core_write_tracking, (fapl_id, is_enabled, page_size));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa(is_enabled),itoa(page_size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Sselect_all)(hid_t spaceid) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_all, (spaceid));
	char **args = assemble_args_list(1, itoa(spaceid));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_short)(hid_t loc_id, const char *dset_name, short *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_short, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),itoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Ewalk1)(H5E_direction_t direction, H5E_walk1_t func, void *client_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ewalk1, (direction, func, client_data));
	char **args = assemble_args_list(3, ptoa(&direction),ptoa(&func),ptoa(client_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Soffset_simple)(hid_t space_id, const hssize_t *offset) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Soffset_simple, (space_id, offset));
	char **args = assemble_args_list(2, itoa(space_id),itoa((offset==NULL)?-1:*offset));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hssize_t WRAPPER_NAME(H5Fget_freespace)(hid_t file_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hssize_t, H5Fget_freespace, (file_id));
	char **args = assemble_args_list(1, itoa(file_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Aget_type)(hid_t attr_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aget_type, (attr_id));
	char **args = assemble_args_list(1, itoa(attr_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_shared_mesg_index)(hid_t plist_id, unsigned index_num, unsigned *mesg_type_flags, unsigned *min_mesg_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_shared_mesg_index, (plist_id, index_num, mesg_type_flags, min_mesg_size));
	char **args = assemble_args_list(4, itoa(plist_id),itoa(index_num),itoa((mesg_type_flags==NULL)?-1:*mesg_type_flags),itoa((min_mesg_size==NULL)?-1:*min_mesg_size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Gget_info)(hid_t loc_id, H5G_info_t *ginfo) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_info, (loc_id, ginfo));
	char **args = assemble_args_list(2, itoa(loc_id),ptoa(ginfo));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5Fget_obj_ids)(hid_t file_id, unsigned types, size_t max_objs, hid_t *obj_id_list) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Fget_obj_ids, (file_id, types, max_objs, obj_id_list));
	char **args = assemble_args_list(4, itoa(file_id),itoa(types),itoa(max_objs),itoa((obj_id_list==NULL)?-1:*obj_id_list));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget_fapl_ros3_token)(hid_t fapl_id, size_t size, char *token) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fapl_ros3_token, (fapl_id, size, token));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa(size),strtoa(token));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Arename)(hid_t loc_id, const char *old_name, const char *new_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Arename, (loc_id, old_name, new_name));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(old_name),strtoa(new_name));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5DSis_scale)(hid_t did) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5DSis_scale, (did));
	char **args = assemble_args_list(1, itoa(did));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5get_libversion)(unsigned *majnum, unsigned *minnum, unsigned *relnum) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5get_libversion, (majnum, minnum, relnum));
	char **args = assemble_args_list(3, itoa((majnum==NULL)?-1:*majnum),itoa((minnum==NULL)?-1:*minnum),itoa((relnum==NULL)?-1:*relnum));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_nlinks)(hid_t plist_id, size_t *nlinks) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_nlinks, (plist_id, nlinks));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((nlinks==NULL)?-1:*nlinks));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Oget_info2)(hid_t loc_id, H5O_info1_t *oinfo, unsigned fields) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info2, (loc_id, oinfo, fields));
	char **args = assemble_args_list(3, itoa(loc_id),ptoa(oinfo),itoa(fields));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_create_intermediate_group)(hid_t plist_id, unsigned crt_intmd) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_create_intermediate_group, (plist_id, crt_intmd));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(crt_intmd));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_file_locking)(hid_t fapl_id, hbool_t *use_file_locking, hbool_t *ignore_when_disabled) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_file_locking, (fapl_id, use_file_locking, ignore_when_disabled));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa((use_file_locking==NULL)?-1:*use_file_locking),itoa((ignore_when_disabled==NULL)?-1:*ignore_when_disabled));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

ssize_t WRAPPER_NAME(H5Pget_virtual_filename)(hid_t dcpl_id, size_t index, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Pget_virtual_filename, (dcpl_id, index, name, size));
	char **args = assemble_args_list(4, itoa(dcpl_id),itoa(index),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_idx3)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_info2_t *oinfo, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_idx3, (loc_id, group_name, idx_type, order, n, oinfo, fields, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(oinfo),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_core)(hid_t fapl_id, size_t increment, hbool_t backing_store) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_core, (fapl_id, increment, backing_store));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa(increment),itoa(backing_store));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_mpi_params)(hid_t fapl_id, MPI_Comm comm, MPI_Info info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_mpi_params, (fapl_id, comm, info));
	char **args = assemble_args_list(3, itoa(fapl_id),ptoa(&comm),ptoa(&info));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Glink2)(hid_t cur_loc_id, const char *cur_name, H5L_type_t type, hid_t new_loc_id, const char *new_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Glink2, (cur_loc_id, cur_name, type, new_loc_id, new_name));
	char **args = assemble_args_list(5, itoa(cur_loc_id),strtoa(cur_name),ptoa(&type),itoa(new_loc_id),strtoa(new_name));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_idx2)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_info1_t *oinfo, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_idx2, (loc_id, group_name, idx_type, order, n, oinfo, fields, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(oinfo),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Ovisit_by_name3)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate2_t op, void *op_data, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ovisit_by_name3, (loc_id, obj_name, idx_type, order, op, op_data, fields, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),ptoa(&op),ptoa(op_data),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Pencode1)(hid_t plist_id, void *buf, size_t *nalloc) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pencode1, (plist_id, buf, nalloc));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(buf),itoa((nalloc==NULL)?-1:*nalloc));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_family)(hid_t fapl_id, hsize_t memb_size, hid_t memb_fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_family, (fapl_id, memb_size, memb_fapl_id));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa(memb_size),itoa(memb_fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LRread_region)(hid_t obj_id, const hdset_reg_ref_t *ref, hid_t mem_type, size_t *numelem, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRread_region, (obj_id, ref, mem_type, numelem, buf));
	char **args = assemble_args_list(5, itoa(obj_id),ptoa(ref),itoa(mem_type),itoa((numelem==NULL)?-1:*numelem),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_elink_prefix)(hid_t plist_id, const char *prefix) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_elink_prefix, (plist_id, prefix));
	char **args = assemble_args_list(2, itoa(plist_id),strtoa(prefix));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_family_offset)(hid_t fapl_id, hsize_t offset) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_family_offset, (fapl_id, offset));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa(offset));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_external)(hid_t plist_id, unsigned idx, size_t name_size, char *name, off_t *offset, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_external, (plist_id, idx, name_size, name, offset, size));
	char **args = assemble_args_list(6, itoa(plist_id),itoa(idx),itoa(name_size),strtoa(name),ptoa(offset),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5VLobject_is_native)(hid_t obj_id, hbool_t *is_native) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5VLobject_is_native, (obj_id, is_native));
	char **args = assemble_args_list(2, itoa(obj_id),itoa((is_native==NULL)?-1:*is_native));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBwrite_fields_index)(hid_t loc_id, const char *dset_name, hsize_t nfields, const int *field_index, hsize_t start, hsize_t nrecords, size_t type_size, const size_t *field_offset, const size_t *dst_sizes, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBwrite_fields_index, (loc_id, dset_name, nfields, field_index, start, nrecords, type_size, field_offset, dst_sizes, buf));
	char **args = assemble_args_list(10, itoa(loc_id),strtoa(dset_name),itoa(nfields),itoa((field_index==NULL)?-1:*field_index),itoa(start),itoa(nrecords),itoa(type_size),itoa((field_offset==NULL)?-1:*field_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(10, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_char)(hid_t loc_id, const char *dset_name, char *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_char, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),strtoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_sym_k)(hid_t plist_id, unsigned ik, unsigned lk) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_sym_k, (plist_id, ik, lk));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(ik),itoa(lk));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5DSwith_new_ref)(hid_t obj_id, hbool_t *with_new_ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DSwith_new_ref, (obj_id, with_new_ref));
	char **args = assemble_args_list(2, itoa(obj_id),itoa((with_new_ref==NULL)?-1:*with_new_ref));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_relax_file_integrity_checks)(hid_t plist_id, uint64_t *flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_relax_file_integrity_checks, (plist_id, flags));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((flags==NULL)?-1:*flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Tlock)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tlock, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5VLunregister_connector)(hid_t connector_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5VLunregister_connector, (connector_id));
	char **args = assemble_args_list(1, itoa(connector_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_istore_k)(hid_t plist_id, unsigned *ik) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_istore_k, (plist_id, ik));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((ik==NULL)?-1:*ik));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

htri_t WRAPPER_NAME(H5VLis_connector_registered_by_name)(const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5VLis_connector_registered_by_name, (name));
	char **args = assemble_args_list(1, strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Fget_page_buffering_stats)(hid_t file_id, unsigned accesses[2], unsigned hits[2], unsigned misses[2], unsigned evictions[2], unsigned bypasses[2]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_page_buffering_stats, (file_id, accesses, hits, misses, evictions, bypasses));
	char **args = assemble_args_list(6, itoa(file_id),ptoa(accesses),ptoa(hits),ptoa(misses),ptoa(evictions),ptoa(bypasses));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_ushort)(hid_t loc_id, const char *obj_name, const char *attr_name, const unsigned short *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_ushort, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_fapl_core)(hid_t fapl_id, size_t *increment, hbool_t *backing_store) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fapl_core, (fapl_id, increment, backing_store));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa((increment==NULL)?-1:*increment),itoa((backing_store==NULL)?-1:*backing_store));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5IMget_image_info)(hid_t loc_id, const char *dset_name, hsize_t *width, hsize_t *height, hsize_t *planes, char *interlace, hssize_t *npals) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMget_image_info, (loc_id, dset_name, width, height, planes, interlace, npals));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(dset_name),itoa((width==NULL)?-1:*width),itoa((height==NULL)?-1:*height),itoa((planes==NULL)?-1:*planes),strtoa(interlace),itoa((npals==NULL)?-1:*npals));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5ESget_count)(hid_t es_id, size_t *count) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESget_count, (es_id, count));
	char **args = assemble_args_list(2, itoa(es_id),itoa((count==NULL)?-1:*count));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

size_t WRAPPER_NAME(H5Tget_size)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(size_t, H5Tget_size, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_metadata_read_attempts)(hid_t plist_id, unsigned attempts) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_metadata_read_attempts, (plist_id, attempts));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(attempts));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_file_locking)(hid_t fapl_id, hbool_t use_file_locking, hbool_t ignore_when_disabled) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_file_locking, (fapl_id, use_file_locking, ignore_when_disabled));
	char **args = assemble_args_list(3, itoa(fapl_id),itoa(use_file_locking),itoa(ignore_when_disabled));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Arename_by_name)(hid_t loc_id, const char *obj_name, const char *old_attr_name, const char *new_attr_name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Arename_by_name, (loc_id, obj_name, old_attr_name, new_attr_name, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(old_attr_name),strtoa(new_attr_name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

htri_t WRAPPER_NAME(H5Aexists_by_name)(hid_t obj_id, const char *obj_name, const char *attr_name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Aexists_by_name, (obj_id, obj_name, attr_name, lapl_id));
	char **args = assemble_args_list(4, itoa(obj_id),strtoa(obj_name),strtoa(attr_name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

char * WRAPPER_NAME(H5Eget_major)(H5E_major_t maj) {
	RECORDER_INTERCEPTOR_PROLOGUE(char *, H5Eget_major, (maj));
	char **args = assemble_args_list(1, ptoa(&maj));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pencode2)(hid_t plist_id, void *buf, size_t *nalloc, hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pencode2, (plist_id, buf, nalloc, fapl_id));
	char **args = assemble_args_list(4, itoa(plist_id),ptoa(buf),itoa((nalloc==NULL)?-1:*nalloc),itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_filter)(hid_t plist_id, H5Z_filter_t filter, unsigned int flags, size_t cd_nelmts, const unsigned int c_values[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_filter, (plist_id, filter, flags, cd_nelmts, c_values));
	char **args = assemble_args_list(5, itoa(plist_id),ptoa(&filter),itoa(flags),itoa(cd_nelmts),ptoa(c_values));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Sset_extent_none)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sset_extent_none, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Gopen2)(hid_t loc_id, const char *name, hid_t gapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gopen2, (loc_id, name, gapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(gapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Tcommit2)(hid_t loc_id, const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tcommit2, (loc_id, name, type_id, lcpl_id, tcpl_id, tapl_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(name),itoa(type_id),itoa(lcpl_id),itoa(tcpl_id),itoa(tapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Dread_multi)(size_t count, hid_t dset_id[], hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[], hid_t dxpl_id, void *buf[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dread_multi, (count, dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf));
	char **args = assemble_args_list(7, itoa(count),ptoa(dset_id),ptoa(mem_type_id),ptoa(mem_space_id),ptoa(file_space_id),itoa(dxpl_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

htri_t WRAPPER_NAME(H5Aexists)(hid_t obj_id, const char *attr_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Aexists, (obj_id, attr_name));
	char **args = assemble_args_list(2, itoa(obj_id),strtoa(attr_name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_long_long)(hid_t loc_id, const char *obj_name, const char *attr_name, long long *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_long_long, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Dscatter)(H5D_scatter_func_t op, void *op_data, hid_t type_id, hid_t dst_space_id, void *dst_buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dscatter, (op, op_data, type_id, dst_space_id, dst_buf));
	char **args = assemble_args_list(5, ptoa(&op),ptoa(op_data),itoa(type_id),itoa(dst_space_id),ptoa(dst_buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Fset_dset_no_attrs_hint)(hid_t file_id, hbool_t minimize) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fset_dset_no_attrs_hint, (file_id, minimize));
	char **args = assemble_args_list(2, itoa(file_id),itoa(minimize));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_elink_cb)(hid_t lapl_id, H5L_elink_traverse_t func, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_elink_cb, (lapl_id, func, op_data));
	char **args = assemble_args_list(3, itoa(lapl_id),ptoa(&func),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_file_space_strategy)(hid_t plist_id, H5F_fspace_strategy_t *strategy, hbool_t *persist, hsize_t *threshold) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_file_space_strategy, (plist_id, strategy, persist, threshold));
	char **args = assemble_args_list(4, itoa(plist_id),ptoa(strategy),itoa((persist==NULL)?-1:*persist),itoa((threshold==NULL)?-1:*threshold));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Premove_filter)(hid_t plist_id, H5Z_filter_t filter) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Premove_filter, (plist_id, filter));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&filter));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Dget_chunk_storage_size)(hid_t dset_id, const hsize_t *offset, hsize_t *chunk_bytes) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dget_chunk_storage_size, (dset_id, offset, chunk_bytes));
	char **args = assemble_args_list(3, itoa(dset_id),itoa((offset==NULL)?-1:*offset),itoa((chunk_bytes==NULL)?-1:*chunk_bytes));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_name1)(hid_t loc_id, const char *name, H5O_info1_t *oinfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_name1, (loc_id, name, oinfo, lapl_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),ptoa(oinfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5PTis_varlen)(hid_t table_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTis_varlen, (table_id));
	char **args = assemble_args_list(1, itoa(table_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_fclose_degree)(hid_t fapl_id, H5F_close_degree_t degree) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fclose_degree, (fapl_id, degree));
	char **args = assemble_args_list(2, itoa(fapl_id),ptoa(&degree));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LRget_region_info)(hid_t obj_id, const hdset_reg_ref_t *ref, size_t *len, char *path, int *rank, hid_t *dtype, H5S_sel_type *sel_type, size_t *numelem, hsize_t *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRget_region_info, (obj_id, ref, len, path, rank, dtype, sel_type, numelem, buf));
	char **args = assemble_args_list(9, itoa(obj_id),ptoa(ref),itoa((len==NULL)?-1:*len),strtoa(path),itoa((rank==NULL)?-1:*rank),itoa((dtype==NULL)?-1:*dtype),ptoa(sel_type),itoa((numelem==NULL)?-1:*numelem),itoa((buf==NULL)?-1:*buf));
	RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

htri_t WRAPPER_NAME(H5LTpath_valid)(hid_t loc_id, const char *path, hbool_t check_object_valid) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5LTpath_valid, (loc_id, path, check_object_valid));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(path),itoa(check_object_valid));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Topen_async)(hid_t loc_id, const char *name, hid_t tapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Topen_async, (loc_id, name, tapl_id, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(tapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_mcdt_search_cb)(hid_t plist_id, H5O_mcdt_search_cb_t func, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_mcdt_search_cb, (plist_id, func, op_data));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(&func),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Zget_filter_info)(H5Z_filter_t filter, unsigned int *filter_config_flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Zget_filter_info, (filter, filter_config_flags));
	char **args = assemble_args_list(2, ptoa(&filter),itoa((filter_config_flags==NULL)?-1:*filter_config_flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_fapl_direct)(hid_t fapl_id, size_t *boundary, size_t *block_size, size_t *cbuf_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fapl_direct, (fapl_id, boundary, block_size, cbuf_size));
	char **args = assemble_args_list(4, itoa(fapl_id),itoa((boundary==NULL)?-1:*boundary),itoa((block_size==NULL)?-1:*block_size),itoa((cbuf_size==NULL)?-1:*cbuf_size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

ssize_t WRAPPER_NAME(H5VLget_connector_name)(hid_t id, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5VLget_connector_name, (id, name, size));
	char **args = assemble_args_list(3, itoa(id),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_name2)(hid_t loc_id, const char *name, H5O_info1_t *oinfo, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_name2, (loc_id, name, oinfo, fields, lapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),ptoa(oinfo),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Tget_super)(hid_t type) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tget_super, (type));
	char **args = assemble_args_list(1, itoa(type));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Eget_auto1)(H5E_auto1_t *func, void **client_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eget_auto1, (func, client_data));
	char **args = assemble_args_list(2, ptoa(func),ptoa(client_data));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Pget_virtual_vspace)(hid_t dcpl_id, size_t index) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pget_virtual_vspace, (dcpl_id, index));
	char **args = assemble_args_list(2, itoa(dcpl_id),itoa(index));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Ovisit2)(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate1_t op, void *op_data, unsigned fields) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ovisit2, (obj_id, idx_type, order, op, op_data, fields));
	char **args = assemble_args_list(6, itoa(obj_id),itoa(idx_type),itoa(order),ptoa(&op),ptoa(op_data),itoa(fields));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

hid_t WRAPPER_NAME(H5Gcreate2)(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gcreate2, (loc_id, name, lcpl_id, gcpl_id, gapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),itoa(lcpl_id),itoa(gcpl_id),itoa(gapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5PTis_valid)(hid_t table_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTis_valid, (table_id));
	char **args = assemble_args_list(1, itoa(table_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Dvlen_reclaim)(hid_t type_id, hid_t space_id, hid_t dxpl_id, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dvlen_reclaim, (type_id, space_id, dxpl_id, buf));
	char **args = assemble_args_list(4, itoa(type_id),itoa(space_id),itoa(dxpl_id),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5VLregister_connector_by_name)(const char *connector_name, hid_t vipl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5VLregister_connector_by_name, (connector_name, vipl_id));
	char **args = assemble_args_list(2, strtoa(connector_name),itoa(vipl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_double)(hid_t loc_id, const char *dset_name, double *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_double, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),ftoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Iget_file_id)(hid_t id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Iget_file_id, (id));
	char **args = assemble_args_list(1, itoa(id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Dchunk_iter)(hid_t dset_id, hid_t dxpl_id, H5D_chunk_iter_op_t cb, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dchunk_iter, (dset_id, dxpl_id, cb, op_data));
	char **args = assemble_args_list(4, itoa(dset_id),itoa(dxpl_id),ptoa(&cb),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Ldelete)(hid_t loc_id, const char *name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ldelete, (loc_id, name, lapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5IMread_image)(hid_t loc_id, const char *dset_name, unsigned char *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMread_image, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),strtoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Gflush)(hid_t group_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gflush, (group_id));
	char **args = assemble_args_list(1, itoa(group_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5PTopen)(hid_t loc_id, const char *dset_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5PTopen, (loc_id, dset_name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(dset_name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Gcreate_anon)(hid_t loc_id, hid_t gcpl_id, hid_t gapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gcreate_anon, (loc_id, gcpl_id, gapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),itoa(gcpl_id),itoa(gapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5PTget_index)(hid_t table_id, hsize_t *pt_index) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTget_index, (table_id, pt_index));
	char **args = assemble_args_list(2, itoa(table_id),itoa((pt_index==NULL)?-1:*pt_index));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LRcreate_ref_to_all)(hid_t loc_id, const char *group_path, const char *ds_path, H5_index_t index_type, H5_iter_order_t order, H5R_type_t ref_type) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRcreate_ref_to_all, (loc_id, group_path, ds_path, index_type, order, ref_type));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(group_path),strtoa(ds_path),itoa(index_type),itoa(order),ptoa(&ref_type));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5TBread_records)(hid_t loc_id, const char *dset_name, hsize_t start, hsize_t nrecords, size_t type_size, const size_t *dst_offset, const size_t *dst_sizes, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBread_records, (loc_id, dset_name, start, nrecords, type_size, dst_offset, dst_sizes, buf));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(dset_name),itoa(start),itoa(nrecords),itoa(type_size),itoa((dst_offset==NULL)?-1:*dst_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

hid_t WRAPPER_NAME(H5Pcreate)(hid_t cls_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pcreate, (cls_id));
	char **args = assemble_args_list(1, itoa(cls_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_page_buffer_size)(hid_t plist_id, size_t *buf_size, unsigned *min_meta_perc, unsigned *min_raw_perc) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_page_buffer_size, (plist_id, buf_size, min_meta_perc, min_raw_perc));
	char **args = assemble_args_list(4, itoa(plist_id),itoa((buf_size==NULL)?-1:*buf_size),itoa((min_meta_perc==NULL)?-1:*min_meta_perc),itoa((min_raw_perc==NULL)?-1:*min_raw_perc));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_gc_references)(hid_t fapl_id, unsigned gc_ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_gc_references, (fapl_id, gc_ref));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa(gc_ref));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5close)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5close, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5Fdelete)(const char *filename, hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fdelete, (filename, fapl_id));
	char **args = assemble_args_list(2, strtoa(filename),itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Sclose)(hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sclose, (space_id));
	char **args = assemble_args_list(1, itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Oflush)(hid_t obj_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oflush, (obj_id));
	char **args = assemble_args_list(1, itoa(obj_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_file_space)(hid_t plist_id, H5F_file_space_type_t *strategy, hsize_t *threshold) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_file_space, (plist_id, strategy, threshold));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(strategy),itoa((threshold==NULL)?-1:*threshold));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5DSset_label)(hid_t did, unsigned int idx, const char *label) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DSset_label, (did, idx, label));
	char **args = assemble_args_list(3, itoa(did),itoa(idx),strtoa(label));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Aopen)(hid_t obj_id, const char *attr_name, hid_t aapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen, (obj_id, attr_name, aapl_id));
	char **args = assemble_args_list(3, itoa(obj_id),strtoa(attr_name),itoa(aapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Sset_extent_simple)(hid_t space_id, int rank, const hsize_t dims[], const hsize_t max[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sset_extent_simple, (space_id, rank, dims, max));
	char **args = assemble_args_list(4, itoa(space_id),itoa(rank),ptoa(dims),ptoa(max));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pinsert2)(hid_t plist_id, const char *name, size_t size, void *value, H5P_prp_set_func_t set, H5P_prp_get_func_t get, H5P_prp_delete_func_t prp_del, H5P_prp_copy_func_t copy, H5P_prp_compare_func_t compare, H5P_prp_close_func_t close) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pinsert2, (plist_id, name, size, value, set, get, prp_del, copy, compare, close));
	char **args = assemble_args_list(10, itoa(plist_id),strtoa(name),itoa(size),ptoa(value),ptoa(&set),ptoa(&get),ptoa(&prp_del),ptoa(&copy),ptoa(&compare),ptoa(&close));
	RECORDER_INTERCEPTOR_EPILOGUE(10, args);
}

herr_t WRAPPER_NAME(H5PTset_index)(hid_t table_id, hsize_t pt_index) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTset_index, (table_id, pt_index));
	char **args = assemble_args_list(2, itoa(table_id),itoa(pt_index));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Dgather)(hid_t src_space_id, const void *src_buf, hid_t type_id, size_t dst_buf_size, void *dst_buf, H5D_gather_func_t op, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dgather, (src_space_id, src_buf, type_id, dst_buf_size, dst_buf, op, op_data));
	char **args = assemble_args_list(7, itoa(src_space_id),ptoa(src_buf),itoa(type_id),itoa(dst_buf_size),ptoa(dst_buf),ptoa(&op),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Pget_fapl_mpio)(hid_t fapl_id, MPI_Comm *comm, MPI_Info *info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_fapl_mpio, (fapl_id, comm, info));
	char **args = assemble_args_list(3, itoa(fapl_id),ptoa(comm),ptoa(info));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_dxpl_mpio_chunk_opt_num)(hid_t dxpl_id, unsigned num_chunk_per_proc) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_dxpl_mpio_chunk_opt_num, (dxpl_id, num_chunk_per_proc));
	char **args = assemble_args_list(2, itoa(dxpl_id),itoa(num_chunk_per_proc));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTread_region)(const char *file, const char *path, const hsize_t *block_coord, hid_t mem_type, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_region, (file, path, block_coord, mem_type, buf));
	char **args = assemble_args_list(5, strtoa(file),strtoa(path),itoa((block_coord==NULL)?-1:*block_coord),itoa(mem_type),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Sextent_copy)(hid_t dst_id, hid_t src_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sextent_copy, (dst_id, src_id));
	char **args = assemble_args_list(2, itoa(dst_id),itoa(src_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Lget_info_by_idx2)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5L_info2_t *linfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lget_info_by_idx2, (loc_id, group_name, idx_type, order, n, linfo, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(linfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Sget_select_bounds)(hid_t spaceid, hsize_t start[], hsize_t end[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sget_select_bounds, (spaceid, start, end));
	char **args = assemble_args_list(3, itoa(spaceid),ptoa(start),ptoa(end));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_filter_by_id1)(hid_t plist_id, H5Z_filter_t id, unsigned int *flags, size_t *cd_nelmts, unsigned cd_values[], size_t namelen, char name[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_filter_by_id1, (plist_id, id, flags, cd_nelmts, cd_values, namelen, name));
	char **args = assemble_args_list(7, itoa(plist_id),ptoa(&id),itoa((flags==NULL)?-1:*flags),itoa((cd_nelmts==NULL)?-1:*cd_nelmts),ptoa(cd_values),itoa(namelen),ptoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t WRAPPER_NAME(H5Aopen_by_idx)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t aapl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_by_idx, (loc_id, obj_name, idx_type, order, n, aapl_id, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),itoa(n),itoa(aapl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_long)(hid_t loc_id, const char *obj_name, const char *attr_name, const long *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_long, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_shared_mesg_phase_change)(hid_t plist_id, unsigned *max_list, unsigned *min_btree) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_shared_mesg_phase_change, (plist_id, max_list, min_btree));
	char **args = assemble_args_list(3, itoa(plist_id),itoa((max_list==NULL)?-1:*max_list),itoa((min_btree==NULL)?-1:*min_btree));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5PLget_loading_state)(unsigned int *plugin_control_mask) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLget_loading_state, (plugin_control_mask));
	char **args = assemble_args_list(1, itoa((plugin_control_mask==NULL)?-1:*plugin_control_mask));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_data_transform)(hid_t plist_id, const char *expression) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_data_transform, (plist_id, expression));
	char **args = assemble_args_list(2, itoa(plist_id),strtoa(expression));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eclear1)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eclear1, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5Pset_elink_acc_flags)(hid_t lapl_id, unsigned flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_elink_acc_flags, (lapl_id, flags));
	char **args = assemble_args_list(2, itoa(lapl_id),itoa(flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Drefresh)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Drefresh, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Odisable_mdc_flushes)(hid_t object_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Odisable_mdc_flushes, (object_id));
	char **args = assemble_args_list(1, itoa(object_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_float)(hid_t loc_id, const char *obj_name, const char *attr_name, float *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_float, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),ftoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_int)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, const int *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_int, (loc_id, dset_name, rank, dims, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),itoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Pdecode)(const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Pdecode, (buf));
	char **args = assemble_args_list(1, ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_uchar)(hid_t loc_id, const char *obj_name, const char *attr_name, unsigned char *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_uchar, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),strtoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Fget_info2)(hid_t obj_id, H5F_info2_t *file_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_info2, (obj_id, file_info));
	char **args = assemble_args_list(2, itoa(obj_id),ptoa(file_info));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Oopen_by_idx)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen_by_idx, (loc_id, group_name, idx_type, order, n, lapl_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Gget_info_by_idx)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5G_info_t *ginfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_info_by_idx, (loc_id, group_name, idx_type, order, n, ginfo, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(ginfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_ndims)(hid_t loc_id, const char *obj_name, const char *attr_name, int *rank) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_ndims, (loc_id, obj_name, attr_name, rank));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((rank==NULL)?-1:*rank));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5DOappend)(hid_t dset_id, hid_t dxpl_id, unsigned axis, size_t extension, hid_t memtype, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DOappend, (dset_id, dxpl_id, axis, extension, memtype, buf));
	char **args = assemble_args_list(6, itoa(dset_id),itoa(dxpl_id),itoa(axis),itoa(extension),itoa(memtype),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Tclose_async)(hid_t type_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Tclose_async, (type_id, es_id));
	char **args = assemble_args_list(2, itoa(type_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_int)(hid_t loc_id, const char *dset_name, int *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_int, (loc_id, dset_name, buffer));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),itoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Rdereference1)(hid_t obj_id, H5R_type_t ref_type, const void *ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Rdereference1, (obj_id, ref_type, ref));
	char **args = assemble_args_list(3, itoa(obj_id),ptoa(&ref_type),ptoa(ref));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LDget_dset_dims)(hid_t did, hsize_t *cur_dims) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LDget_dset_dims, (did, cur_dims));
	char **args = assemble_args_list(2, itoa(did),itoa((cur_dims==NULL)?-1:*cur_dims));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Sget_select_hyper_blocklist)(hid_t spaceid, hsize_t startblock, hsize_t numblocks, hsize_t buf[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sget_select_hyper_blocklist, (spaceid, startblock, numblocks, buf));
	char **args = assemble_args_list(4, itoa(spaceid),itoa(startblock),itoa(numblocks),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

const void * WRAPPER_NAME(H5Pget_driver_info)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(const void *, H5Pget_driver_info, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Sencode2)(hid_t obj_id, void *buf, size_t *nalloc, hid_t fapl) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sencode2, (obj_id, buf, nalloc, fapl));
	char **args = assemble_args_list(4, itoa(obj_id),ptoa(buf),itoa((nalloc==NULL)?-1:*nalloc),itoa(fapl));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Lcreate_ud)(hid_t link_loc_id, const char *link_name, H5L_type_t link_type, const void *udata, size_t udata_size, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcreate_ud, (link_loc_id, link_name, link_type, udata, udata_size, lcpl_id, lapl_id));
	char **args = assemble_args_list(7, itoa(link_loc_id),strtoa(link_name),ptoa(&link_type),ptoa(udata),itoa(udata_size),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t WRAPPER_NAME(H5Tcopy)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tcopy, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hsize_t WRAPPER_NAME(H5Dget_storage_size)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hsize_t, H5Dget_storage_size, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Dopen1)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dopen1, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_elink_cb)(hid_t lapl_id, H5L_elink_traverse_t *func, void **op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_elink_cb, (lapl_id, func, op_data));
	char **args = assemble_args_list(3, itoa(lapl_id),ptoa(func),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_char)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, const char *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_char, (loc_id, dset_name, rank, dims, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),strtoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_meta_block_size)(hid_t fapl_id, hsize_t *size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_meta_block_size, (fapl_id, size));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((size==NULL)?-1:*size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eappend_stack)(hid_t dst_stack_id, hid_t src_stack_id, hbool_t close_source_stack) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eappend_stack, (dst_stack_id, src_stack_id, close_source_stack));
	char **args = assemble_args_list(3, itoa(dst_stack_id),itoa(src_stack_id),itoa(close_source_stack));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_nlinks)(hid_t plist_id, size_t nlinks) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_nlinks, (plist_id, nlinks));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(nlinks));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_copy_object)(hid_t plist_id, unsigned copy_options) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_copy_object, (plist_id, copy_options));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(copy_options));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Smodify_select)(hid_t space1_id, H5S_seloper_t op, hid_t space2_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Smodify_select, (space1_id, op, space2_id));
	char **args = assemble_args_list(3, itoa(space1_id),ptoa(&op),itoa(space2_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_shared_mesg_nindexes)(hid_t plist_id, unsigned *nindexes) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_shared_mesg_nindexes, (plist_id, nindexes));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((nindexes==NULL)?-1:*nindexes));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Lcreate_soft)(const char *link_target, hid_t link_loc_id, const char *link_name, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcreate_soft, (link_target, link_loc_id, link_name, lcpl_id, lapl_id));
	char **args = assemble_args_list(5, strtoa(link_target),itoa(link_loc_id),strtoa(link_name),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

ssize_t WRAPPER_NAME(H5DSget_scale_name)(hid_t did, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5DSget_scale_name, (did, name, size));
	char **args = assemble_args_list(3, itoa(did),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_mdc_config)(hid_t plist_id, H5AC_cache_config_t *config_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_mdc_config, (plist_id, config_ptr));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(config_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTfind_attribute)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTfind_attribute, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5TBdelete_record)(hid_t loc_id, const char *dset_name, hsize_t start, hsize_t nrecords) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBdelete_record, (loc_id, dset_name, start, nrecords));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(dset_name),itoa(start),itoa(nrecords));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

char * WRAPPER_NAME(H5Pget_class_name)(hid_t pclass_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(char *, H5Pget_class_name, (pclass_id));
	char **args = assemble_args_list(1, itoa(pclass_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pset_sizes)(hid_t plist_id, size_t sizeof_addr, size_t sizeof_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_sizes, (plist_id, sizeof_addr, sizeof_size));
	char **args = assemble_args_list(3, itoa(plist_id),itoa(sizeof_addr),itoa(sizeof_size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_mpio)(hid_t fapl_id, MPI_Comm comm, MPI_Info info) {
	// if (info != MPI_INFO_NULL){
    //     int nkeys;
    //     MPI_Info_get_nkeys(info, &nkeys);  // Get the number of keys in the MPI_Info object

    //     printf("MPI_Info contains %d key(s):\n", nkeys);

    //     for (int i = 0; i < nkeys; i++) {
    //         char key[MPI_MAX_INFO_KEY];
    //         char value[MPI_MAX_INFO_VAL];
    //         int flag;

    //         // Get the i-th key
    //         MPI_Info_get_nthkey(info, i, key);

    //         // Get the value associated with the key
    //         MPI_Info_get(info, key, MPI_MAX_INFO_VAL, value, &flag);

    //         if (flag) {
    //             printf("  Key: %s, Value: %s\n", key, value);
    //         } else {
    //             printf("  Key: %s has no associated value.\n", key);
    //         }
    //     }
    // }
	RECORDER_INTERCEPTOR_PROLOGUE_WITH_MPI_INFO(herr_t, H5Pset_fapl_mpio, (fapl_id, comm, info), info);
	char **args = assemble_args_list(3, itoa(fapl_id),ptoa(&comm),ptoa(&info));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);

	// int real_arg_count = 1;
    // void** real_args = alloca(sizeof(void*)*real_arg_count);
    // real_args[0] = (void*) &info;
    // RECORDER_INTERCEPTOR_EPILOGUE_WITH_REAL_ARGS(3, args, real_arg_count, real_args);
}

herr_t WRAPPER_NAME(H5Pget_all_coll_metadata_ops)(hid_t plist_id, hbool_t *is_collective) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_all_coll_metadata_ops, (plist_id, is_collective));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((is_collective==NULL)?-1:*is_collective));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Funmount)(hid_t loc, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Funmount, (loc, name));
	char **args = assemble_args_list(2, itoa(loc),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eset_auto1)(H5E_auto1_t func, void *client_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eset_auto1, (func, client_data));
	char **args = assemble_args_list(2, ptoa(&func),ptoa(client_data));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_char_encoding)(hid_t plist_id, H5T_cset_t encoding) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_char_encoding, (plist_id, encoding));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&encoding));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_version)(hid_t plist_id, unsigned *boot, unsigned *freelist, unsigned *stab, unsigned *shhdr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_version, (plist_id, boot, freelist, stab, shhdr));
	char **args = assemble_args_list(5, itoa(plist_id),itoa((boot==NULL)?-1:*boot),itoa((freelist==NULL)?-1:*freelist),itoa((stab==NULL)?-1:*stab),itoa((shhdr==NULL)?-1:*shhdr));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

H5G_obj_t WRAPPER_NAME(H5Gget_objtype_by_idx)(hid_t loc_id, hsize_t idx) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5G_obj_t, H5Gget_objtype_by_idx, (loc_id, idx));
	char **args = assemble_args_list(2, itoa(loc_id),itoa(idx));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5is_library_terminating)(hbool_t *is_terminating) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5is_library_terminating, (is_terminating));
	char **args = assemble_args_list(1, itoa((is_terminating==NULL)?-1:*is_terminating));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5IMget_npalettes)(hid_t loc_id, const char *image_name, hssize_t *npals) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMget_npalettes, (loc_id, image_name, npals));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(image_name),itoa((npals==NULL)?-1:*npals));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5Fis_accessible)(const char *container_name, hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Fis_accessible, (container_name, fapl_id));
	char **args = assemble_args_list(2, strtoa(container_name),itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5open)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5open, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5ESget_err_status)(hid_t es_id, hbool_t *err_occurred) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESget_err_status, (es_id, err_occurred));
	char **args = assemble_args_list(2, itoa(es_id),itoa((err_occurred==NULL)?-1:*err_occurred));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_mdc_config)(hid_t plist_id, H5AC_cache_config_t *config_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_mdc_config, (plist_id, config_ptr));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(config_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Oset_comment)(hid_t obj_id, const char *comment) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oset_comment, (obj_id, comment));
	char **args = assemble_args_list(2, itoa(obj_id),strtoa(comment));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_char)(hid_t loc_id, const char *obj_name, const char *attr_name, char *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_char, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),strtoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_sec2)(hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_sec2, (fapl_id));
	char **args = assemble_args_list(1, itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Pget_type_conv_cb)(hid_t dxpl_id, H5T_conv_except_func_t *op, void **operate_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_type_conv_cb, (dxpl_id, op, operate_data));
	char **args = assemble_args_list(3, itoa(dxpl_id),ptoa(op),ptoa(operate_data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5PLappend)(const char *search_path) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLappend, (search_path));
	char **args = assemble_args_list(1, strtoa(search_path));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Sselect_elements)(hid_t space_id, H5S_seloper_t op, size_t num_elem, const hsize_t *coord) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Sselect_elements, (space_id, op, num_elem, coord));
	char **args = assemble_args_list(4, itoa(space_id),ptoa(&op),itoa(num_elem),itoa((coord==NULL)?-1:*coord));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Fopen)(const char *filename, unsigned flags, hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fopen, (filename, flags, fapl_id));
	char **args = assemble_args_list(3, strtoa(filename),itoa(flags),itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Ocopy)(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id, const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ocopy, (src_loc_id, src_name, dst_loc_id, dst_name, ocpypl_id, lcpl_id));
	char **args = assemble_args_list(6, itoa(src_loc_id),strtoa(src_name),itoa(dst_loc_id),strtoa(dst_name),itoa(ocpypl_id),itoa(lcpl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5DOread_chunk)(hid_t dset_id, hid_t dxpl_id, const hsize_t *offset, uint32_t *filters, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DOread_chunk, (dset_id, dxpl_id, offset, filters, buf));
	char **args = assemble_args_list(5, itoa(dset_id),itoa(dxpl_id),itoa((offset==NULL)?-1:*offset),itoa((filters==NULL)?-1:*filters),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Aget_info)(hid_t attr_id, H5A_info_t *ainfo) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aget_info, (attr_id, ainfo));
	char **args = assemble_args_list(2, itoa(attr_id),ptoa(ainfo));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_chunk_opts)(hid_t plist_id, unsigned *opts) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_chunk_opts, (plist_id, opts));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((opts==NULL)?-1:*opts));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Aiterate_by_name)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t *idx, H5A_operator2_t op, void *op_data, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aiterate_by_name, (loc_id, obj_name, idx_type, order, idx, op, op_data, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),itoa((idx==NULL)?-1:*idx),ptoa(&op),ptoa(op_data),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5LTmake_dataset_long)(hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims, const long *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTmake_dataset_long, (loc_id, dset_name, rank, dims, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(rank),itoa((dims==NULL)?-1:*dims),itoa((buffer==NULL)?-1:*buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pset_fapl_stdio)(hid_t fapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_fapl_stdio, (fapl_id));
	char **args = assemble_args_list(1, itoa(fapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5TBread_fields_name)(hid_t loc_id, const char *dset_name, const char *field_names, hsize_t start, hsize_t nrecords, size_t type_size, const size_t *field_offset, const size_t *dst_sizes, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBread_fields_name, (loc_id, dset_name, field_names, start, nrecords, type_size, field_offset, dst_sizes, buf));
	char **args = assemble_args_list(9, itoa(loc_id),strtoa(dset_name),strtoa(field_names),itoa(start),itoa(nrecords),itoa(type_size),itoa((field_offset==NULL)?-1:*field_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

int WRAPPER_NAME(H5Gget_comment)(hid_t loc_id, const char *name, size_t bufsize, char *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Gget_comment, (loc_id, name, bufsize, buf));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(bufsize),strtoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Oopen)(hid_t loc_id, const char *name, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen, (loc_id, name, lapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Lmove)(hid_t src_loc, const char *src_name, hid_t dst_loc, const char *dst_name, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lmove, (src_loc, src_name, dst_loc, dst_name, lcpl_id, lapl_id));
	char **args = assemble_args_list(6, itoa(src_loc),strtoa(src_name),itoa(dst_loc),strtoa(dst_name),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Trefresh)(hid_t type_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Trefresh, (type_id));
	char **args = assemble_args_list(1, itoa(type_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Eunregister_class)(hid_t class_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eunregister_class, (class_id));
	char **args = assemble_args_list(1, itoa(class_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Eget_current_stack)(void) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Eget_current_stack, ());
	char **args = NULL;
	RECORDER_INTERCEPTOR_EPILOGUE(0, args);
}

herr_t WRAPPER_NAME(H5Glink)(hid_t cur_loc_id, H5L_type_t type, const char *cur_name, const char *new_name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Glink, (cur_loc_id, type, cur_name, new_name));
	char **args = assemble_args_list(4, itoa(cur_loc_id),ptoa(&type),strtoa(cur_name),strtoa(new_name));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

size_t WRAPPER_NAME(H5Pget_buffer)(hid_t plist_id, void **tconv, void **bkg) {
	RECORDER_INTERCEPTOR_PROLOGUE(size_t, H5Pget_buffer, (plist_id, tconv, bkg));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(tconv),ptoa(bkg));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5IMget_palette_info)(hid_t loc_id, const char *image_name, int pal_number, hsize_t *pal_dims) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMget_palette_info, (loc_id, image_name, pal_number, pal_dims));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(image_name),itoa(pal_number),itoa((pal_dims==NULL)?-1:*pal_dims));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Ovisit3)(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate2_t op, void *op_data, unsigned fields) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ovisit3, (obj_id, idx_type, order, op, op_data, fields));
	char **args = assemble_args_list(6, itoa(obj_id),itoa(idx_type),itoa(order),ptoa(&op),ptoa(op_data),itoa(fields));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Pget_vol_info)(hid_t plist_id, void **vol_info) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_vol_info, (plist_id, vol_info));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(vol_info));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int WRAPPER_NAME(H5Pget_external_count)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Pget_external_count, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5TBappend_records)(hid_t loc_id, const char *dset_name, hsize_t nrecords, size_t type_size, const size_t *field_offset, const size_t *dst_sizes, const void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBappend_records, (loc_id, dset_name, nrecords, type_size, field_offset, dst_sizes, buf));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(dset_name),itoa(nrecords),itoa(type_size),itoa((field_offset==NULL)?-1:*field_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

ssize_t WRAPPER_NAME(H5Gget_objname_by_idx)(hid_t loc_id, hsize_t idx, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Gget_objname_by_idx, (loc_id, idx, name, size));
	char **args = assemble_args_list(4, itoa(loc_id),itoa(idx),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5IMmake_image_8bit)(hid_t loc_id, const char *dset_name, hsize_t width, hsize_t height, const unsigned char *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5IMmake_image_8bit, (loc_id, dset_name, width, height, buffer));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(dset_name),itoa(width),itoa(height),strtoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Fget_intent)(hid_t file_id, unsigned *intent) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fget_intent, (file_id, intent));
	char **args = assemble_args_list(2, itoa(file_id),itoa((intent==NULL)?-1:*intent));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5VLwrap_register)(void *obj, H5I_type_t type) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5VLwrap_register, (obj, type));
	char **args = assemble_args_list(2, ptoa(obj),ptoa(&type));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Eprint1)(FILE *stream) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Eprint1, (stream));
	char **args = assemble_args_list(1, ptoa(stream));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5LTfind_dataset)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTfind_dataset, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Tget_native_type)(hid_t type_id, H5T_direction_t direction) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Tget_native_type, (type_id, direction));
	char **args = assemble_args_list(2, itoa(type_id),ptoa(&direction));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Dget_type)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_type, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5check_version)(unsigned majnum, unsigned minnum, unsigned relnum) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5check_version, (majnum, minnum, relnum));
	char **args = assemble_args_list(3, itoa(majnum),itoa(minnum),itoa(relnum));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5ESclose)(hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESclose, (es_id));
	char **args = assemble_args_list(1, itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Oare_mdc_flushes_disabled)(hid_t object_id, hbool_t *are_disabled) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oare_mdc_flushes_disabled, (object_id, are_disabled));
	char **args = assemble_args_list(2, itoa(object_id),itoa((are_disabled==NULL)?-1:*are_disabled));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5LTopen_file_image)(void *buf_ptr, size_t buf_size, unsigned flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5LTopen_file_image, (buf_ptr, buf_size, flags));
	char **args = assemble_args_list(3, ptoa(buf_ptr),itoa(buf_size),itoa(flags));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_uint)(hid_t loc_id, const char *obj_name, const char *attr_name, const unsigned int *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_uint, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Dclose)(hid_t dset_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dclose, (dset_id));
	char **args = assemble_args_list(1, itoa(dset_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Aiterate2)(hid_t loc_id, H5_index_t idx_type, H5_iter_order_t order, hsize_t *idx, H5A_operator2_t op, void *op_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aiterate2, (loc_id, idx_type, order, idx, op, op_data));
	char **args = assemble_args_list(6, itoa(loc_id),itoa(idx_type),itoa(order),itoa((idx==NULL)?-1:*idx),ptoa(&op),ptoa(op_data));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Rcreate)(void *ref, hid_t loc_id, const char *name, H5R_type_t ref_type, hid_t space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rcreate, (ref, loc_id, name, ref_type, space_id));
	char **args = assemble_args_list(5, ptoa(ref),itoa(loc_id),strtoa(name),ptoa(&ref_type),itoa(space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_file_image)(hid_t fapl_id, void **buf_ptr_ptr, size_t *buf_len_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_file_image, (fapl_id, buf_ptr_ptr, buf_len_ptr));
	char **args = assemble_args_list(3, itoa(fapl_id),ptoa(buf_ptr_ptr),itoa((buf_len_ptr==NULL)?-1:*buf_len_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_virtual)(hid_t dcpl_id, hid_t vspace_id, const char *src_file_name, const char *src_dset_name, hid_t src_space_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_virtual, (dcpl_id, vspace_id, src_file_name, src_dset_name, src_space_id));
	char **args = assemble_args_list(5, itoa(dcpl_id),itoa(vspace_id),strtoa(src_file_name),strtoa(src_dset_name),itoa(src_space_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5get_free_list_sizes)(size_t *reg_size, size_t *arr_size, size_t *blk_size, size_t *fac_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5get_free_list_sizes, (reg_size, arr_size, blk_size, fac_size));
	char **args = assemble_args_list(4, itoa((reg_size==NULL)?-1:*reg_size),itoa((arr_size==NULL)?-1:*arr_size),itoa((blk_size==NULL)?-1:*blk_size),itoa((fac_size==NULL)?-1:*fac_size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Gunlink)(hid_t loc_id, const char *name) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gunlink, (loc_id, name));
	char **args = assemble_args_list(2, itoa(loc_id),strtoa(name));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Topen2)(hid_t loc_id, const char *name, hid_t tapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Topen2, (loc_id, name, tapl_id));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(name),itoa(tapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Lcreate_external)(const char *file_name, const char *obj_name, hid_t link_loc_id, const char *link_name, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcreate_external, (file_name, obj_name, link_loc_id, link_name, lcpl_id, lapl_id));
	char **args = assemble_args_list(6, strtoa(file_name),strtoa(obj_name),itoa(link_loc_id),strtoa(link_name),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5TBread_fields_index)(hid_t loc_id, const char *dset_name, hsize_t nfields, const int *field_index, hsize_t start, hsize_t nrecords, size_t type_size, const size_t *field_offset, const size_t *dst_sizes, void *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5TBread_fields_index, (loc_id, dset_name, nfields, field_index, start, nrecords, type_size, field_offset, dst_sizes, buf));
	char **args = assemble_args_list(10, itoa(loc_id),strtoa(dset_name),itoa(nfields),itoa((field_index==NULL)?-1:*field_index),itoa(start),itoa(nrecords),itoa(type_size),itoa((field_offset==NULL)?-1:*field_offset),itoa((dst_sizes==NULL)?-1:*dst_sizes),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(10, args);
}

herr_t WRAPPER_NAME(H5PLprepend)(const char *search_path) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PLprepend, (search_path));
	char **args = assemble_args_list(1, strtoa(search_path));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

ssize_t WRAPPER_NAME(H5Fget_name)(hid_t obj_id, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Fget_name, (obj_id, name, size));
	char **args = assemble_args_list(3, itoa(obj_id),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5LTset_attribute_short)(hid_t loc_id, const char *obj_name, const char *attr_name, const short *buffer, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTset_attribute_short, (loc_id, obj_name, attr_name, buffer, size));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((buffer==NULL)?-1:*buffer),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_ullong)(hid_t loc_id, const char *obj_name, const char *attr_name, unsigned long long *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_ullong, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pclose_class)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pclose_class, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Oget_native_info)(hid_t loc_id, H5O_native_info_t *oinfo, unsigned fields) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_native_info, (loc_id, oinfo, fields));
	char **args = assemble_args_list(3, itoa(loc_id),ptoa(oinfo),itoa(fields));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Otoken_from_str)(hid_t loc_id, const char *token_str, H5O_token_t *token) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Otoken_from_str, (loc_id, token_str, token));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(token_str),ptoa(token));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pget_libver_bounds)(hid_t plist_id, H5F_libver_t *low, H5F_libver_t *high) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_libver_bounds, (plist_id, low, high));
	char **args = assemble_args_list(3, itoa(plist_id),ptoa(low),ptoa(high));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_istore_k)(hid_t plist_id, unsigned ik) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_istore_k, (plist_id, ik));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(ik));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pfree_merge_committed_dtype_paths)(hid_t plist_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pfree_merge_committed_dtype_paths, (plist_id));
	char **args = assemble_args_list(1, itoa(plist_id));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Rdereference2)(hid_t obj_id, hid_t oapl_id, H5R_type_t ref_type, const void *ref) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Rdereference2, (obj_id, oapl_id, ref_type, ref));
	char **args = assemble_args_list(4, itoa(obj_id),itoa(oapl_id),ptoa(&ref_type),ptoa(ref));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5LTcopy_region)(const char *file_src, const char *path_src, const hsize_t *block_coord_src, const char *file_dest, const char *path_dest, const hsize_t *block_coord_dset) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTcopy_region, (file_src, path_src, block_coord_src, file_dest, path_dest, block_coord_dset));
	char **args = assemble_args_list(6, strtoa(file_src),strtoa(path_src),itoa((block_coord_src==NULL)?-1:*block_coord_src),strtoa(file_dest),strtoa(path_dest),itoa((block_coord_dset==NULL)?-1:*block_coord_dset));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset)(hid_t loc_id, const char *dset_name, hid_t type_id, void *buffer) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset, (loc_id, dset_name, type_id, buffer));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(dset_name),itoa(type_id),ptoa(buffer));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pset_preserve)(hid_t plist_id, hbool_t status) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_preserve, (plist_id, status));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(status));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int WRAPPER_NAME(H5Piterate)(hid_t id, int *idx, H5P_iterate_t iter_func, void *iter_data) {
	RECORDER_INTERCEPTOR_PROLOGUE(int, H5Piterate, (id, idx, iter_func, iter_data));
	char **args = assemble_args_list(4, itoa(id),itoa((idx==NULL)?-1:*idx),ptoa(&iter_func),ptoa(iter_data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Pget_family_offset)(hid_t fapl_id, hsize_t *offset) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_family_offset, (fapl_id, offset));
	char **args = assemble_args_list(2, itoa(fapl_id),itoa((offset==NULL)?-1:*offset));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_vol_cap_flags)(hid_t plist_id, uint64_t *cap_flags) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_vol_cap_flags, (plist_id, cap_flags));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((cap_flags==NULL)?-1:*cap_flags));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LRcreate_region_references)(hid_t obj_id, size_t num_elem, const char **path, const hsize_t *block_coord, hdset_reg_ref_t *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LRcreate_region_references, (obj_id, num_elem, path, block_coord, buf));
	char **args = assemble_args_list(5, itoa(obj_id),itoa(num_elem),ptoa(path),itoa((block_coord==NULL)?-1:*block_coord),ptoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5LTget_attribute_long)(hid_t loc_id, const char *obj_name, const char *attr_name, long *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTget_attribute_long, (loc_id, obj_name, attr_name, data));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((data==NULL)?-1:*data));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5free_memory)(void *mem) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5free_memory, (mem));
	char **args = assemble_args_list(1, ptoa(mem));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5ESget_err_count)(hid_t es_id, size_t *num_errs) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESget_err_count, (es_id, num_errs));
	char **args = assemble_args_list(2, itoa(es_id),itoa((num_errs==NULL)?-1:*num_errs));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

ssize_t WRAPPER_NAME(H5Fget_obj_count)(hid_t file_id, unsigned types) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Fget_obj_count, (file_id, types));
	char **args = assemble_args_list(2, itoa(file_id),itoa(types));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5LTread_dataset_string)(hid_t loc_id, const char *dset_name, char *buf) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5LTread_dataset_string, (loc_id, dset_name, buf));
	char **args = assemble_args_list(3, itoa(loc_id),strtoa(dset_name),strtoa(buf));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Ovisit_by_name2)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, H5O_iterate1_t op, void *op_data, unsigned fields, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ovisit_by_name2, (loc_id, obj_name, idx_type, order, op, op_data, fields, lapl_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),ptoa(&op),ptoa(op_data),itoa(fields),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Pset_file_image)(hid_t fapl_id, void *buf_ptr, size_t buf_len) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_file_image, (fapl_id, buf_ptr, buf_len));
	char **args = assemble_args_list(3, itoa(fapl_id),ptoa(buf_ptr),itoa(buf_len));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Pset_shared_mesg_index)(hid_t plist_id, unsigned index_num, unsigned mesg_type_flags, unsigned min_mesg_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_shared_mesg_index, (plist_id, index_num, mesg_type_flags, min_mesg_size));
	char **args = assemble_args_list(4, itoa(plist_id),itoa(index_num),itoa(mesg_type_flags),itoa(min_mesg_size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Lcreate_hard)(hid_t cur_loc, const char *cur_name, hid_t dst_loc, const char *dst_name, hid_t lcpl_id, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcreate_hard, (cur_loc, cur_name, dst_loc, dst_name, lcpl_id, lapl_id));
	char **args = assemble_args_list(6, itoa(cur_loc),strtoa(cur_name),itoa(dst_loc),strtoa(dst_name),itoa(lcpl_id),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5PTappend)(hid_t table_id, size_t nrecords, const void *data) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5PTappend, (table_id, nrecords, data));
	char **args = assemble_args_list(3, itoa(table_id),itoa(nrecords),ptoa(data));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_idx1)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_info1_t *oinfo, hid_t lapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_idx1, (loc_id, group_name, idx_type, order, n, oinfo, lapl_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(oinfo),itoa(lapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5DSset_scale)(hid_t dsid, const char *dimname) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5DSset_scale, (dsid, dimname));
	char **args = assemble_args_list(2, itoa(dsid),strtoa(dimname));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Dcreate_anon)(hid_t loc_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dcreate_anon, (loc_id, type_id, space_id, dcpl_id, dapl_id));
	char **args = assemble_args_list(5, itoa(loc_id),itoa(type_id),itoa(space_id),itoa(dcpl_id),itoa(dapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Pget_actual_selection_io_mode)(hid_t plist_id, uint32_t *actual_selection_io_mode) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_actual_selection_io_mode, (plist_id, actual_selection_io_mode));
	char **args = assemble_args_list(2, itoa(plist_id),itoa((actual_selection_io_mode==NULL)?-1:*actual_selection_io_mode));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_elink_file_cache_size)(hid_t plist_id, unsigned efc_size) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_elink_file_cache_size, (plist_id, efc_size));
	char **args = assemble_args_list(2, itoa(plist_id),itoa(efc_size));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Aclose_async)(hid_t attr_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aclose_async, (attr_id, es_id));
	char **args = assemble_args_list(2, itoa(attr_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Acreate_async)(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate_async, (loc_id, attr_name, type_id, space_id, acpl_id, aapl_id, es_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(attr_name),itoa(type_id),itoa(space_id),itoa(acpl_id),itoa(aapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t WRAPPER_NAME(H5Acreate_by_name_async)(hid_t loc_id, const char *obj_name, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Acreate_by_name_async, (loc_id, obj_name, attr_name, type_id, space_id, acpl_id, aapl_id, lapl_id, es_id));
	char **args = assemble_args_list(9, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa(type_id),itoa(space_id),itoa(acpl_id),itoa(aapl_id),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

herr_t WRAPPER_NAME(H5Aexists_async)(hid_t obj_id, const char *attr_name, hbool_t *exists, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aexists_async, (obj_id, attr_name, exists, es_id));
	char **args = assemble_args_list(4, itoa(obj_id),strtoa(attr_name),itoa((exists==NULL)?-1:*exists),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Aexists_by_name_async)(hid_t loc_id, const char *obj_name, const char *attr_name, hbool_t *exists, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aexists_by_name_async, (loc_id, obj_name, attr_name, exists, lapl_id, es_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa((exists==NULL)?-1:*exists),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

hid_t WRAPPER_NAME(H5Aopen_async)(hid_t obj_id, const char *attr_name, hid_t aapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_async, (obj_id, attr_name, aapl_id, es_id));
	char **args = assemble_args_list(4, itoa(obj_id),strtoa(attr_name),itoa(aapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Aopen_by_idx_async)(hid_t loc_id, const char *obj_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t aapl_id, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_by_idx_async, (loc_id, obj_name, idx_type, order, n, aapl_id, lapl_id, es_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(obj_name),itoa(idx_type),itoa(order),itoa(n),itoa(aapl_id),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

hid_t WRAPPER_NAME(H5Aopen_by_name_async)(hid_t loc_id, const char *obj_name, const char *attr_name, hid_t aapl_id, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Aopen_by_name_async, (loc_id, obj_name, attr_name, aapl_id, lapl_id, es_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(obj_name),strtoa(attr_name),itoa(aapl_id),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Aread_async)(hid_t attr_id, hid_t dtype_id, void *buf, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Aread_async, (attr_id, dtype_id, buf, es_id));
	char **args = assemble_args_list(4, itoa(attr_id),itoa(dtype_id),ptoa(buf),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Arename_async)(hid_t loc_id, const char *old_name, const char *new_name, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Arename_async, (loc_id, old_name, new_name, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(old_name),strtoa(new_name),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Arename_by_name_async)(hid_t loc_id, const char *obj_name, const char *old_attr_name, const char *new_attr_name, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Arename_by_name_async, (loc_id, obj_name, old_attr_name, new_attr_name, lapl_id, es_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(obj_name),strtoa(old_attr_name),strtoa(new_attr_name),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Awrite_async)(hid_t attr_id, hid_t type_id, const void *buf, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Awrite_async, (attr_id, type_id, buf, es_id));
	char **args = assemble_args_list(4, itoa(attr_id),itoa(type_id),ptoa(buf),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Dcreate_async)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dcreate_async, (loc_id, name, type_id, space_id, lcpl_id, dcpl_id, dapl_id, es_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(name),itoa(type_id),itoa(space_id),itoa(lcpl_id),itoa(dcpl_id),itoa(dapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

hid_t WRAPPER_NAME(H5Dopen_async)(hid_t loc_id, const char *name, hid_t dapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dopen_async, (loc_id, name, dapl_id, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(dapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Dget_space_async)(hid_t dset_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Dget_space_async, (dset_id, es_id));
	char **args = assemble_args_list(2, itoa(dset_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Dread_async)(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, void *buf, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dread_async, (dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, es_id));
	char **args = assemble_args_list(7, itoa(dset_id),itoa(mem_type_id),itoa(mem_space_id),itoa(file_space_id),itoa(dxpl_id),ptoa(buf),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Dread_multi_async)(size_t count, hid_t dset_id[], hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[], hid_t dxpl_id, void *buf[], hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dread_multi_async, (count, dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, es_id));
	char **args = assemble_args_list(8, itoa(count),ptoa(dset_id),ptoa(mem_type_id),ptoa(mem_space_id),ptoa(file_space_id),itoa(dxpl_id),ptoa(buf),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Dwrite_async)(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, const void *buf, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dwrite_async, (dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, es_id));
	char **args = assemble_args_list(7, itoa(dset_id),itoa(mem_type_id),itoa(mem_space_id),itoa(file_space_id),itoa(dxpl_id),ptoa(buf),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Dwrite_multi_async)(size_t count, hid_t dset_id[], hid_t mem_type_id[], hid_t mem_space_id[], hid_t file_space_id[], hid_t dxpl_id, const void *buf[], hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dwrite_multi_async, (count, dset_id, mem_type_id, mem_space_id, file_space_id, dxpl_id, buf, es_id));
	char **args = assemble_args_list(8, itoa(count),ptoa(dset_id),ptoa(mem_type_id),ptoa(mem_space_id),ptoa(file_space_id),itoa(dxpl_id),ptoa(buf),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Dset_extent_async)(hid_t dset_id, const hsize_t size[], hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dset_extent_async, (dset_id, size, es_id));
	char **args = assemble_args_list(3, itoa(dset_id),ptoa(size),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Dclose_async)(hid_t dset_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Dclose_async, (dset_id, es_id));
	char **args = assemble_args_list(2, itoa(dset_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Fcreate_async)(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fcreate_async, (filename, flags, fcpl_id, fapl_id, es_id));
	char **args = assemble_args_list(5, strtoa(filename),itoa(flags),itoa(fcpl_id),itoa(fapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Fopen_async)(const char *filename, unsigned flags, hid_t access_plist, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Fopen_async, (filename, flags, access_plist, es_id));
	char **args = assemble_args_list(4, strtoa(filename),itoa(flags),itoa(access_plist),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Freopen_async)(hid_t file_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Freopen_async, (file_id, es_id));
	char **args = assemble_args_list(2, itoa(file_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Fflush_async)(hid_t object_id, H5F_scope_t scope, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fflush_async, (object_id, scope, es_id));
	char **args = assemble_args_list(3, itoa(object_id),itoa(scope),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Fclose_async)(hid_t file_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Fclose_async, (file_id, es_id));
	char **args = assemble_args_list(2, itoa(file_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Gcreate_async)(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gcreate_async, (loc_id, name, lcpl_id, gcpl_id, gapl_id, es_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(name),itoa(lcpl_id),itoa(gcpl_id),itoa(gapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

hid_t WRAPPER_NAME(H5Gopen_async)(hid_t loc_id, const char *name, hid_t gapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Gopen_async, (loc_id, name, gapl_id, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(gapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Gget_info_async)(hid_t loc_id, H5G_info_t *ginfo, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_info_async, (loc_id, ginfo, es_id));
	char **args = assemble_args_list(3, itoa(loc_id),ptoa(ginfo),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Gget_info_by_name_async)(hid_t loc_id, const char *name, H5G_info_t *ginfo, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_info_by_name_async, (loc_id, name, ginfo, lapl_id, es_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),ptoa(ginfo),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Gget_info_by_idx_async)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5G_info_t *ginfo, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gget_info_by_idx_async, (loc_id, group_name, idx_type, order, n, ginfo, lapl_id, es_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),ptoa(ginfo),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

herr_t WRAPPER_NAME(H5Gclose_async)(hid_t group_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Gclose_async, (group_id, es_id));
	char **args = assemble_args_list(2, itoa(group_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Lcreate_hard_async)(hid_t cur_loc_id, const char *cur_name, hid_t new_loc_id, const char *new_name, hid_t lcpl_id, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcreate_hard_async, (cur_loc_id, cur_name, new_loc_id, new_name, lcpl_id, lapl_id, es_id));
	char **args = assemble_args_list(7, itoa(cur_loc_id),strtoa(cur_name),itoa(new_loc_id),strtoa(new_name),itoa(lcpl_id),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Lcreate_soft_async)(const char *link_target, hid_t link_loc_id, const char *link_name, hid_t lcpl_id, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lcreate_soft_async, (link_target, link_loc_id, link_name, lcpl_id, lapl_id, es_id));
	char **args = assemble_args_list(6, strtoa(link_target),itoa(link_loc_id),strtoa(link_name),itoa(lcpl_id),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Ldelete_async)(hid_t loc_id, const char *name, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ldelete_async, (loc_id, name, lapl_id, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Ldelete_by_idx_async)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ldelete_by_idx_async, (loc_id, group_name, idx_type, order, n, lapl_id, es_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Lexists_async)(hid_t loc_id, const char *name, hbool_t *exists, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Lexists_async, (loc_id, name, exists, lapl_id, es_id));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),itoa((exists==NULL)?-1:*exists),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

herr_t WRAPPER_NAME(H5Literate_async)(hid_t group_id, H5_index_t idx_type, H5_iter_order_t order, hsize_t *idx_p, H5L_iterate2_t op, void *op_data, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Literate_async, (group_id, idx_type, order, idx_p, op, op_data, es_id));
	char **args = assemble_args_list(7, itoa(group_id),itoa(idx_type),itoa(order),itoa((idx_p==NULL)?-1:*idx_p),ptoa(&op),ptoa(op_data),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t WRAPPER_NAME(H5Mcreate_async)(hid_t loc_id, const char *name, hid_t key_type_id, hid_t val_type_id, hid_t lcpl_id, hid_t mcpl_id, hid_t mapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Mcreate_async, (loc_id, name, key_type_id, val_type_id, lcpl_id, mcpl_id, mapl_id, es_id));
	char **args = assemble_args_list(8, itoa(loc_id),strtoa(name),itoa(key_type_id),itoa(val_type_id),itoa(lcpl_id),itoa(mcpl_id),itoa(mapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

hid_t WRAPPER_NAME(H5Mopen_async)(hid_t loc_id, const char *name, hid_t mapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Mopen_async, (loc_id, name, mapl_id, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(mapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Mclose_async)(hid_t map_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Mclose_async, (map_id, es_id));
	char **args = assemble_args_list(2, itoa(map_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Mput_async)(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t val_mem_type_id, const void *value, hid_t dxpl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Mput_async, (map_id, key_mem_type_id, key, val_mem_type_id, value, dxpl_id, es_id));
	char **args = assemble_args_list(7, itoa(map_id),itoa(key_mem_type_id),ptoa(key),itoa(val_mem_type_id),ptoa(value),itoa(dxpl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Mget_async)(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t val_mem_type_id, void *value, hid_t dxpl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Mget_async, (map_id, key_mem_type_id, key, val_mem_type_id, value, dxpl_id, es_id));
	char **args = assemble_args_list(7, itoa(map_id),itoa(key_mem_type_id),ptoa(key),itoa(val_mem_type_id),ptoa(value),itoa(dxpl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

hid_t WRAPPER_NAME(H5Oopen_async)(hid_t loc_id, const char *name, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen_async, (loc_id, name, lapl_id, es_id));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Oopen_by_idx_async)(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Oopen_by_idx_async, (loc_id, group_name, idx_type, order, n, lapl_id, es_id));
	char **args = assemble_args_list(7, itoa(loc_id),strtoa(group_name),itoa(idx_type),itoa(order),itoa(n),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Oget_info_by_name_async)(hid_t loc_id, const char *name, H5O_info2_t *oinfo, unsigned fields, hid_t lapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oget_info_by_name_async, (loc_id, name, oinfo, fields, lapl_id, es_id));
	char **args = assemble_args_list(6, itoa(loc_id),strtoa(name),ptoa(oinfo),itoa(fields),itoa(lapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

herr_t WRAPPER_NAME(H5Ocopy_async)(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id, const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Ocopy_async, (src_loc_id, src_name, dst_loc_id, dst_name, ocpypl_id, lcpl_id, es_id));
	char **args = assemble_args_list(7, itoa(src_loc_id),strtoa(src_name),itoa(dst_loc_id),strtoa(dst_name),itoa(ocpypl_id),itoa(lcpl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

herr_t WRAPPER_NAME(H5Oclose_async)(hid_t object_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oclose_async, (object_id, es_id));
	char **args = assemble_args_list(2, itoa(object_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Oflush_async)(hid_t obj_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Oflush_async, (obj_id, es_id));
	char **args = assemble_args_list(2, itoa(obj_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Orefresh_async)(hid_t oid, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Orefresh_async, (oid, es_id));
	char **args = assemble_args_list(2, itoa(oid),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Rcreate_attr)(hid_t loc_id, const char *name, const char *attr_name, hid_t oapl_id, H5R_ref_t *ref_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rcreate_attr, (loc_id, name, attr_name, oapl_id, ref_ptr));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),strtoa(attr_name),itoa(oapl_id),ptoa(ref_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

ssize_t WRAPPER_NAME(H5Rget_file_name)(const H5R_ref_t *ref_ptr, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Rget_file_name, (ref_ptr, name, size));
	char **args = assemble_args_list(3, ptoa(ref_ptr),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Rdestroy)(H5R_ref_t *ref_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rdestroy, (ref_ptr));
	char **args = assemble_args_list(1, ptoa(ref_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

hid_t WRAPPER_NAME(H5Ropen_object)(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ropen_object, (ref_ptr, rapl_id, oapl_id));
	char **args = assemble_args_list(3, ptoa(ref_ptr),itoa(rapl_id),itoa(oapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Ropen_region)(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ropen_region, (ref_ptr, rapl_id, oapl_id));
	char **args = assemble_args_list(3, ptoa(ref_ptr),itoa(rapl_id),itoa(oapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Ropen_attr)(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t aapl_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ropen_attr, (ref_ptr, rapl_id, aapl_id));
	char **args = assemble_args_list(3, ptoa(ref_ptr),itoa(rapl_id),itoa(aapl_id));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

htri_t WRAPPER_NAME(H5Requal)(const H5R_ref_t *ref1_ptr, const H5R_ref_t *ref2_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(htri_t, H5Requal, (ref1_ptr, ref2_ptr));
	char **args = assemble_args_list(2, ptoa(ref1_ptr),ptoa(ref2_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Rget_obj_type3)(H5R_ref_t *ref_ptr, hid_t rapl_id, H5O_type_t *obj_type) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rget_obj_type3, (ref_ptr, rapl_id, obj_type));
	char **args = assemble_args_list(3, ptoa(ref_ptr),itoa(rapl_id),ptoa(obj_type));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

ssize_t WRAPPER_NAME(H5Rget_obj_name)(H5R_ref_t *ref_ptr, hid_t rapl_id, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Rget_obj_name, (ref_ptr, rapl_id, name, size));
	char **args = assemble_args_list(4, ptoa(ref_ptr),itoa(rapl_id),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5Rcreate_region)(hid_t loc_id, const char *name, hid_t space_id, hid_t oapl_id, H5R_ref_t *ref_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rcreate_region, (loc_id, name, space_id, oapl_id, ref_ptr));
	char **args = assemble_args_list(5, itoa(loc_id),strtoa(name),itoa(space_id),itoa(oapl_id),ptoa(ref_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

H5R_type_t WRAPPER_NAME(H5Rget_type)(const H5R_ref_t *ref_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(H5R_type_t, H5Rget_type, (ref_ptr));
	char **args = assemble_args_list(1, ptoa(ref_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

herr_t WRAPPER_NAME(H5Rcreate_object)(hid_t loc_id, const char *name, hid_t oapl_id, H5R_ref_t *ref_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rcreate_object, (loc_id, name, oapl_id, ref_ptr));
	char **args = assemble_args_list(4, itoa(loc_id),strtoa(name),itoa(oapl_id),ptoa(ref_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

ssize_t WRAPPER_NAME(H5Rget_attr_name)(const H5R_ref_t *ref_ptr, char *name, size_t size) {
	RECORDER_INTERCEPTOR_PROLOGUE(ssize_t, H5Rget_attr_name, (ref_ptr, name, size));
	char **args = assemble_args_list(3, ptoa(ref_ptr),strtoa(name),itoa(size));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

herr_t WRAPPER_NAME(H5Rcopy)(const H5R_ref_t *src_ref_ptr, H5R_ref_t *dst_ref_ptr) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Rcopy, (src_ref_ptr, dst_ref_ptr));
	char **args = assemble_args_list(2, ptoa(src_ref_ptr),ptoa(dst_ref_ptr));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

hid_t WRAPPER_NAME(H5Ropen_object_async)(unsigned app_line, H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ropen_object_async, (app_line, ref_ptr, rapl_id, oapl_id, es_id));
	char **args = assemble_args_list(5, itoa(app_line),ptoa(ref_ptr),itoa(rapl_id),itoa(oapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

hid_t WRAPPER_NAME(H5Ropen_region_async)(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t oapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ropen_region_async, (ref_ptr, rapl_id, oapl_id, es_id));
	char **args = assemble_args_list(4, ptoa(ref_ptr),itoa(rapl_id),itoa(oapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

hid_t WRAPPER_NAME(H5Ropen_attr_async)(H5R_ref_t *ref_ptr, hid_t rapl_id, hid_t aapl_id, hid_t es_id) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Ropen_attr_async, (ref_ptr, rapl_id, aapl_id, es_id));
	char **args = assemble_args_list(4, ptoa(ref_ptr),itoa(rapl_id),itoa(aapl_id),itoa(es_id));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5atclose)(H5_atclose_func_t func, void *ctx) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5atclose, (func, ctx));
	char **args = assemble_args_list(2, ptoa(&func),ptoa(ctx));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pset_selection_io)(hid_t plist_id, H5D_selection_io_mode_t selection_io_mode) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pset_selection_io, (plist_id, selection_io_mode));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(&selection_io_mode));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5Pget_selection_io)(hid_t plist_id, H5D_selection_io_mode_t *selection_io_mode) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5Pget_selection_io, (plist_id, selection_io_mode));
	char **args = assemble_args_list(2, itoa(plist_id),ptoa(selection_io_mode));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5ESget_err_info)(hid_t es_id, size_t num_err_info, H5ES_err_info_t err_info[], size_t *err_cleared) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESget_err_info, (es_id, num_err_info, err_info, err_cleared));
	char **args = assemble_args_list(4, itoa(es_id),itoa(num_err_info),ptoa(err_info),itoa((err_cleared==NULL)?-1:*err_cleared));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5ESregister_insert_func)(hid_t es_id, H5ES_event_insert_func_t func, void *ctx) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESregister_insert_func, (es_id, func, ctx));
	char **args = assemble_args_list(3, itoa(es_id),ptoa(&func),ptoa(ctx));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

hid_t WRAPPER_NAME(H5Iregister_future)(H5I_type_t type, const void *object, H5I_future_realize_func_t realize_cb, H5I_future_discard_func_t discard_cb) {
	RECORDER_INTERCEPTOR_PROLOGUE(hid_t, H5Iregister_future, (type, object, realize_cb, discard_cb));
	char **args = assemble_args_list(4, ptoa(&type),ptoa(object),ptoa(&realize_cb),ptoa(&discard_cb));
	RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

herr_t WRAPPER_NAME(H5ESfree_err_info)(size_t num_err_info, H5ES_err_info_t err_info[]) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESfree_err_info, (num_err_info, err_info));
	char **args = assemble_args_list(2, itoa(num_err_info),ptoa(err_info));
	RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

herr_t WRAPPER_NAME(H5ESregister_complete_func)(hid_t es_id, H5ES_event_complete_func_t func, void *ctx) {
	RECORDER_INTERCEPTOR_PROLOGUE(herr_t, H5ESregister_complete_func, (es_id, func, ctx));
	char **args = assemble_args_list(3, itoa(es_id),ptoa(&func),ptoa(ctx));
	RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}
