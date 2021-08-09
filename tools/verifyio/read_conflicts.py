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
    exist_nodes = set()

    conflicts = []
    pairs = []
    for rank in range(total_ranks):
        conflicts.append([])

    f = open(path, "r")
    lines = f.readlines()
    f.close()

    for line in lines:
        pair = line.replace(" ", "").replace("\n", "").split(",")

        if pair[0] not in exist_nodes:
            tmp = pair[0].split("-")
            func = tmp[0]
            rank = int(tmp[1])
            seq_id = int(tmp[2])
            c1 = Call(rank, seq_id, func);
            conflicts[c1.rank].append(c1)
            exist_nodes.add(pair[0])

        if pair[1] not in exist_nodes:
            tmp = pair[1].split("-")
            func = tmp[0]
            rank = int(tmp[1])
            seq_id = int(tmp[2])
            c2 = Call(rank, seq_id, func);
            conflicts[c2.rank].append(c2)
            exist_nodes.add(pair[1])

        # To test for properly synchonization
        # We can ignore the conflicting pair on
        # same node.
        if c1.rank != c2.rank:
            pairs.append([c1, c2])

    return conflicts, pairs
