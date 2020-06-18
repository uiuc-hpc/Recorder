#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "reader.h"

using namespace std;

typedef struct Interval_t {
    int rank;
    size_t offset;
    size_t count;
    bool isRead;
    int local_segment;
    vector<int> remote_segments;
} Interval;

typedef struct RRecord_t {
    int rank;
    Record* record;
} RRecord;

typedef struct Segment_t {
    int rank;
    int id;
    bool closed;

    Segment_t(int _rank, int _id, bool _closed) {
        rank = _rank;
        id = _id;
        closed = _closed;
    }
} Segment;


void print_records(vector<Record*> records) {
    for(int i = 0; i < records.size(); i++) {
        Record *r = records[i];
        cout<<"args: ";
        for(int j = 0; j < r->arg_count; j++)
            cout<<r->args[j]<<" ";
        cout<<endl;
    }
}


void handle_data_operation(RRecord *rr,
                           unordered_map<int, string> &filemap,              // fd -> filename
                           unordered_map<int, size_t> &offsetBook,           // fd -> offset
                           unordered_map<string, size_t> &localEndOfFile,    // filename -> file end offset (globally)
                           unordered_map<string, vector<Interval*>> &intervals
                          )
{
    int rank = rr->rank;
    Record *R = rr->record;
    const char* func = func_list[R->func_id];

    Interval *I = new Interval();
    if(strstr(func, "read"))
        I->isRead = true;
    else
        I->isRead = false;

    int fd;
    if(strstr(func, "writev") || strstr(func, "readv")) {
        fd = atoi(R->args[0]);
        I->offset = offsetBook[fd];
        I->count = atoi(R->args[1]);
        offsetBook[fd] += I->count;
    } else if(strstr(func, "fwrite") || strstr(func, "fread")) {
        fd = atoi(R->args[3]);
        I->offset = offsetBook[fd];
        I->count = atoi(R->args[1]) * atoi(R->args[2]);
        offsetBook[fd] += I->count;
    } else if(strstr(func, "pwrite") || strstr(func, "pread")) {
        fd = atoi(R->args[0]);
        I->count = atoi(R->args[2]);
        I->offset = atoi(R->args[3]);
    } else if(strstr(func, "write") || strstr(func, "read")) {
        fd = atoi(R->args[0]);
        I->count = atoi(R->args[2]);
        I->offset = offsetBook[fd];
        offsetBook[fd] += I->count;
    } else if(strstr(func, "fprintf")) {
        fd = atoi(R->args[0]);
        I->count = atoi(R->args[1]);
        offsetBook[fd] += I->count;
    }


    if(filemap.find(fd) != filemap.end()) {
        string filename = filemap[fd];
        localEndOfFile[filename] = max(localEndOfFile[filename], I->offset+I->count);
        intervals[filename].push_back(I);
    }

}

void handle_metadata_operation(RRecord* rr,
                               unordered_map<int, string> &filemap,              // fd -> filename
                               unordered_map<int, size_t> &offsetBook,           // fd -> offset
                               unordered_map<string, size_t> &localEndOfFile,    // filename -> file end offset (globally)
                               unordered_map<string, size_t> &globalEndOfFile,   // filename -> file end offset (on local rank)
                               unordered_map<string, vector<Segment>> &segmentBook
                               )
{
    int rank = rr->rank;
    Record *R = rr->record;
    const char* func = func_list[R->func_id];

    if(strstr(func, "fopen")) {
        int fd = R->res;
        string filename = R->args[0];

        filemap[fd] = filename;
        offsetBook[fd] = 0;
        if(strstr(R->args[1], "a"))   // append
            offsetBook[fd] = max(localEndOfFile[filename], globalEndOfFile[filename]);

        int new_segment_id = 0;
        if(segmentBook[filename].size() > 0)
            new_segment_id = segmentBook[filename].back().id + 1;       // last one
        Segment s(rank, new_segment_id, false);
        segmentBook[filename].push_back(s);
    } else if(strstr(func, "fdopen")) {
        // TODO
        // close the existing fd (args[0]) if opened
        // R->res is the new fd

    } else if(strstr(func, "open")) {
        int fd = R->res;
        string filename = R->args[0];

        filemap[fd] = filename;
        offsetBook[fd] = 0;

        // TODO append

        int new_segment_id = 0;
        if(segmentBook[filename].size() > 0)
            new_segment_id = segmentBook[filename].back().id + 1;       // last one
        Segment s(rank, new_segment_id, false);
        segmentBook[filename].push_back(s);
    } else if(strstr(func, "seek")) {
        int fd = atoi(R->args[0]);
        int offset = atoi(R->args[1]);
        int whence = atoi(R->args[2]);
        if(filemap.find(fd) == filemap.end()) return;
        string filename = filemap[fd];

        if(whence == SEEK_SET)
            offsetBook[fd] = offset;
        else if(whence == SEEK_CUR)
            offsetBook[fd] += offset;
        else if(whence == SEEK_END)
            offsetBook[fd] = max(localEndOfFile[filename], globalEndOfFile[filename]);
    } else if(strstr(func, "close") || strstr(func, "sync")) {
        int fd = atoi(R->args[0]);
        if(filemap.find(fd) == filemap.end()) return;
        string filename = filemap[fd];

        // 1. close all segments on the local process for this file
        for(int i = 0; i < segmentBook[filename].size(); i++) {
            if(segmentBook[filename][i].rank == rank)
                segmentBook[filename][i].closed = true;
        }
        // 2. start a new segment for all other processes have the same file opened.
        // skip this step for Session Semantics
        int new_segment_id = segmentBook[filename].back().id + 1;       // largest segment id
        unordered_set<int> visited_ranks;

        vector<Segment> new_segs;
        for(int i = 0; i < segmentBook[filename].size(); i++) {
            Segment seg = segmentBook[filename][i];

            if(visited_ranks.find(seg.rank) != visited_ranks.end())     // already visited
                continue;

            if(seg.rank != rank && !seg.closed) {
                Segment new_seg(seg.rank, new_segment_id++, false);
                new_segs.push_back(new_seg);
                visited_ranks.insert(seg.rank);
            }
        }
        segmentBook[filename].insert(segmentBook[filename].end(), new_segs.begin(), new_segs.end());
    }

}

