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
        args = data.split(",")
        rank, seq_id, func, mpifh = int(args[0]), int(args[1]), args[2], args[3]
        return VerifyIONode(rank, seq_id, func, file_id, mpifh);

    exist_nodes = set()
    exist_n2s = set()
    duplicate = 0

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

        buf = line.replace("\n", "").split(":")
        n1_buf = buf[0]
        n2s_buf = buf[1].split(" ")[:2]

        if buf[1] not in exist_n2s:
            exist_n2s.add(buf[1])

        n1 = parse_one_node(n1_buf, file_id)
        if n1_buf not in exist_nodes:
            io_nodes[n1.rank].append(n1)
            exist_nodes.add(n1_buf)

        n2s = [[] for i in repeat(None, nprocs)]
        for n2_buf in n2s_buf:
            n2 = parse_one_node(n2_buf, file_id)
            if n2_buf not in exist_nodes:
                io_nodes[n2.rank].append(n2)
                exist_nodes.add(n2_buf)
            n2s[n2.rank].append(n2)

        pairs.append((n1, n2s))
        # TODO:
        # To save time, we check up to 1000 conflict
        # pairs.
        #if len(pairs) >= 1000:
        #    break;

    return io_nodes, pairs
