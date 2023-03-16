#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "reader.h"

#define CONFLICT_TYPE_NONE 0
#define CONFLICT_TYPE_RAW  1
#define CONFLICT_TYPE_WAW  2

static int semantics = POSIX_SEMANTICS;


/*
void access_patterns(IntervalsMap *IM, int num_files) {
    int i, idx;
    int consecqutive = 0; int sequential = 0; int random = 0;

    for(idx = 0; idx < num_files; idx++) {

        printf("%d: %s, intervals: %ld\n", idx, IM[idx].filename, IM[idx].num_intervals);

        for(i = 0; i < IM[idx].num_intervals - 1; i++) {

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
*/

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

/*
void detect_overlaps(IntervalsMap *IM, int num_files) {
    int i, idx;
    for(idx = 0; idx < num_files; idx++) {

        char *filename = IM[idx].filename;
        Interval *intervals = IM[idx].intervals;

        // sort by offset
        qsort(intervals, IM[idx].num_intervals, sizeof(Interval), compare_by_offset);

        int overlaps[2][4] = {0} ; // [different rank, same rank][RAR, RAW, WAR, WAW]

        for(i = 0; i < IM[idx].num_intervals-1; i++) {
            Interval *i1 = & intervals[i];
            Interval *i2 = & intervals[i+1];

            // non-overlapping
            if(i1->offset+i1->count <= i2->offset)
                continue;

            printf("%s, i1(%d, %lu, %lu, %d), i2(%d, %lu, %lu, %d) \n", filename, i1->rank, i1->offset, i1->count, i1->isRead, i2->rank, i2->offset, i2->count, i2->isRead);
            int same_rank = (i1->rank == i2->rank)? 1 : 0;
            if(i1->isRead && i2->isRead)                // RAR
                overlaps[same_rank][0] += 1;
            else if(!i1->isRead && !i2->isRead)         // WAW
                overlaps[same_rank][3] += 1;
            else {
                Interval *starts_first = (i1->tstart < i2->tstart) ? i1 : i2;
                if(starts_first->isRead)                // WAR
                    overlaps[same_rank][2] += 1;
                else                                    // RAW
                    overlaps[same_rank][1] += 1;
            }
        }

        if(sum_array(overlaps[0], 4)+sum_array(overlaps[1], 4))
            printf("%s, RAR: D-%d,S-%d, RAW: D-%d,S-%d, WAR: D-%d,S-%d, WAW: D-%d,S-%d\n", filename, overlaps[0][0], overlaps[1][0],
                        overlaps[0][1], overlaps[1][1], overlaps[0][2], overlaps[1][2], overlaps[0][3], overlaps[1][3]);
    }
}
*/

double get_neighbor_op_timestamp(double current, double *tops, int count, bool next) {
    int i;
    if(next) {                      // timestamp of next expected operation
        for(i = 0; i < count; i++)
            if(tops[i] >= current)
                return tops[i];
    } else {                        // timestamp of previous expected operation
        for(i = count-1; i >= 0; i--)
            if(tops[i] <= current)
                return tops[i];
    }

    // If no such operation found, return -1
    return -1;
}

int check_potential_conflict(Interval* i1, Interval* i2, IntervalsMap *IM, int idx) {

    // non-overlapping so no conflict
    if(i1->offset+i1->count <= i2->offset)
        return CONFLICT_TYPE_NONE;

    bool conflict = true;
    Interval *starts_first = (i1->tstart <= i2->tstart) ? i1 : i2;
    Interval *starts_later = (i1->tstart >  i2->tstart) ? i1 : i2;

    // Read starts first will never cause a conflict
    // The only conflicts are WAW or RAW
    if(starts_first->isRead)
        conflict = false;

    if (semantics == COMMIT_SEMANTICS) {
        double next_commit = get_neighbor_op_timestamp(starts_first->tstart,
                                                    IM[idx].tcommits[starts_first->rank],
                                                    IM[idx].num_commits[starts_first->rank], true);
        if(next_commit != -1 && next_commit < starts_later->tstart)
            conflict = false;
    } else if(semantics == SESSION_SEMANTICS) {

        double next_close = get_neighbor_op_timestamp(starts_first->tstart,
                                                    IM[idx].tcloses[starts_first->rank],
                                                    IM[idx].num_closes[starts_first->rank], true);
        double prev_open = get_neighbor_op_timestamp(starts_later->tstart,
                                                    IM[idx].topens[starts_later->rank],
                                                    IM[idx].num_opens[starts_later->rank], false);
        if(next_close != -1 && prev_open != -1 && next_close < prev_open)
            conflict = false;
    } else {
        // POSIX semantics
        // Conflict only when the two ops start exactly the same time
        // otherwise, we assume they are not conflicting
        if(i1->tstart != i2->tstart)
            conflict = false;
    }

    //printf("potential: %s, op1(%d-%d, %lu, %lu, %s), op2(%d-%d, %lu, %lu, %s) \n", IM[idx].filename,
    //        i1->rank, i1->seqId, i1->offset, i1->count, i1->isRead?"read":"write",
    //        i2->rank, i2->seqId, i2->offset, i2->count, i2->isRead?"read":"write");

    if(!conflict)
        return CONFLICT_TYPE_NONE;
    if(starts_later->isRead)     // RAW
        return CONFLICT_TYPE_RAW;
    else
        return CONFLICT_TYPE_WAW;
}

