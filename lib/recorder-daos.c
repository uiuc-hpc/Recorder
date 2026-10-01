#define _XOPEN_SOURCE 500
#define _GNU_SOURCE         //  Need to be on top to use RTLD_NEXT

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include "recorder.h"

#ifdef RECORDER_WITH_DAOS

#include <daos.h>
#include <daos_fs.h>

/*
 * Wrappers of the DAOS File System (DFS) API and the native DAOS API.
 *
 * Following the same convention as the other layers (e.g., recorder-hdf5.c),
 * every wrapper.
 *
 * Identifiers are recorded HDF5-style, i.e., as raw handle/pointer values:
 *   - daos_handle_t (by value)   -> itoa(handle.cookie)
 *   - daos_handle_t* (output)    -> itoa((p==NULL)?-1:p->cookie)
 *   - daos_obj_id_t (by value)   -> itoa(oid.hi), itoa(oid.lo)  (two fields)
 *   - d_iov_t (by value)         -> ptoa(iov.iov_buf), itoa(iov.iov_len)
 *   - pointers (dfs_t*, etc.)    -> ptoa(ptr)
 *   - scalars (offsets, sizes,
 *     flags, modes, counts)      -> itoa(value); output scalars are guarded
 *     with the (p==NULL)?-1:*p idiom used elsewhere in Recorder.
 *   - strings (paths/names/keys) -> strtoa(str)
 *
 * No object->name resolution is performed; identifiers are stored as-is,
 * exactly like the HDF5 layer stores hid_t handles.
 */


/* ------------------------------------------------------------------ *
 *  DFS API (libdfs)
 * ------------------------------------------------------------------ */

int WRAPPER_NAME(dfs_mount)(daos_handle_t poh, daos_handle_t coh, int flags, dfs_t **dfs) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_mount, (poh, coh, flags, dfs));
    char **args = assemble_args_list(4, itoa(poh.cookie), itoa(coh.cookie), itoa(flags),
                                     ptoa((dfs==NULL)?NULL:*dfs));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(dfs_umount)(dfs_t *dfs) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_umount, (dfs));
    char **args = assemble_args_list(1, ptoa(dfs));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int WRAPPER_NAME(dfs_global2local)(daos_handle_t poh, daos_handle_t coh, int flags, d_iov_t glob, dfs_t **dfs) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_global2local, (poh, coh, flags, glob, dfs));
    char **args = assemble_args_list(6, itoa(poh.cookie), itoa(coh.cookie), itoa(flags),
                                     ptoa(glob.iov_buf), itoa(glob.iov_len), ptoa((dfs==NULL)?NULL:*dfs));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(dfs_lookup)(dfs_t *dfs, const char *path, int flags, dfs_obj_t **obj, mode_t *mode, struct stat *stbuf) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_lookup, (dfs, path, flags, obj, mode, stbuf));
    char **args = assemble_args_list(6, ptoa(dfs), strtoa(path), itoa(flags),
                                     ptoa((obj==NULL)?NULL:*obj), itoa((mode==NULL)?-1:*mode), ptoa(stbuf));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(dfs_lookup_rel)(dfs_t *dfs, dfs_obj_t *parent, const char *name, int flags, dfs_obj_t **obj, mode_t *mode, struct stat *stbuf) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_lookup_rel, (dfs, parent, name, flags, obj, mode, stbuf));
    char **args = assemble_args_list(7, ptoa(dfs), ptoa(parent), strtoa(name), itoa(flags),
                                     ptoa((obj==NULL)?NULL:*obj), itoa((mode==NULL)?-1:*mode), ptoa(stbuf));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int WRAPPER_NAME(dfs_open)(dfs_t *dfs, dfs_obj_t *parent, const char *name, mode_t mode, int flags, daos_oclass_id_t cid, daos_size_t chunk_size, const char *value, dfs_obj_t **obj) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_open, (dfs, parent, name, mode, flags, cid, chunk_size, value, obj));
    char **args = assemble_args_list(9, ptoa(dfs), ptoa(parent), strtoa(name), itoa(mode), itoa(flags),
                                     itoa(cid), itoa(chunk_size), strtoa(value), ptoa((obj==NULL)?NULL:*obj));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

