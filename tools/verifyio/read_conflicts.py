#!/usr/bin/env python
# encoding: utf-8
from gen_nodes import Call

'''
Read confliciing pairs from a file
Each line of the file should have the following format:
    rank1-seqId1, rank2-seqId2

Return nodes of conflicting accesses
'''
def read_conflicting_accesses(path, total_ranks):
    conflicts = []
    pairs = []
    for rank in range(total_ranks):
        conflicts.append([])

    f = open(path, "r")
    lines = f.readlines()
    f.close()

    for line in lines:
        pair = line.replace(" ", "").split(",")

        tmp = pair[0].split("-")
        func = tmp[0]
        rank = int(tmp[1])
        seq_id = int(tmp[2])
        c1 = Call(rank, seq_id, func);

        tmp = pair[1].split("-")
        func = tmp[0]
        rank = int(tmp[1])
        seq_id = int(tmp[2])
        c2 = Call(rank, seq_id, func);

        conflicts[c1.rank].append(c1)
        conflicts[c2.rank].append(c2)
        pairs.append([c1, c2])


    return conflicts, pairs
