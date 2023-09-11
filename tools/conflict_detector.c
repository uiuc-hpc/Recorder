#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "reader.h"

int compare_by_offset(const void *lhs, const void *rhs) {
    Interval *first = (Interval*)lhs;
    Interval *second = (Interval*)rhs;
    return first->offset - second->offset;
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
    sprintf(path, "%s/conflicts.txt", base_dir);
    conflict_file = fopen(path, "w");
    fprintf(conflict_file, "#rank,id,op1(fh,offset,count) "
                           "rank,id,op1(fh,offset,count)\n");

    int idx, i, j;
    for(idx = 0; idx < num_files; idx++) {

        char *filename = IM[idx].filename;
        Interval *intervals = IM[idx].intervals;
        fprintf(conflict_file, "#%d:%s\n", idx, filename);

        // sort by offset
        qsort(intervals, IM[idx].num_intervals, sizeof(Interval), compare_by_offset);

        int i = 0, j = 0;
        Interval *i1, *i2;
        while(i < IM[idx].num_intervals-1) {

            i1 = &intervals[i];
            j = i + 1;

            int conflict_count = 0;

            while(j < IM[idx].num_intervals) {
                i2 = &intervals[j];

                if(is_conflict(i1,i2)) {
                    fprintf(conflict_file, "%d,%d,%s(%s,%zu,%zu) %d,%d,%s(%s,%zu,%zu)\n",
                            i1->rank,i1->seqId,i1->isRead?"read":"write",i1->mpifh,i1->offset,i1->count,
                            i2->rank,i2->seqId,i2->isRead?"read":"write",i2->mpifh,i2->offset,i2->count);
                    conflict_count++;
                }

                if(i1->offset+i1->count > i2->offset) {
                    j++;
                } else {
                    // print out the number of conflicts
                    if (conflict_count > 0) {
                        fprintf(stdout, "rank:%d, id:%d, %s(%s,%zu,%zu) conflicts: %d\n",
                                i1->rank,i1->seqId,i1->isRead?"read":"write",i1->mpifh,i1->offset,i1->count,
                                conflict_count);
                    }
                    // skip the rest, as they will all have
                    // a bigger starting offset
                    break;
                }
            }

            i++;
        }
    }
    fclose(conflict_file);
}

int main(int argc, char* argv[]) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    printf("Format:\nFilename,"
           "op1(rank-seqId, offset, bytes),"
           "op2(rank-seqId, offset, bytes)\n\n");

    int i, rank, num_files;
    IntervalsMap *IM = build_offset_intervals(&reader, &num_files);

    detect_conflicts(IM, num_files, argv[1]);

    // Free IM
    for(i = 0; i < num_files; i++) {
        free(IM[i].filename);
        free(IM[i].intervals);
    }
    free(IM);

    recorder_free_reader(&reader);
    return 0;
}
