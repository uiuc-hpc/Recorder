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

    int idx, i, j;
    for(idx = 0; idx < num_files; idx++) {

        char *filename = IM[idx].filename;
        Interval *intervals = IM[idx].intervals;

        // sort by offset
        qsort(intervals, IM[idx].num_intervals, sizeof(Interval), compare_by_offset);


        int i = 0, j = 0;
        Interval *i1, *i2;
        while(i < IM[idx].num_intervals-1) {
            i1 = &intervals[i];

            j = i + 1;
            while(j < IM[idx].num_intervals) {
                i2 = &intervals[j];

                bool conflict = is_conflict(i1, i2);

                if(conflict) {
                    printf("%s, op1(%d-%d, %lu, %lu, %s), op2(%d-%d, %lu, %lu, %s) \n", filename,
                            i1->rank, i1->seqId, i1->offset, i1->count, i1->isRead?"read":"write",
                            i2->rank, i2->seqId, i2->offset, i2->count, i2->isRead?"read":"write");
                    fprintf(conflict_file, "%s-%d-%d, %s-%d-%d\n",
                            i1->isRead?"read":"write", i1->rank, i1->seqId,
                            i2->isRead?"read":"write", i2->rank, i2->seqId);
                }

                if(i1->offset <= i2->offset && i1->offset+i1->count <= i2->offset+i2->count)
                    j++;
                else
                    break;
            }

            i++;
        }
    }
    fclose(conflict_file);
}

int main(int argc, char* argv[]) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    printf("Format:\nFilename,                          \
            io op1(rank-seqId, offset, bytes, isRead),  \
            io op2(rank-seqId, offset, bytes, isRead)\n\n");

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
