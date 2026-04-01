/*
 * test_reader.c — validates that recorder_init_reader() correctly loads a
 * trace directory produced by the current Recorder version.
 *
 * Usage: test_reader <trace_dir>
 * Returns 0 if all checks pass, 1 if any check fails.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"

static void count_cb(Record *r, void *arg) {
    (*(int *)arg)++;
}

#define CHECK(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: " fmt "\n", ##__VA_ARGS__); \
            failed++; \
        } \
    } while (0)

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <trace_dir>\n", argv[0]);
        return 1;
    }

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    int failed = 0;

    /* Version stored in the trace must match the reader */
    CHECK(reader.metadata.version_major == RECORDER_VERSION_MAJOR &&
          reader.metadata.version_minor == RECORDER_VERSION_MINOR,
          "version mismatch: trace=%d.%d reader=%d.%d",
          reader.metadata.version_major, reader.metadata.version_minor,
          RECORDER_VERSION_MAJOR, RECORDER_VERSION_MINOR);

    /* Must have at least one process */
    CHECK(reader.metadata.total_ranks >= 1,
          "total_ranks=%d", reader.metadata.total_ranks);

    /* Function list must be populated */
    CHECK(reader.supported_funcs > 0,
          "supported_funcs=%d", reader.supported_funcs);

    /* The two sources of function count must agree */
    CHECK(reader.metadata.num_funcs == reader.supported_funcs,
          "num_funcs mismatch: metadata=%d supported_funcs=%d",
          reader.metadata.num_funcs, reader.supported_funcs);

    /* Spot-check: first 5 func_list entries must be non-empty strings */
    for (int i = 0; i < 5 && i < reader.supported_funcs; i++) {
        CHECK(reader.func_list[i] != NULL && reader.func_list[i][0] != '\0',
              "func_list[%d] is empty or NULL", i);
    }

    /* All test programs do POSIX I/O */
    CHECK(reader.metadata.posix_tracing,
          "posix_tracing is disabled");

    /* Must be able to decode at least one record for rank 0 */
    int count = 0;
    recorder_decode_records(&reader, 0, count_cb, &count);
    CHECK(count > 0, "no records decoded for rank 0");

    int funcs = reader.supported_funcs;
    recorder_free_reader(&reader);

    if (!failed)
        printf("PASS: ranks=%d funcs=%d records_rank0=%d\n",
               reader.metadata.total_ranks, funcs, count);

    return failed ? 1 : 0;
}