void detect_potential_conflicts(IntervalsMap *IM, int num_files, const char* base_dir) {
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

        int conflicts[2][2] = {0} ; // [different rank, same rank][RAW, WAW]


        int i = 0, j = 0;
        Interval *i1, *i2;
        while(i < IM[idx].num_intervals-1) {
            i1 = &intervals[i];

            j = i + 1;
            while(j < IM[idx].num_intervals) {
                i2 = &intervals[j];

                int conflict = check_potential_conflict(i1, i2, IM, idx);

                if(conflict == CONFLICT_TYPE_NONE) {

                } else {
                    printf("%s, op1(%d-%d, %lu, %lu, %s), op2(%d-%d, %lu, %lu, %s) \n", filename,
                            i1->rank, i1->seqId, i1->offset, i1->count, i1->isRead?"read":"write",
                            i2->rank, i2->seqId, i2->offset, i2->count, i2->isRead?"read":"write");
                    fprintf(conflict_file, "%s-%d-%d, %s-%d-%d\n",
                            i1->isRead?"read":"write", i1->rank, i1->seqId,
                            i2->isRead?"read":"write", i2->rank, i2->seqId);

                    int same_rank = (i1->rank == i2->rank)? 1 : 0;

                    if(conflict == CONFLICT_TYPE_RAW)
                        conflicts[same_rank][0] += 1;
                    else if(conflict == CONFLICT_TYPE_WAW)
                        conflicts[same_rank][1] += 1;
                }

                if(i1->offset <= i2->offset && i1->offset+i1->count <= i2->offset+i2->count)
                    j++;
                else
                    break;
            }

            i++;
        }

        if(sum_array(conflicts[0], 2)+sum_array(conflicts[1], 2))
            printf("%s, Read-after-write (RAW): D-%d,S-%d, Write-after-write (WAW): D-%d,S-%d\n", filename, conflicts[0][0], conflicts[1][0], conflicts[0][1], conflicts[1][1]);
        else
            printf("No potential conflict found for file %s.\n", filename);
        printf("================================================================================\n\n");
    }
    fclose(conflict_file);
}

int main(int argc, char* argv[]) {

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    if(argc == 3 && strstr(argv[2], "--semantics="))  {
        if(strstr(argv[2], "posix")) {
            semantics = POSIX_SEMANTICS;
        } else if(strstr(argv[2], "commit"))
            semantics = COMMIT_SEMANTICS;
        else if(strstr(argv[2], "session"))
            semantics = SESSION_SEMANTICS;
    }

    if(semantics == POSIX_SEMANTICS)
        printf("Check potential conflicts under POSIX Semantics\n");
    if(semantics == COMMIT_SEMANTICS)
        printf("Check potential conflicts under Commit Semantics\n");
    if(semantics == SESSION_SEMANTICS)
        printf("Check potential conflicts under Session Semantics\n");

    printf("Format:\nFilename, io op1(rank-seqId, offset, bytes, isRead), io op2(rank-seqId, offset, bytes, isRead)\n\n");

    int i, rank, num_files;
    IntervalsMap *IM = build_offset_intervals(&reader, semantics, &num_files);

    //access_patterns(IM, num_files);
    //detect_overlaps(IM, num_files);
    detect_potential_conflicts(IM, num_files, argv[1]);

    for(i = 0; i < num_files; i++) {
        free(IM[i].filename);
        free(IM[i].intervals);

        free(IM[i].num_opens);
        free(IM[i].num_closes);
        free(IM[i].num_commits);

        for(rank = 0; rank < reader.metadata.total_ranks; rank++) {
            free(IM[i].topens[rank]);
            free(IM[i].tcloses[rank]);
            free(IM[i].tcommits[rank]);
        }

        free(IM[i].topens);
        free(IM[i].tcloses);
        free(IM[i].tcommits);
    }
    free(IM);

    recorder_free_reader(&reader);

    return 0;
}
