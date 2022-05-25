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
int semantics;


size_t get_eof(string filename, unordered_map<string, size_t> local_eof, unordered_map<string, size_t> global_eof) {
    size_t e1 = 0, e2 = 0;
    if(local_eof.find(filename) != local_eof.end())
        e1 = local_eof[filename];
    if(global_eof.find(filename) != global_eof.end())
        e2 = global_eof[filename];
    return max(e1, e2);
}

/**
 * Build a list of timestamps of open, close and commit
 * This function must be called after intervals of IntervalsMap have been filled.
 * Also, records need to be sorted by tstart, which we did in build_offset_intervals().
 */
void setup_open_close_commit(int nprocs, IntervalsMap *IM, int num_files) {
    int i, j, rank;
    unordered_map<string, vector<double>> topens[nprocs];
    unordered_map<string, vector<double>> tcloses[nprocs];
    unordered_map<string, vector<double>> tcommits[nprocs];
    unordered_map<int, string> filemaps[nprocs];


    for(i = 0; i < records.size(); i++) {
        RRecord rr = records[i];
        Record *R = rr.record;
        const char* func = recorder_get_func_name(reader, R);

        string filename = "";

        if(strstr(func, "open")) {          // open, fopen, fdopen
            filename = R->args[0];
        } else if(strstr(func, "close")) {
            filename = R->args[0];
        } else if(strstr(func, "sync")) {
            filename = R->args[0];
        }

        if(filename == "") continue;

        if(strstr(func, "open")) {
            topens[rr.rank][filename].push_back(R->tstart);
        } else if(strstr(func, "close")) {
            tcloses[rr.rank][filename].push_back(R->tstart);
            if(semantics == POSIX_SEMANTICS || semantics == COMMIT_SEMANTICS)
                tcommits[rr.rank][filename].push_back(R->tstart);
        } else if(strstr(func, "sync")) {
            tcommits[rr.rank][filename].push_back(R->tstart);
        }
    }

    for(i = 0; i < num_files; i++) {
        string filename = IM[i].filename;
        IM[i].topens = (double**) malloc(sizeof(double*) * nprocs);
        IM[i].tcloses = (double**) malloc(sizeof(double*) * nprocs);
        IM[i].tcommits = (double**) malloc(sizeof(double*) * nprocs);

        IM[i].num_opens = (int*) malloc(sizeof(int) * nprocs);
        IM[i].num_closes = (int*) malloc(sizeof(int) * nprocs);
        IM[i].num_commits = (int*) malloc(sizeof(int) * nprocs);

        for(rank = 0; rank < nprocs; rank++) {

            IM[i].num_opens[rank] = topens[rank][filename].size();
            IM[i].num_closes[rank] = tcloses[rank][filename].size();
            IM[i].num_commits[rank] = tcommits[rank][filename].size();


            IM[i].topens[rank] = (double*) malloc(sizeof(double) * IM[i].num_opens[rank]);
            IM[i].tcloses[rank] = (double*) malloc(sizeof(double) * IM[i].num_closes[rank]);
            IM[i].tcommits[rank] = (double*) malloc(sizeof(double) * IM[i].num_commits[rank]);

            for(j = 0; j < IM[i].num_opens[rank]; j++)
                IM[i].topens[rank][j] = topens[rank][filename][j];
            for(j = 0; j < IM[i].num_closes[rank]; j++)
                IM[i].tcloses[rank][j] = tcloses[rank][filename][j];
            for(j = 0; j < IM[i].num_commits[rank]; j++)
                IM[i].tcommits[rank][j] = tcommits[rank][filename][j];
        }
    }
}

void handle_data_operation(RRecord &rr,
                            unordered_map<string, size_t> &offset_book,          // <fd, current offset>
                            unordered_map<string, size_t> &local_eof,            // <filename, eof> (locally)
                            unordered_map<string, size_t> &global_eof,           // <filename, eof> (globally)
                            unordered_map<string, vector<Interval>> &intervals)
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

    string filename = "";
    if(strstr(func, "writev") || strstr(func, "readv")) {
        filename = R->args[0];
        I.offset = offset_book[filename];
        I.count = atol(R->args[1]);
        offset_book[filename] += I.count;
    } else if(strstr(func, "fwrite") || strstr(func, "fread")) {
        filename = R->args[3];
        I.offset = offset_book[filename];
        I.count = atol(R->args[1]) * atol(R->args[2]);
        offset_book[filename] += I.count;
    } else if(strstr(func, "pwrite") || strstr(func, "pread")) {
        filename = R->args[0];
        I.count = atol(R->args[2]);
        I.offset = atol(R->args[3]);
    } else if(strstr(func, "write") || strstr(func, "read")) {
        filename = R->args[0];
        I.count = atol(R->args[2]);
        I.offset = offset_book[filename];
        offset_book[filename] += I.count;
    } else if(strstr(func, "fprintf")) {
        filename = R->args[0];
        I.count = atol(R->args[1]);
        offset_book[filename] += I.count;
    }

    if(filename != "") {

        if(local_eof.find(filename) == local_eof.end())
            local_eof[filename] = I.offset + I.count;
        else
            local_eof[filename] = max(local_eof[filename], I.offset+I.count);

        // On POSIX semantics, update global eof now
        // On Commit semantics and Session semantics, update global eof at close/sync
        if(semantics == POSIX_SEMANTICS)
            global_eof[filename] = get_eof(filename, local_eof, global_eof);

        intervals[filename].push_back(I);
    }
}

