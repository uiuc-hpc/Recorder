#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "reader.h"


using namespace std;

static bool posix_semantics = true;

typedef struct RRecord_t {
    int rank;
    Record* record;
} RRecord;


size_t get_eof(string filename, unordered_map<string, size_t> local_eof, unordered_map<string, size_t> global_eof) {
    size_t e1 = 0, e2 = 0;
    if(local_eof.find(filename) != local_eof.end())
        e1 = local_eof[filename];
    if(global_eof.find(filename) != global_eof.end())
        e2 = global_eof[filename];
    return max(e1, e2);
}

void handle_data_operation(RRecord &rr,
                           unordered_map<int, string> &filemap,                 // <fd, filename>
                           unordered_map<int, size_t> &offset_book,             // <fd, current offset>
                           unordered_map<string, size_t> &local_eof,            // <filename, eof> (locally)
                           unordered_map<string, size_t> &global_eof,           // <filename, eof> (globally)
                           unordered_map<string, vector<Interval>> &intervals
                          )
{
    int rank = rr.rank;
    Record *R = rr.record;
    const char* func = func_list[R->func_id];

    if(!strstr(func, "read") && !strstr(func, "write"))
        return;

    Interval I;
    strstr(func, "read") ? I.isRead = true : I.isRead = false;

    int fd;
    if(strstr(func, "writev") || strstr(func, "readv")) {
        fd = atoi(R->args[0]);
        I.offset = offset_book[fd];
        I.count = atoi(R->args[1]);
        offset_book[fd] += I.count;
    } else if(strstr(func, "fwrite") || strstr(func, "fread")) {
        fd = atoi(R->args[3]);
        I.offset = offset_book[fd];
        I.count = atoi(R->args[1]) * atoi(R->args[2]);
        offset_book[fd] += I.count;
    } else if(strstr(func, "pwrite") || strstr(func, "pread")) {
        fd = atoi(R->args[0]);
        I.count = atoi(R->args[2]);
        I.offset = atoi(R->args[3]);
    } else if(strstr(func, "write") || strstr(func, "read")) {
        fd = atoi(R->args[0]);
        I.count = atoi(R->args[2]);
        I.offset = offset_book[fd];
        offset_book[fd] += I.count;
    } else if(strstr(func, "fprintf")) {
        fd = atoi(R->args[0]);
        I.count = atoi(R->args[1]);
        offset_book[fd] += I.count;
    }

    if(filemap.find(fd) != filemap.end()) {
        string filename = filemap[fd];

        if(local_eof.find(filename) == local_eof.end())
            local_eof[filename] = I.offset+I.count;
        else
            local_eof[filename] = max(local_eof[filename], I.offset+I.count);

        // On POSIX semantics, update global eof now
        // On Commit semantics and Session semantics, update global eof at close/sync
        if(posix_semantics)
            global_eof[filename] = get_eof(filename, local_eof, global_eof);

        intervals[filename].push_back(I);
    }
}


void handle_metadata_operation(RRecord &rr,
                               unordered_map<int, string> &filemap,         // <fd, filename>
                               unordered_map<int, size_t> &offset_book,     // <fd, current offset>
                               unordered_map<string, size_t> &local_eof,    // <filename, end of file> (locally)
                               unordered_map<string, size_t> &global_eof    // <filename, end of file> (globally)
                               )
{
    int rank = rr.rank;
    Record *R = rr.record;
    const char* func = func_list[R->func_id];

    if(strstr(func, "fopen") || strstr(func, "fdopen")) {
        int fd = R->res;
        string filename;

        if(strstr(func, "fdopen")) {
            int old_fd = atoi(R->args[0]);
            if(filemap.find(old_fd) == filemap.end()) return;
            filename = filemap[old_fd];
        } else {
            filename = R->args[0];
        }

        filemap[fd] = filename;

        offset_book[fd] = 0;
        if(strstr(R->args[1], "a"))   // append
            offset_book[fd] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "open")) {
        int fd = R->res;
        string filename = R->args[0];

        filemap[fd] = filename;
        offset_book[fd] = 0;

        // append
        int flag = atoi(R->args[1]);
        if(flag & O_APPEND)
            offset_book[fd] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "seek")) {
        int fd = atoi(R->args[0]);
        int offset = atoi(R->args[1]);
        int whence = atoi(R->args[2]);
        if(filemap.find(fd) == filemap.end()) return;
        string filename = filemap[fd];

        if(whence == SEEK_SET)
            offset_book[fd] = offset;
        else if(whence == SEEK_CUR)
            offset_book[fd] += offset;
        else if(whence == SEEK_END)
            offset_book[fd] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "close") || strstr(func, "sync")) {
        int fd = atoi(R->args[0]);
        if(filemap.find(fd) == filemap.end()) return;
        string filename = filemap[fd];
        // if close, remove from filemap.
        if(strstr(func, "close")) {
            filemap.erase(fd);
            offset_book.erase(fd);
        }

        // For commit semantics and session semantics, update the global eof
        global_eof[filename] = get_eof(filename, local_eof, global_eof);
    }
}


