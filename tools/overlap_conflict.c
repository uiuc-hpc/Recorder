#include <stdlib.h>
#include <stdio.h>
#include "reader.h"
using namespace std;


void access_patterns(IntervalsMap *IM, int num_files) {
    int consecqutive = 0; int sequential = 0; int random = 0;

    for(int idx = 0; idx < num_files; idx++) {

        printf("%d: %s, intervals: %ld\n", idx, IM[idx].filename, IM[idx].num_intervals);

        for(int i = 0; i < IM[idx].num_intervals - 1; i++) {

            Interval *i1 = & IM[idx].intervals[i];
            Interval *i2 = & IM[idx].intervals[i+1];

            if(i1->offset+i1->count == i2->offset)
                consecqutive++;
            else if(i1->offset+i1->count < i2->offset)
                sequential++;
            else
                random++;
        }
    }

    printf("consecutive: %d, sequential: %d, random: %d\n", consecqutive, sequential, random);
}

int compare_by_offset(const void *lhs, const void *rhs) {
    Interval *first = (Interval*)lhs;
    Interval *second = (Interval*)rhs;
    return first->offset - second->offset;
}
void detect_overlaps(IntervalsMap *IM, int num_files) {

    for(int idx = 0; idx < num_files; idx++) {

        char *filename = IM[idx].filename;
        Interval *intervals = IM[idx].intervals;

        qsort(intervals, IM[idx].num_intervals, sizeof(Interval), compare_by_offset);

        int overlaps[2][4]; // [same or different rank][RAR, RAW, WAR, WAW]
        for(int i = 0; i < 2; i++)
            for(int j = 0; j < 4; j++)
                overlaps[i][j] = 0;

        for(int i = 0; i < IM[idx].num_intervals-1; i++) {
            Interval *i1 = & intervals[i];
            Interval *i2 = & intervals[i+1];

            // no conflicts
            if(i1->offset+i1->count <= i2->offset)
                continue;

            int same_rank = (i1->rank == i2->rank)? 1 : 0;
            if(i1->isRead && i2->isRead)
                overlaps[same_rank][0] = 1;
            else if(!i1->isRead && !i2->isRead)
                overlaps[same_rank][3] = 1;
            else {
            }
        }
    }
}


int main(int argc, char* argv[]) {
    RecorderReader reader;
    recorder_read_traces(argv[1], &reader);


    int num_files;
    IntervalsMap *IM = build_offset_intervals(reader, &num_files);


    access_patterns(IM, num_files);
    //detect_overlaps(IM, num_files);

    for(int i = 0; i < num_files; i++) {
        free(IM[i].filename);
        free(IM[i].intervals);
    }
    free(IM);

    return 0;
}