void handle_metadata_operation(RRecord &rr,
                                unordered_map<string, size_t> &offset_book,          // <filename, current offset>
                                unordered_map<string, size_t> &local_eof,            // <filename, eof> (locally)
                                unordered_map<string, size_t> &global_eof            // <filename, eof> (globally)
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

        // append
        int flag = atoi(R->args[1]);
        if(flag & O_APPEND)
            offset_book[filename] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "seek") || strstr(func, "seeko")) {
        filename = R->args[0];
        int offset = atol(R->args[1]);
        int whence = atoi(R->args[2]);

        if(whence == SEEK_SET)
            offset_book[filename] = offset;
        else if(whence == SEEK_CUR)
            offset_book[filename] += offset;
        else if(whence == SEEK_END)
            offset_book[filename] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "close") || strstr(func, "sync")) {
        filename = R->args[0];

        // if close, remove from table
        if(strstr(func, "close"))
            offset_book.erase(filename);

        // For all three semantics update the global eof
        global_eof[filename] = get_eof(filename, local_eof, global_eof);
    } else {
        return;
    }
}


bool compare_by_tstart(const RRecord &lhs, const RRecord &rhs) {
    return lhs.record->tstart < rhs.record->tstart;
}

static int current_seq_id = 0;

void insert_one_record(Record* r, void* arg) {

    current_seq_id++;

    bool user_func = (r->func_id == RECORDER_USER_FUNCTION);
    if(user_func) return;

    const char* func = recorder_get_func_name(reader, r);
    if(strstr(func,"MPI") || strstr(func, "H5") || strstr(func, "dir") || strstr(func, "link"))
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
        CST cst;
        CFG cfg;
        recorder_read_cst(reader, rank, &cst);
        recorder_read_cfg(reader, rank, &cfg);

        current_seq_id = 0;
        recorder_decode_records_core(reader, &cst, &cfg, insert_one_record, &rank, false);

        recorder_free_cst(&cst);
        recorder_free_cfg(&cfg);
    }

    sort(records.begin(), records.end(), compare_by_tstart);
}

IntervalsMap* build_offset_intervals(RecorderReader *_reader, int semantics, int *num_files) {

    reader = _reader;
    flatten_and_sort_records(reader);

    // Final result, all the folowing vectors will be cleaned automatically
    // once the function exited.
    unordered_map<string, vector<Interval>> intervals;                           // <filename, list of intervals>

    unordered_map<string, size_t> offset_books[reader->metadata.total_ranks];    // local <filename, current offset> map
    unordered_map<string, size_t> local_eofs[reader->metadata.total_ranks];      // local <filename, end of file> map
    unordered_map<string, size_t> global_eof;                                    // global <filename, end of file> map

    int i;
    for(i = 0; i < records.size(); i++) {
        RRecord rr = records[i];
        handle_metadata_operation(rr, offset_books[rr.rank], local_eofs[rr.rank], global_eof);
        handle_data_operation(rr, offset_books[rr.rank], local_eofs[rr.rank], global_eof, intervals);
    }

    // Now we have the list of intervals for all files
    // We copy it from the vector to a C style pointer
    // The vector and the memory it allocated will be
    // freed once out of scope (leave this function)
    // Also, using a C style struct is easier for Python binding.
    *num_files = intervals.size();

    IntervalsMap *IM = (IntervalsMap*) malloc(sizeof(IntervalsMap) * (*num_files));

    i = 0;
    for(auto it = intervals.cbegin(); it != intervals.cend(); it++) {
        IM[i].filename = strdup(it->first.c_str());
        IM[i].num_intervals = it->second.size();
        IM[i].intervals = (Interval*) malloc(sizeof(Interval) * it->second.size());
        memcpy(IM[i].intervals, it->second.data(), sizeof(Interval) * it->second.size());
        i++;
    }

    // Fill in topens, tcloses and tcommits
    setup_open_close_commit(reader->metadata.total_ranks, IM, *num_files);

    for(i = 0; i < records.size(); i++) {
        recorder_free_record(records[i].record);
    }
    return IM;
}