int WRAPPER_NAME(dfs_dup)(dfs_t *dfs, dfs_obj_t *obj, int flags, dfs_obj_t **new_obj) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_dup, (dfs, obj, flags, new_obj));
    char **args = assemble_args_list(4, ptoa(dfs), ptoa(obj), itoa(flags), ptoa((new_obj==NULL)?NULL:*new_obj));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(dfs_obj_global2local)(dfs_t *dfs, int flags, d_iov_t glob, dfs_obj_t **obj) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_obj_global2local, (dfs, flags, glob, obj));
    char **args = assemble_args_list(5, ptoa(dfs), itoa(flags), ptoa(glob.iov_buf), itoa(glob.iov_len),
                                     ptoa((obj==NULL)?NULL:*obj));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int WRAPPER_NAME(dfs_release)(dfs_obj_t *obj) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_release, (obj));
    char **args = assemble_args_list(1, ptoa(obj));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int WRAPPER_NAME(dfs_read)(dfs_t *dfs, dfs_obj_t *obj, d_sg_list_t *sgl, daos_off_t off, daos_size_t *read_size, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_read, (dfs, obj, sgl, off, read_size, ev));
    char **args = assemble_args_list(6, ptoa(dfs), ptoa(obj), ptoa(sgl), itoa(off),
                                     itoa((read_size==NULL)?-1:*read_size), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(dfs_readx)(dfs_t *dfs, dfs_obj_t *obj, dfs_iod_t *iod, d_sg_list_t *sgl, daos_size_t *read_size, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_readx, (dfs, obj, iod, sgl, read_size, ev));
    char **args = assemble_args_list(6, ptoa(dfs), ptoa(obj), ptoa(iod), ptoa(sgl),
                                     itoa((read_size==NULL)?-1:*read_size), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(dfs_write)(dfs_t *dfs, dfs_obj_t *obj, d_sg_list_t *sgl, daos_off_t off, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_write, (dfs, obj, sgl, off, ev));
    char **args = assemble_args_list(5, ptoa(dfs), ptoa(obj), ptoa(sgl), itoa(off), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int WRAPPER_NAME(dfs_writex)(dfs_t *dfs, dfs_obj_t *obj, dfs_iod_t *iod, d_sg_list_t *sgl, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_writex, (dfs, obj, iod, sgl, ev));
    char **args = assemble_args_list(5, ptoa(dfs), ptoa(obj), ptoa(iod), ptoa(sgl), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int WRAPPER_NAME(dfs_get_size)(dfs_t *dfs, dfs_obj_t *obj, daos_size_t *size) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_get_size, (dfs, obj, size));
    char **args = assemble_args_list(3, ptoa(dfs), ptoa(obj), itoa((size==NULL)?-1:*size));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int WRAPPER_NAME(dfs_punch)(dfs_t *dfs, dfs_obj_t *obj, daos_off_t offset, daos_size_t len) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_punch, (dfs, obj, offset, len));
    char **args = assemble_args_list(4, ptoa(dfs), ptoa(obj), itoa(offset), itoa(len));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(dfs_remove)(dfs_t *dfs, dfs_obj_t *parent, const char *name, bool force, daos_obj_id_t *oid) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_remove, (dfs, parent, name, force, oid));
    char **args = assemble_args_list(6, ptoa(dfs), ptoa(parent), strtoa(name), itoa(force),
                                     itoa((oid==NULL)?-1:oid->hi), itoa((oid==NULL)?-1:oid->lo));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(dfs_ostat)(dfs_t *dfs, dfs_obj_t *obj, struct stat *stbuf) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_ostat, (dfs, obj, stbuf));
    char **args = assemble_args_list(3, ptoa(dfs), ptoa(obj), ptoa(stbuf));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int WRAPPER_NAME(dfs_osetattr)(dfs_t *dfs, dfs_obj_t *obj, struct stat *stbuf, int flags) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, dfs_osetattr, (dfs, obj, stbuf, flags));
    char **args = assemble_args_list(4, ptoa(dfs), ptoa(obj), ptoa(stbuf), itoa(flags));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}


/* ------------------------------------------------------------------ *
 *  Native DAOS API - Container
 * ------------------------------------------------------------------ */