bool compare_by_tstart(const RRecord *lhs, const RRecord *rhs) {
    return lhs->record->tstart < rhs->record->tstart;
}

unordered_map<string, vector<Interval*>> build_offset_list(RecorderReader reader) {

    vector<RRecord*> records;

    for(int rank = 0; rank < reader.records.size(); rank++) {
        vector<Record*> rs = reader.records[rank];
        for(int i = 0; i < rs.size(); i++) {
            RRecord *rr = (RRecord*) malloc(sizeof(RRecord));
            rr->rank = rank;
            rr->record = rs[i];
            records.push_back(rr);
        }
    }

    cout<<"total size:"<<records.size()<<endl;
    sort(records.begin(), records.end(), compare_by_tstart);

    // Each rank maintains own filemap, offsetBook
    unordered_map<int, string> filemaps[reader.RGD.total_ranks];
    unordered_map<int, size_t> offsetBooks[reader.RGD.total_ranks];
    unordered_map<string, size_t> localEndOfFiles[reader.RGD.total_ranks];


    unordered_map<string, size_t> globalEndOfFile;
    unordered_map<string, vector<Segment>> segmentBook;

    unordered_map<string, vector<Interval*>> intervals;

    for(int i = 0; i < records.size(); i++) {
        RRecord *rr = records[i];
        const char* func = func_list[rr->record->func_id];
        if(strstr(func,"MPI") || strstr(func, "H5") || strstr(func, "dir") || strstr(func, "link"))
            continue;

        handle_metadata_operation(rr, filemaps[rr->rank], offsetBooks[rr->rank],
                localEndOfFiles[rr->rank], globalEndOfFile, segmentBook);

        if(strstr(func, "read") || strstr(func, "write")) {
            handle_data_operation(rr, filemaps[rr->rank], offsetBooks[rr->rank],
                    localEndOfFiles[rr->rank], intervals);
        }
    }
    cout<<"intervals: "<<intervals.size()<<endl;
    return intervals;
}

void access_patterns(unordered_map<string, vector<Interval*>> intervals) {
    int consecqutive = 0; int sequential = 0; int random = 0;

    for(auto iter=intervals.begin(); iter!=intervals.end(); ++iter) {
        string filename = iter->first;
        vector<Interval*> is = iter->second;
        cout<<filename<<" "<<is.size()<<endl;
        for(int i = 0; i < is.size()-1; i++) {
            Interval* i1 = is[i];
            Interval* i2 = is[i+1];
            if(i1->offset+i1->count == i2->offset)
                consecqutive++;
            else if(i1->offset+i1->count < i2->offset)
                sequential++;
            else
                random++;
        }
    }

    cout<<"consecutive:"<<consecqutive<<", sequential:"<<sequential<<", random:"<<random;
}

bool compare_by_offset(const Interval *lhs, const Interval *rhs) {
    return lhs->offset < rhs->offset;
}
void conflicts(unordered_map<string, vector<Interval*>> intervals) {
    for(auto iter=intervals.begin(); iter!=intervals.end(); ++iter) {
        string filename = iter->first;
        vector<Interval*> is = iter->second;
        cout<<filename<<" "<<is.size()<<endl;

        sort(is.begin(), is.end(), compare_by_offset);
        for(int i = 0; i < is.size()-1; i++) {
            Interval i1 = is[i];
            Interval i2 = is[i+1];

            // no conflicts
            if(i1->offset+i1->count <= i2->offset)
                continue;
        }
    }
}


int main(int argc, char* argv[]) {
    char path[128];
    sprintf(path, "%s/recorder.mt", argv[1]);

    RecorderReader reader;

    read_global_metadata(path, &(reader.RGD));

    for(int rank =0; rank < reader.RGD.total_ranks; rank++) {
        RecorderLocalDef RLD;
        sprintf(path, "%s/%d.mt", argv[1], rank);
        read_local_metadata(path, &RLD);
        reader.RLDs.push_back(RLD);

        sprintf(path, "%s/%d.itf", argv[1], rank);
        vector<Record*> records = read_records(path);
        decompress_records(records);
        cout<<"rank: "<<rank<<", size: "<<records.size()<<endl;
        reader.records.push_back(records);
    }


    unordered_map<string, vector<Interval*>> intervals = build_offset_list(reader);

    access_patterns(intervals);
    //conflicts(intervals);

    return 0;
}