bool compare_by_tstart(const RRecord &lhs, const RRecord &rhs) {
    return lhs.record->tstart < rhs.record->tstart;
}

/**
 * Flatten reader.records, which is an array of array
 * reader.records[rank] is a array of records for rank.
 *
 * After flatten the records, sort them by tstart
 *
 */
vector<RRecord> flatten_and_sort_records(RecorderReader reader) {
    vector<RRecord> records;

    int nprocs = reader.RGD.total_ranks;

    int i = 0;
    for(int rank = 0; rank < nprocs; rank++) {
        for(int j = 0; j < reader.RLDs[rank].total_records; j++) {

            Record *r = &(reader.records[rank][j]);
            const char* func = func_list[r->func_id];
            if(strstr(func,"MPI") || strstr(func, "H5") || strstr(func, "dir") || strstr(func, "link"))
                continue;

            RRecord rr;
            rr.rank = rank;
            rr.record = r;

            records.push_back(rr);
        }
    }

    sort(records.begin(), records.end(), compare_by_tstart);
    return records;
}


IntervalsMap* build_offset_intervals(RecorderReader reader, int *num_files) {

    vector<RRecord> records = flatten_and_sort_records(reader);
    printf("total records: %ld\n", records.size());

    // Final result, all the folowing vectors will be cleaned automatically
    // once the function finished.
    unordered_map<string, vector<Interval>> intervals;                  // <filename, list of intervals>

    // Each rank maintains its own filemap and offset_book
    unordered_map<int, string> filemaps[reader.RGD.total_ranks];        // local <fd, filename> map
    unordered_map<int, size_t> offset_books[reader.RGD.total_ranks];    // local <fd, current offset> map
    unordered_map<string, size_t> local_eofs[reader.RGD.total_ranks];   // local <filename, end of file> map
    unordered_map<string, size_t> global_eof;                           // global <filename, end of file> map


    for(int i = 0; i < records.size(); i++) {

        RRecord rr = records[i];

        handle_metadata_operation(rr, filemaps[rr.rank], offset_books[rr.rank],
                                    local_eofs[rr.rank], global_eof);

        handle_data_operation(rr, filemaps[rr.rank], offset_books[rr.rank],
                                local_eofs[rr.rank], global_eof, intervals);
    }


    // Now we have the list of intervals for all files
    // We copy it from the vector to a C style pointer
    // Because the vector and the memory it allocated will be
    // freed once out of scope (leave this function)
    // Also, using a C style struct is easier for Python binding.
    *num_files = intervals.size();
    IntervalsMap *IM= (IntervalsMap*) malloc(sizeof(IntervalsMap) * (*num_files));

    int i = 0;
    for(auto it = intervals.cbegin(); it != intervals.cend(); it++) {
        IM[i].filename = strdup(it->first.c_str());
        IM[i].num_intervals = it->second.size();
        IM[i].intervals = (Interval*) malloc(sizeof(Interval) * it->second.size());
        memcpy(IM[i].intervals, it->second.data(), sizeof(Interval) * it->second.size());
        i++;
    }

    return IM;
}

/*
int main(int argc, char* argv[]) {
    RecorderReader reader;
    int num_files;
    recorder_read_traces(argv[1], &reader);

    build_offset_intervals(reader, &num_files);

    return 0;
}
*/
