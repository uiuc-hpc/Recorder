#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
extern "C" {                            // Needed to mix linking C and C++ sources
#include "reader.h"
}

using namespace std;

typedef struct RRecord_t {
    int rank;
    int seq_id;
    Record* record;
} RRecord;


RecorderReader *reader;
vector<RRecord> records;

static inline size_t str2sizet(char* arg) {
    size_t res;
    sscanf(arg, "%zu", &res);
    return res;
}

size_t get_eof(string filename, unordered_map<string, size_t> local_eof, unordered_map<string, size_t> global_eof) {
    size_t e1 = 0, e2 = 0;
    if(local_eof.find(filename) != local_eof.end())
        e1 = local_eof[filename];
    if(global_eof.find(filename) != global_eof.end())
        e2 = global_eof[filename];
    return max(e1, e2);
}

void handle_data_operation(RRecord &rr,
                            unordered_map<string, size_t> &offset_book,          // <fd, current offset>
                            unordered_map<string, size_t> &local_eof,            // <filename, eof> (locally)
                            unordered_map<string, size_t> &global_eof,           // <filename, eof> (globally)
                            unordered_map<string, vector<Interval>> &intervals,
                            string current_mpifh, int current_mpi_call_depth)
{
    Record *R = rr.record;
    const char* func = recorder_get_func_name(reader, R);

    if(!strstr(func, "read") && !strstr(func, "write"))
        return;

    Interval I;
    I.rank = rr.rank;
    I.seqId = rr.seq_id;
    I.tstart = R->tstart;
    I.isRead = strstr(func, "read") ? true: false;
    memset(I.mpifh, 0, sizeof(I.mpifh));
    strcpy(I.mpifh, "-");

    if(R->level == current_mpi_call_depth+1)
        strcpy(I.mpifh, current_mpifh.c_str());

    string filename = "";
    if(strstr(func, "writev") || strstr(func, "readv")) {
        filename = R->args[0];
        I.offset = offset_book[filename];
        I.count = str2sizet(R->args[1]);
        offset_book[filename] += I.count;
    } else if(strstr(func, "fwrite") || strstr(func, "fread")) {
        filename = R->args[3];
        I.offset = offset_book[filename];
        I.count = str2sizet(R->args[1]) * str2sizet(R->args[2]);
        offset_book[filename] += I.count;
    } else if(strstr(func, "pwrite") || strstr(func, "pread")) {
        filename = R->args[0];
        I.count = str2sizet(R->args[2]);
        I.offset = str2sizet(R->args[3]);
    } else if(strstr(func, "write") || strstr(func, "read")) {
        filename = R->args[0];
        I.count = str2sizet(R->args[2]);
        I.offset = offset_book[filename];
        offset_book[filename] += I.count;
    } else if(strstr(func, "fprintf")) {
        filename = R->args[0];
        I.count = str2sizet(R->args[1]);
        offset_book[filename] += I.count;
    }

    if(filename != "") {

        if(local_eof.find(filename) == local_eof.end())
            local_eof[filename] = I.offset + I.count;
        else
            local_eof[filename] = max(local_eof[filename], I.offset+I.count);

        /* TODO:
         * On POSIX systems, update global eof now
         * On other systems (e.,g with commit semantics
         * and session semantics), update global eof at
         * close/sync ? */
        global_eof[filename] = get_eof(filename, local_eof, global_eof);

        intervals[filename].push_back(I);
    }
}

/*
 * Inspect metadata operations to make
 * sure we can correctly keep track of
 * the position of file pointers.
 *
 * Params:
 *   offset_book: <filename, currect offset>
 *   local_eof:   <filename, eof>
 *   global_eof:  <filename, eof>
 */
void handle_metadata_operation(RRecord &rr,
                                unordered_map<string, size_t> &offset_book,
                                unordered_map<string, size_t> &local_eof,
                                unordered_map<string, size_t> &global_eof
                              )
{
    Record *R = rr.record;
    const char* func = recorder_get_func_name(reader, R);

    string filename = "";

    if(strstr(func, "fopen") || strstr(func, "fdopen")) {
        filename = R->args[0];
        offset_book[filename] = 0;

        // append
        if(strstr(R->args[1], "a"))
            offset_book[filename] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "open")) {
        filename = R->args[0];
        offset_book[filename] = 0;

        /* TODO: Do O_APPEND, SEEK_SET, ... have
         * the same value on this machine and the machine where
         * traces were collected? */
        int flag = atoi(R->args[1]);
        if(flag & O_APPEND)
            offset_book[filename] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "seek") || strstr(func, "seeko")) {
        filename = R->args[0];
        size_t offset = str2sizet(R->args[1]);
        int whence = atoi(R->args[2]);

        if(whence == SEEK_SET)
            offset_book[filename] = offset;
        else if(whence == SEEK_CUR)
            offset_book[filename] += offset;
        else if(whence == SEEK_END)
            offset_book[filename] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "close") || strstr(func, "sync")) {
        filename = R->args[0];

        // Update the global eof at close time
        global_eof[filename] = get_eof(filename, local_eof, global_eof);

        // Remove from the table
        if(strstr(func, "close"))
            offset_book.erase(filename);
    } else {
        return;
    }
}