/* DAOS aliases daos_cont_open to the versioned symbol daos_cont_open2 via
 * `#define daos_cont_open daos_cont_open2` in daos.h. We must name the wrapper
 * after the real symbol explicitly: WRAPPER_NAME() uses ## (token paste), which
 * suppresses macro expansion, so WRAPPER_NAME(daos_cont_open) would produce
 * wrapper_daos_cont_open while the GOTCHA_WRAP/GOTCHA_WRAP_ACTION machinery
 * (which expands the argument first) expects wrapper_daos_cont_open2. */
int WRAPPER_NAME(daos_cont_open2)(daos_handle_t poh, const char *cont, unsigned int flags, daos_handle_t *coh, daos_cont_info_t *info, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_cont_open2, (poh, cont, flags, coh, info, ev));
    char **args = assemble_args_list(6, itoa(poh.cookie), strtoa(cont), itoa(flags),
                                     itoa((coh==NULL)?-1:coh->cookie), ptoa(info), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(daos_cont_global2local)(daos_handle_t poh, d_iov_t glob, daos_handle_t *coh) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_cont_global2local, (poh, glob, coh));
    char **args = assemble_args_list(4, itoa(poh.cookie), ptoa(glob.iov_buf), itoa(glob.iov_len),
                                     itoa((coh==NULL)?-1:coh->cookie));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(daos_cont_close)(daos_handle_t coh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_cont_close, (coh, ev));
    char **args = assemble_args_list(2, itoa(coh.cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}


/* ------------------------------------------------------------------ *
 *  Native DAOS API - Object (Multi-Level Key-Array)
 * ------------------------------------------------------------------ */

int WRAPPER_NAME(daos_obj_open)(daos_handle_t coh, daos_obj_id_t oid, unsigned int mode, daos_handle_t *oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_open, (coh, oid, mode, oh, ev));
    char **args = assemble_args_list(6, itoa(coh.cookie), itoa(oid.hi), itoa(oid.lo), itoa(mode),
                                     itoa((oh==NULL)?-1:oh->cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(daos_obj_fetch)(daos_handle_t oh, daos_handle_t th, uint64_t flags, daos_key_t *dkey, unsigned int nr, daos_iod_t *iods, d_sg_list_t *sgls, daos_iom_t *ioms, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_fetch, (oh, th, flags, dkey, nr, iods, sgls, ioms, ev));
    char **args = assemble_args_list(9, itoa(oh.cookie), itoa(th.cookie), itoa(flags), ptoa(dkey),
                                     itoa(nr), ptoa(iods), ptoa(sgls), ptoa(ioms), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

int WRAPPER_NAME(daos_obj_update)(daos_handle_t oh, daos_handle_t th, uint64_t flags, daos_key_t *dkey, unsigned int nr, daos_iod_t *iods, d_sg_list_t *sgls, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_update, (oh, th, flags, dkey, nr, iods, sgls, ev));
    char **args = assemble_args_list(8, itoa(oh.cookie), itoa(th.cookie), itoa(flags), ptoa(dkey),
                                     itoa(nr), ptoa(iods), ptoa(sgls), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

int WRAPPER_NAME(daos_obj_punch)(daos_handle_t oh, daos_handle_t th, uint64_t flags, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_punch, (oh, th, flags, ev));
    char **args = assemble_args_list(4, itoa(oh.cookie), itoa(th.cookie), itoa(flags), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(daos_obj_punch_dkeys)(daos_handle_t oh, daos_handle_t th, uint64_t flags, unsigned int nr, daos_key_t *dkeys, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_punch_dkeys, (oh, th, flags, nr, dkeys, ev));
    char **args = assemble_args_list(6, itoa(oh.cookie), itoa(th.cookie), itoa(flags), itoa(nr),
                                     ptoa(dkeys), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(daos_obj_punch_akeys)(daos_handle_t oh, daos_handle_t th, uint64_t flags, daos_key_t *dkey, unsigned int nr, daos_key_t *akeys, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_punch_akeys, (oh, th, flags, dkey, nr, akeys, ev));
    char **args = assemble_args_list(7, itoa(oh.cookie), itoa(th.cookie), itoa(flags), ptoa(dkey),
                                     itoa(nr), ptoa(akeys), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int WRAPPER_NAME(daos_obj_list_dkey)(daos_handle_t oh, daos_handle_t th, uint32_t *nr, daos_key_desc_t *kds, d_sg_list_t *sgl, daos_anchor_t *anchor, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_list_dkey, (oh, th, nr, kds, sgl, anchor, ev));
    char **args = assemble_args_list(7, itoa(oh.cookie), itoa(th.cookie), itoa((nr==NULL)?-1:*nr),
                                     ptoa(kds), ptoa(sgl), ptoa(anchor), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int WRAPPER_NAME(daos_obj_list_akey)(daos_handle_t oh, daos_handle_t th, daos_key_t *dkey, uint32_t *nr, daos_key_desc_t *kds, d_sg_list_t *sgl, daos_anchor_t *anchor, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_list_akey, (oh, th, dkey, nr, kds, sgl, anchor, ev));
    char **args = assemble_args_list(8, itoa(oh.cookie), itoa(th.cookie), ptoa(dkey),
                                     itoa((nr==NULL)?-1:*nr), ptoa(kds), ptoa(sgl), ptoa(anchor), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

int WRAPPER_NAME(daos_obj_list_recx)(daos_handle_t oh, daos_handle_t th, daos_key_t *dkey, daos_key_t *akey, daos_size_t *size, uint32_t *nr, daos_recx_t *recxs, daos_epoch_range_t *eprs, daos_anchor_t *anchor, bool incr_order, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_list_recx, (oh, th, dkey, akey, size, nr, recxs, eprs, anchor, incr_order, ev));
    char **args = assemble_args_list(11, itoa(oh.cookie), itoa(th.cookie), ptoa(dkey), ptoa(akey),
                                     itoa((size==NULL)?-1:*size), itoa((nr==NULL)?-1:*nr), ptoa(recxs),
                                     ptoa(eprs), ptoa(anchor), itoa(incr_order), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(11, args);
}

int WRAPPER_NAME(daos_obj_close)(daos_handle_t oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_obj_close, (oh, ev));
    char **args = assemble_args_list(2, itoa(oh.cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}


/* ------------------------------------------------------------------ *
 *  Native DAOS API - Array
 * ------------------------------------------------------------------ */

int WRAPPER_NAME(daos_array_create)(daos_handle_t coh, daos_obj_id_t oid, daos_handle_t th, daos_size_t cell_size, daos_size_t chunk_size, daos_handle_t *oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_create, (coh, oid, th, cell_size, chunk_size, oh, ev));
    char **args = assemble_args_list(8, itoa(coh.cookie), itoa(oid.hi), itoa(oid.lo), itoa(th.cookie),
                                     itoa(cell_size), itoa(chunk_size), itoa((oh==NULL)?-1:oh->cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

int WRAPPER_NAME(daos_array_open)(daos_handle_t coh, daos_obj_id_t oid, daos_handle_t th, unsigned int mode, daos_size_t *cell_size, daos_size_t *chunk_size, daos_handle_t *oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_open, (coh, oid, th, mode, cell_size, chunk_size, oh, ev));
    char **args = assemble_args_list(9, itoa(coh.cookie), itoa(oid.hi), itoa(oid.lo), itoa(th.cookie), itoa(mode),
                                     itoa((cell_size==NULL)?-1:*cell_size), itoa((chunk_size==NULL)?-1:*chunk_size),
                                     itoa((oh==NULL)?-1:oh->cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

int WRAPPER_NAME(daos_array_open_with_attr)(daos_handle_t coh, daos_obj_id_t oid, daos_handle_t th, unsigned int mode, daos_size_t cell_size, daos_size_t chunk_size, daos_handle_t *oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_open_with_attr, (coh, oid, th, mode, cell_size, chunk_size, oh, ev));
    char **args = assemble_args_list(9, itoa(coh.cookie), itoa(oid.hi), itoa(oid.lo), itoa(th.cookie), itoa(mode),
                                     itoa(cell_size), itoa(chunk_size), itoa((oh==NULL)?-1:oh->cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

int WRAPPER_NAME(daos_array_read)(daos_handle_t oh, daos_handle_t th, daos_array_iod_t *iod, d_sg_list_t *sgl, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_read, (oh, th, iod, sgl, ev));
    char **args = assemble_args_list(5, itoa(oh.cookie), itoa(th.cookie), ptoa(iod), ptoa(sgl), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int WRAPPER_NAME(daos_array_write)(daos_handle_t oh, daos_handle_t th, daos_array_iod_t *iod, d_sg_list_t *sgl, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_write, (oh, th, iod, sgl, ev));
    char **args = assemble_args_list(5, itoa(oh.cookie), itoa(th.cookie), ptoa(iod), ptoa(sgl), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int WRAPPER_NAME(daos_array_get_size)(daos_handle_t oh, daos_handle_t th, daos_size_t *size, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_get_size, (oh, th, size, ev));
    char **args = assemble_args_list(4, itoa(oh.cookie), itoa(th.cookie), itoa((size==NULL)?-1:*size), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(daos_array_set_size)(daos_handle_t oh, daos_handle_t th, daos_size_t size, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_set_size, (oh, th, size, ev));
    char **args = assemble_args_list(4, itoa(oh.cookie), itoa(th.cookie), itoa(size), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(daos_array_stat)(daos_handle_t oh, daos_handle_t th, daos_array_stbuf_t *stbuf, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_stat, (oh, th, stbuf, ev));
    char **args = assemble_args_list(4, itoa(oh.cookie), itoa(th.cookie), ptoa(stbuf), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(daos_array_punch)(daos_handle_t oh, daos_handle_t th, daos_array_iod_t *iod, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_punch, (oh, th, iod, ev));
    char **args = assemble_args_list(4, itoa(oh.cookie), itoa(th.cookie), ptoa(iod), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int WRAPPER_NAME(daos_array_destroy)(daos_handle_t oh, daos_handle_t th, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_destroy, (oh, th, ev));
    char **args = assemble_args_list(3, itoa(oh.cookie), itoa(th.cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int WRAPPER_NAME(daos_array_close)(daos_handle_t oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_array_close, (oh, ev));
    char **args = assemble_args_list(2, itoa(oh.cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}


/* ------------------------------------------------------------------ *
 *  Native DAOS API - Key-Value
 * ------------------------------------------------------------------ */

int WRAPPER_NAME(daos_kv_open)(daos_handle_t coh, daos_obj_id_t oid, unsigned int mode, daos_handle_t *oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_open, (coh, oid, mode, oh, ev));
    char **args = assemble_args_list(6, itoa(coh.cookie), itoa(oid.hi), itoa(oid.lo), itoa(mode),
                                     itoa((oh==NULL)?-1:oh->cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int WRAPPER_NAME(daos_kv_get)(daos_handle_t oh, daos_handle_t th, uint64_t flags, const char *key, daos_size_t *size, void *buf, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_get, (oh, th, flags, key, size, buf, ev));
    char **args = assemble_args_list(7, itoa(oh.cookie), itoa(th.cookie), itoa(flags), strtoa(key),
                                     itoa((size==NULL)?-1:*size), ptoa(buf), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int WRAPPER_NAME(daos_kv_put)(daos_handle_t oh, daos_handle_t th, uint64_t flags, const char *key, daos_size_t size, const void *buf, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_put, (oh, th, flags, key, size, buf, ev));
    char **args = assemble_args_list(7, itoa(oh.cookie), itoa(th.cookie), itoa(flags), strtoa(key),
                                     itoa(size), ptoa(buf), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int WRAPPER_NAME(daos_kv_remove)(daos_handle_t oh, daos_handle_t th, uint64_t flags, const char *key, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_remove, (oh, th, flags, key, ev));
    char **args = assemble_args_list(5, itoa(oh.cookie), itoa(th.cookie), itoa(flags), strtoa(key), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int WRAPPER_NAME(daos_kv_list)(daos_handle_t oh, daos_handle_t th, uint32_t *nr, daos_key_desc_t *kds, d_sg_list_t *sgl, daos_anchor_t *anchor, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_list, (oh, th, nr, kds, sgl, anchor, ev));
    char **args = assemble_args_list(7, itoa(oh.cookie), itoa(th.cookie), itoa((nr==NULL)?-1:*nr),
                                     ptoa(kds), ptoa(sgl), ptoa(anchor), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int WRAPPER_NAME(daos_kv_destroy)(daos_handle_t oh, daos_handle_t th, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_destroy, (oh, th, ev));
    char **args = assemble_args_list(3, itoa(oh.cookie), itoa(th.cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int WRAPPER_NAME(daos_kv_close)(daos_handle_t oh, daos_event_t *ev) {
    RECORDER_INTERCEPTOR_PROLOGUE(int, daos_kv_close, (oh, ev));
    char **args = assemble_args_list(2, itoa(oh.cookie), ptoa(ev));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

#endif /* RECORDER_WITH_DAOS */
