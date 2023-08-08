#!/usr/bin/env python
# encoding, utf-8
from itertools import repeat
from verifyio_graph import VerifyIONode

accepted_mpi_funcs = [
 'MPI_Send', 'MPI_Ssend', 'MPI_Isend',          
 'MPI_Recv', 'MPI_Sendrecv', 'MPI_Irecv',
 'MPI_Wait', 'MPI_Waitall', 'MPI_Waitany',
 'MPI_Waitsome', 'MPI_Test', 'MPI_Testall',
 'MPI_Testany', 'MPI_Testsome', 'MPI_Bcast',
 'MPI_Ibcast', 'MPI_Reduce', 'MPI_Ireduce',
 'MPI_Gather', 'MPI_Igather', 'MPI_Gatherv',
 'MPI_Igatherv', 'MPI_Barrier', 'MPI_Alltoall',
 'MPI_Allreduce', 'MPI_Allgatherv', 
 'MPI_Reduce_scatter', 'MPI_File_open',
 'MPI_File_close', 'MPI_File_read_at_all',
 'MPI_File_write_at_all', 'MPI_File_set_size',
 'MPI_File_set_view', 'MPI_File_sync', 
 'MPI_File_read_all', 'MPI_File_read_ordered',
 'MPI_File_write_all','MPI_File_write_ordered',
 'MPI_Comm_dup', 'MPI_Comm_split',
 'MPI_Comm_split_type', 'MPI_Cart_create',
 'MPI_Cart_sub'
]

def read_mpi_nodes(reader):
    nprocs = reader.GM.total_ranks
    mpi_nodes = [[] for i in repeat(None, nprocs)]

    func_list = reader.funcs
    for rank in range(nprocs):
        records = reader.records[rank]
        for seq_id in range(reader.LMs[rank].total_records):
            func = func_list[records[seq_id].func_id]
            mpifh = None
            if func in accepted_mpi_funcs:
                if func.startswith("MPI_File"):
                    mpifh = records[seq_id].args[0].decode('utf-8')
                mpi_node = VerifyIONode(rank, seq_id, func, -1, mpifh)
                mpi_nodes[rank].append(mpi_node)

    return mpi_nodes

'''
Read confliciing pairs from a file
Each line of the file should have the following format,
    func-rank1-seqId1, func-rank2-seqId2

Return:
    nodes: conflicting I/O accesses of type VerifyIONode
    pairs: list of [c1, c2], where c1 and c2 are VerifyIONode
'''
def read_io_nodes(reader, path):
    
    # format: rank,seqId,func(mpifh,offset,count)
    def parse_one_node(data, file_id):
        data = data.replace(")", "")
        meta = data.split("(")[0]
        args = data.split("(")[1]
        meta = meta.split(",")
        args = args.split(",")
        rank, seq_id, func = int(meta[0]), int(meta[1]), meta[2]
        mpifh, offset, count = args[0], int(args[1]), int(args[2])
        return VerifyIONode(rank, seq_id, func, file_id, mpifh);

    exist_nodes = set()

    nprocs = reader.GM.total_ranks
    io_nodes = [[] for i in repeat(None, nprocs)]
    pairs = []

    f = open(path, "r")
    lines = f.readlines()[1:] # skip first line
    f.close()

    file_id = 0
    filename = ""
    for line in lines:

        if line[0] == "#":
            file_id  = line.split(":")[0]
            filename = line.split(":")[1]
            continue

        pair = line.replace("\n", "").split(" ")

        n1 = parse_one_node(pair[0], file_id)
        n2 = parse_one_node(pair[1], file_id)

        if pair[0] not in exist_nodes:
            io_nodes[n1.rank].append(n1)
            exist_nodes.add(pair[0])
        if pair[1] not in exist_nodes:
            io_nodes[n2.rank].append(n2)
            exist_nodes.add(pair[1])

        # To test for properly synchonization
        # We can ignore the conflicting pair on
        # same node.
        if n1.rank != n2.rank:
            pairs.append([n1, n2])

        # TODO:
        # To save time, we check up to 1000 conflict
        # pairs.
        if len(pairs) > 1000:
            break;

    return io_nodes, pairs