bool compare_by_tstart(const RRecord &lhs, const RRecord &rhs) {
    return lhs.record->tstart < rhs.record->tstart;
}

static int current_seq_id = 0;

void insert_one_record(Record* r, void* arg) {

    /* increase seq id before the checks
     * so we always have the correct seq id
     * when we insert a record */
    current_seq_id++;

    int func_type = recorder_get_func_type(reader, r);
    const char* func = recorder_get_func_name(reader, r);

    if((func_type != RECORDER_POSIX) && (func_type != RECORDER_MPIIO))
        return;

    // For MPI-IO calls keep only MPI_File_write* and MPI_File_read*
    if((func_type == RECORDER_MPIIO) && (!strstr(func, "MPI_File_write")) 
        && (!strstr(func, "MPI_File_read")) && (!strstr(func, "MPI_File_iread"))
        && (!strstr(func, "MPI_File_iwrite")))
        return;
    
    if(strstr(func, "dir") || strstr(func, "link"))
        return;

    RRecord rr;
    rr.record = r;
    rr.rank = *((int*) arg);
    rr.seq_id = current_seq_id - 1;

    records.push_back(rr);
}

void flatten_and_sort_records(RecorderReader *reader) {

    int nprocs = reader->metadata.total_ranks;

    for(int rank = 0; rank < nprocs; rank++) {
        current_seq_id = 0;
        CST* cst;
        CFG* cfg;
        recorder_get_cst_cfg(reader, rank, &cst, &cfg);
        recorder_decode_records_core(reader, cst, cfg, insert_one_record, &rank, false);
    }

    sort(records.begin(), records.end(), compare_by_tstart);
}

/*
 * Return an array of <filename, intervals>
 * mapping. The length of this array will be
 * saved in 'num_files'.
 * caller is responsible for freeing space
 * after use.
 */
IntervalsMap* build_offset_intervals(RecorderReader *_reader, int *num_files) {

    reader = _reader;
    flatten_and_sort_records(reader);

    // <filename, intervals>
    unordered_map<string, vector<Interval>> intervals;

    unordered_map<string, size_t> offset_books[reader->metadata.total_ranks];
    unordered_map<string, size_t> local_eofs[reader->metadata.total_ranks];
    unordered_map<string, size_t> global_eof;

    string current_mpifh = "";
    int current_mpi_call_depth;

    int i;
    for(i = 0; i < records.size(); i++) {
        RRecord rr = records[i];
        const char* func = recorder_get_func_name(reader, rr.record);
        // Only MPI_File_write* and MPI_File_read* calls here
        // thanks to insert_one_record()
        if(strstr(func, "MPI")) {
            current_mpifh = rr.record->args[0];
            current_mpi_call_depth = (int) rr.record->level;
        // POSIX calls
        } else {
            handle_metadata_operation(rr, offset_books[rr.rank], local_eofs[rr.rank], global_eof);
            handle_data_operation(rr, offset_books[rr.rank], local_eofs[rr.rank], global_eof, intervals, current_mpifh, current_mpi_call_depth);
        }
    }

    /* Now we have the list of intervals for all files,
     * we copy it from the C++ vector to a C style pointer.
     * The vector and the memory it allocated will be
     * freed once out of scope (leave this function)
     * Also, using a C style struct is easier for Python binding.
     */
    *num_files = intervals.size();

    IntervalsMap *IM = (IntervalsMap*) malloc(sizeof(IntervalsMap) * (*num_files));

    i = 0;
    for(auto it = intervals.cbegin(); it != intervals.cend(); it++) {
        /* it->first: filename
         * it->second: vector<Interval> */
        IM[i].filename = strdup(it->first.c_str());
        IM[i].num_intervals = it->second.size();
        IM[i].intervals = (Interval*) malloc(sizeof(Interval) * it->second.size());
        memcpy(IM[i].intervals, it->second.data(), sizeof(Interval) * it->second.size());
        i++;
    }

    for(i = 0; i < records.size(); i++) {
        recorder_free_record(records[i].record);
    }
    return IM;
}
