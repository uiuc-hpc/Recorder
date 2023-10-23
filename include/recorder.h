#ifndef __RECORDER_H
#define __RECORDER_H

#include "recorder-utils.h"
#include "recorder-logger.h"
#include "recorder-gotcha.h"
#include "recorder-pattern-recognition.h"

/* List of runtime environment variables */
#define RECORDER_WITH_NON_MPI       		        "RECORDER_WITH_NON_MPI"
#define RECORDER_TRACES_DIR         		        "RECORDER_TRACES_DIR"
#define RECORDER_BUFFER_SIZE        		        "RECORDER_BUFFER_SIZE"
#define RECORDER_TIME_RESOLUTION    		        "RECORDER_TIME_RESOLUTION"
#define RECORDER_LOG_POINTER        		        "RECORDER_LOG_POINTER"
#define RECORDER_LOG_TID            		        "RECORDER_LOG_TID"
#define RECORDER_LOG_LEVEL          		        "RECORDER_LOG_LEVEL"
#define RECORDER_INTERPROCESS_COMPRESSION	        "RECORDER_INTERPROCESS_COMPRESSION"
#define RECORDER_INTERPROCESS_PATTERN_RECOGNITION   "RECORDER_INTERPROCESS_RECOGNITION"
#define RECORDER_INTRAPROCESS_PATTERN_RECOGNITION   "RECORDER_INTRAPROCESS_RECOGNITION"
#define RECORDER_EXCLUSION_FILE     		        "RECORDER_EXCLUSION_FILE"
#define RECORDER_INCLUSION_FILE     		        "RECORDER_INCLUSION_FILE"

/* 
 * Allowing users to exclude the interception
 * of certain layers at runtime.
 *
 * e.g., export RECORDER_MPIIO_TRACING=0
 */
#define RECORDER_POSIX_TRACING              "RECORDER_POSIX_TRACING"
#define RECORDER_MPIIO_TRACING              "RECORDER_MPIIO_TRACING"
#define RECORDER_MPI_TRACING                "RECORDER_MPI_TRACING"
#define RECORDER_HDF5_TRACING               "RECORDER_HDF5_TRACING"


/**
 * I/O Interceptor
 * Phase 1:
 *
 * we intercept functions (e.g., from recorder-posix.c) and then
 * call this interception funciton.
 *
 * Here, we first run the original function so we can get the ouput
 * parameters correctly.
 *
 * We also construct a [struct Record] for each function. But latter we
 * can change the fields, e.g., fopen will convert the FILE* to an integer res.
 *
 */
#define RECORDER_INTERCEPTOR_PROLOGUE_CORE(ret, func, real_args)                    \
    Record *record = recorder_malloc(sizeof(Record));                               \
    record->func_id = get_function_id_by_name(#func);                               \
    record->tid = recorder_gettid();                                                \
    logger_record_enter(record);                                                    \
    record->tstart = recorder_wtime();                                              \
    GOTCHA_SET_REAL_CALL_NOCHECK(func);                                             \
    ret res = GOTCHA_REAL_CALL(func) real_args ;                                    \
    record->tend = recorder_wtime();

// Fortran wrappers call this
// ierr is of type MPI_Fint*, set only for fortran calls
#define RECORDER_INTERCEPTOR_PROLOGUE_F(ret, func, real_args, ierr)                 \
    if(!logger_initialized()) {                                                     \
        GOTCHA_SET_REAL_CALL_NOCHECK(func);                                         \
        ret res = GOTCHA_REAL_CALL(func) real_args ;                                \
        if ((ierr) != NULL) { *(ierr) = res; }                                      \
        return res;                                                                 \
    }                                                                               \
    RECORDER_INTERCEPTOR_PROLOGUE_CORE(ret, func, real_args)                        \
    if ((ierr) != NULL) { *(ierr) = res; }

// C wrappers call this
#define RECORDER_INTERCEPTOR_PROLOGUE(ret, func, real_args)                         \
    if(!logger_initialized()) {                                                     \
        GOTCHA_SET_REAL_CALL_NOCHECK(func);                                         \
        ret res = GOTCHA_REAL_CALL(func) real_args ;                                \
        return res;                                                                 \
    }                                                                               \
    RECORDER_INTERCEPTOR_PROLOGUE_CORE(ret, func, real_args)

/**
 * I/O Interceptor
 * Phase 2:
 *
 * Set other fields of the record, i.e, arg_count and args.
 * Finally write out the record
 *
 */
#define RECORDER_INTERCEPTOR_EPILOGUE(record_arg_count, record_args)                \
    record->arg_count = record_arg_count;                                           \
    record->args = record_args;                                                     \
    logger_record_exit(record);                                                     \
    return res;

#endif /* __RECORDER_H */
