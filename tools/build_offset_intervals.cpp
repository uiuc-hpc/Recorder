#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
extern "C" {                            // Needed to mix linking C and C++ sources
#include "reader.h"
}
using namespace std;

typedef struct RRecord_t {
    int rank;
    Record* record;
} RRecord;


inline int exclude_filename(const char *filename) {
    if (filename == NULL) return 0; // pass

    /* these are paths that we will not trace */
    // TODO put these in configuration file?
    static const char *exclusions[] = {"/dev/", "/proc", "/sys", "/etc", "/usr/tce/packages",
                        "pipe:[", "anon_inode:[", "socket:[", NULL};
    int i = 0;
    // Need to make sure both parameters for strncmp are not NULL, otherwise its gonna crash
    while(exclusions[i] != NULL) {
        int find = strncmp(exclusions[i], filename, strlen(exclusions[i]));
        if (find == 0)      // find it. should ignore this filename
            return 1;
        i++;
    }
    return 0;
}


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
void setup_open_close_commit(vector<RRecord> records, int nprocs, IntervalsMap *IM, int num_files, Semantics semantics) {
    int i, j, rank;
    unordered_map<string, vector<double>> topens[nprocs];
    unordered_map<string, vector<double>> tcloses[nprocs];
    unordered_map<string, vector<double>> tcommits[nprocs];
    unordered_map<int, string> filemaps[nprocs];


    for(i = 0; i < records.size(); i++) {
        RRecord rr = records[i];
        Record *R = rr.record;
        const char* func = func_list[R->func_id];

        int fd = -1;
        string filename;

        if(strstr(func, "fdopen")) {
            fd = R->res;
            int old_fd = atoi(R->args[0]);
            if(filemaps[rr.rank].find(old_fd) == filemaps[rr.rank].end())
                continue;
            filename = filemaps[rr.rank][old_fd];
        } else if(strstr(func, "open")) {   // fopen and open
            fd = R->res;
            filename = R->args[0];
        } else if(strstr(func, "close")) {
            fd = atoi(R->args[0]);
            if(filemaps[rr.rank].find(fd) == filemaps[rr.rank].end())
                continue;
        } else if(strstr(func, "sync")) {
            fd = atoi(R->args[0]);
            if(filemaps[rr.rank].find(fd) == filemaps[rr.rank].end())
                continue;
        }

        if(fd == -1) continue;
        if(strstr(func, "open")) {
            filemaps[rr.rank][fd] = filename;
            topens[rr.rank][filename].push_back(R->tstart);
        } else if(strstr(func, "close")) {
            filename = filemaps[rr.rank][fd];
            filemaps[rr.rank].erase(fd);
            tcloses[rr.rank][filename].push_back(R->tstart);
            if(semantics == POSIX_SEMANTICS || semantics == COMMIT_SEMANTICS)
                tcommits[rr.rank][filename].push_back(R->tstart);
        } else if(strstr(func, "sync")) {
            filename = filemaps[rr.rank][fd];
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
                            unordered_map<int, string> &filemap,                 // <fd, filename>
                            unordered_map<int, size_t> &offset_book,             // <fd, current offset>
                            unordered_map<string, size_t> &local_eof,            // <filename, eof> (locally)
                            unordered_map<string, size_t> &global_eof,           // <filename, eof> (globally)
                            unordered_map<string, vector<Interval>> &intervals,
                            Semantics semantics )
{
    Record *R = rr.record;
    const char* func = func_list[R->func_id];

    if(!strstr(func, "read") && !strstr(func, "write"))
        return;

    Interval I;
    I.rank = rr.rank;
    I.tstart = R->tstart;
    I.isRead = strstr(func, "read") ? true: false;

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
        if(semantics == POSIX_SEMANTICS)
            global_eof[filename] = get_eof(filename, local_eof, global_eof);

        intervals[filename].push_back(I);
    }
}


void handle_metadata_operation(RRecord &rr,
                                unordered_map<int, string> &filemap,                 // <fd, filename>
                                unordered_map<int, size_t> &offset_book,             // <fd, current offset>
                                unordered_map<string, size_t> &local_eof,            // <filename, eof> (locally)
                                unordered_map<string, size_t> &global_eof            // <filename, eof> (globally)
                                )
{

    int rank = rr.rank;
    Record *R = rr.record;
    const char* func = func_list[R->func_id];

    string filename;

    if(strstr(func, "fopen") || strstr(func, "fdopen")) {
        int fd = R->res;

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
        filename = R->args[0];

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
        filename = filemap[fd];

        if(whence == SEEK_SET)
            offset_book[fd] = offset;
        else if(whence == SEEK_CUR)
            offset_book[fd] += offset;
        else if(whence == SEEK_END)
            offset_book[fd] = get_eof(filename, local_eof, global_eof);

    } else if(strstr(func, "close") || strstr(func, "sync")) {
        int fd = atoi(R->args[0]);
        if(filemap.find(fd) == filemap.end()) return;
        filename = filemap[fd];
        // if close, remove from filemap.
        if(strstr(func, "close")) {
            filemap.erase(fd);
            offset_book.erase(fd);
        }

        // For all three semantics update the global eof
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

    int rank, j;
    for(rank = 0; rank < nprocs; rank++) {
        for(j = 0; j < reader.RLDs[rank].total_records; j++) {

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


IntervalsMap* build_offset_intervals(RecorderReader reader, int *num_files, enum Semantics semantics) {

    vector<RRecord> records = flatten_and_sort_records(reader);

    // Final result, all the folowing vectors will be cleaned automatically
    // once the function exited.
    unordered_map<string, vector<Interval>> intervals;                  // <filename, list of intervals>

    // Each rank maintains its own filemap and offset_book
    unordered_map<int, string> filemaps[reader.RGD.total_ranks];        // local <fd, filename> map
    unordered_map<int, size_t> offset_books[reader.RGD.total_ranks];    // local <fd, current offset> map
    unordered_map<string, size_t> local_eofs[reader.RGD.total_ranks];   // local <filename, end of file> map
    unordered_map<string, size_t> global_eof;                           // global <filename, end of file> map

    int i;
    for(i = 0; i < records.size(); i++) {

        RRecord rr = records[i];

        handle_metadata_operation(rr, filemaps[rr.rank], offset_books[rr.rank],
                                    local_eofs[rr.rank], global_eof);

        handle_data_operation(rr, filemaps[rr.rank], offset_books[rr.rank],
                                local_eofs[rr.rank], global_eof, intervals, semantics);
    }



    // Now we have the list of intervals for all files
    // We copy it from the vector to a C style pointer
    // The vector and the memory it allocated will be
    // freed once out of scope (leave this function)
    // Also, using a C style struct is easier for Python binding.
    *num_files = 0;
    for(auto it = intervals.cbegin(); it != intervals.cend(); it++) {
        if(!exclude_filename(it->first.c_str()))
            *num_files += 1;
    }

    IntervalsMap *IM = (IntervalsMap*) malloc(sizeof(IntervalsMap) * (*num_files));

    i = 0;
    for(auto it = intervals.cbegin(); it != intervals.cend(); it++) {
        if(exclude_filename(it->first.c_str()))
            continue;
        IM[i].filename = strdup(it->first.c_str());
        IM[i].num_intervals = it->second.size();
        IM[i].intervals = (Interval*) malloc(sizeof(Interval) * it->second.size());
        memcpy(IM[i].intervals, it->second.data(), sizeof(Interval) * it->second.size());
        i++;
    }

    // Fill in topens, tcloses and tcommits
    setup_open_close_commit(records, reader.RGD.total_ranks, IM, *num_files, semantics);

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
