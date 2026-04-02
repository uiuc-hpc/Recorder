#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <vector>
#include <algorithm>
extern "C" {
#include "reader.h"
#include "reader-private.h"
}
using namespace std;

// Allow setting the limit of number of
// conflicts we detect and save to file
// mainly used to accerlate the detection
// and verification process.
static int conflicts_cap = INT_MAX;

int compare_by_offset(const void *lhs, const void *rhs) {
    Interval *first = (Interval*)lhs;
    Interval *second = (Interval*)rhs;
    return first->offset - second->offset;
}

int compare_by_index(const void *lhs, const void *rhs) {
    Interval *first = (Interval*)lhs;
    Interval *second = (Interval*)rhs;

    if (first->rank < second->rank)
        return 1;
    else if (first->rank == second->rank)
        return first->seqId < second->seqId;
    else
        return 0;
}

int sum_array(int *arr, int count) {
    int i, sum = 0;
    for(i = 0; i < count; i++) {
        sum += arr[i];
    }
    return sum;
}

int is_conflict(Interval* i1, Interval* i2) {
    // TODO: same rank but multi-threaded?
    if(i1->rank == i2->rank)
        return false;
    if(i1->isRead && i2->isRead)
        return false;
    if(i1->offset+i1->count <= i2->offset)
        return false;
    if(i2->offset+i2->count <= i1->offset)
        return false;
    return true;
}

void detect_conflicts(IntervalsMap *IM, int num_files, const char* base_dir) {
    FILE* conflict_file;
    char path[512];
    size_t total_conflicts = 0;

    sprintf(path, "%s/conflicts.dat", base_dir);
    conflict_file = fopen(path, "w");

    int idx, i, j;
    for(idx = 0; idx < num_files; idx++) {

        char *filename = IM[idx].filename;
        Interval *intervals = IM[idx].intervals;
        //fprintf(conflict_file, "#%d:%s\n", idx, filename);

        // sort by offset
        qsort(intervals, IM[idx].num_intervals, sizeof(Interval), compare_by_offset);

        int i = 0, j = 0;
        Interval *i1, *i2;

        while(i < IM[idx].num_intervals-1) {
            i1 = &intervals[i];
            j = i + 1;

            vector<Interval*> conflicts;
            while(j < IM[idx].num_intervals) {
                i2 = &intervals[j];

                if(is_conflict(i1,i2)) {
                    conflicts.push_back(i2);
                }

                if(i1->offset+i1->count > i2->offset) {
                    j++;
                } else {
                    // skip the rest, as they will all have
                    // a bigger starting offset
                    break;
                }
            }

            size_t num_conflict_pairs = conflicts.size();
            if (num_conflict_pairs > 0) {
                total_conflicts += num_conflict_pairs;

                fwrite(&i1->rank, sizeof(int), 1, conflict_file);
                fwrite(&i1->seqId, sizeof(int), 1, conflict_file);
                fwrite(&num_conflict_pairs, sizeof(size_t), 1, conflict_file);

                // previously intervals were sorted by starting offset
                // when saving it out, we sort it by sequence id
                sort(conflicts.begin(), conflicts.end(), compare_by_index);

                // write out all conflict pairs
                for (vector<Interval*>::iterator it = conflicts.begin(); it!=conflicts.end(); ++it) {
                    i2 = *it;
                    fwrite(&i2->rank, sizeof(int), 1, conflict_file);
                    fwrite(&i2->seqId, sizeof(int), 1, conflict_file);
                }
                conflicts.clear();
            }

            // end of one starting operation
            // move on to the next one
            i++;
            if (total_conflicts > conflicts_cap)
                goto done;
        } // end of one file
    }

done:
    fclose(conflict_file);
}

int main(int argc, char* argv[]) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    if (argc == 3)
        conflicts_cap = atoi(argv[2]);

    int i, num_files;
    IntervalsMap *IM = build_offset_intervals(&reader, &num_files);

    detect_conflicts(IM, num_files, reader.logs_dir);

    // Free IM
    for(i = 0; i < num_files; i++) {
        free(IM[i].filename);
        free(IM[i].intervals);
    }
    free(IM);

    recorder_free_reader(&reader);
    return 0;
}
