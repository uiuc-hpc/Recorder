class Call:
    def __init__(self, rank, index, call):
        self.rank = int(rank)
        self.index = int(index) # Sequence Id of the call
        self.call = call
        self.func = call

class MPICall(Call):
    def __init__(self, rank, index, call, src, dst, stag, rtag, comm, tindx, req=None, reqflag=-1):
        self.rank = int(rank)
        self.index = int(index) # Sequence Id of the call
        self.call = str(call)
        self.func = str(call)        # TODO duplicate as some records' call will be set to None
        self.src = int(src) if src else None
        self.dst = int(dst) if dst else None
        self.stag = int(stag) if stag else None
        self.rtag = int(rtag) if rtag else None
        self.comm = comm
        self.req = req
        self.reqflag = reqflag
        self.tindx = tindx

        self.tstart = 0
        self.tend = 0

def generate_mpi_nodes(reader):
    ignored_calls = set()
    nodes = []
    func_list = reader.funcs

    for rank in range(reader.GM.total_ranks):
        index = 0
        this_rank = []
        records = reader.records[rank]

        for i in range(reader.LMs[rank].total_records):
            call = func_list[records[i].func_id]
            args = records[i].args

            skip = True
            src, dst, stag, rtag, comm, req, reqflag, tindx = None, None, None, None, None, None, -1, None

            ## Point-to-Point calls
            if call == 'MPI_Send' or call == 'MPI_Ssend' or call == 'MPI_Isend':
                skip, dst, stag, comm = False, args[3], args[4], args[5]
            elif call == 'MPI_Recv':
                skip, src, rtag, comm = False, args[3],args[4], args[5]
            elif call == 'MPI_Sendrecv':
                skip, src, dst, stag, rtag, comm = False, args[8], args[3], args[4], args[9], args[10]
            elif call == 'MPI_Irecv':
                skip, src, rtag, comm, req = False, args[3], args[4], args[5], args[6]
            elif call == 'MPI_Wait':
                skip, req = False, set([args[0]])
            elif call == 'MPI_Waitall':
                reqs = args[1][1:-1].split(',')
                skip, req = False, set(reqs)
            elif call == 'MPI_Waitany':
                reqs = args[1][1:-1].split(',')
                skip, req, tindx = False, reqs, [args[2]]
            elif call == 'MPI_Waitsome':
                reqs = args[1][1:-1].split(',')
                tind = args[3][1:-1].split(',')
                skip, req, reqflag, tindx = False, reqs, int(args[2]), tind
            elif call == 'MPI_Test':
                skip, req, reqflag = False, set([args[0]]), int(args[1])
            elif call == 'MPI_Testall':
                reqs = args[1][1:-1].split(',')
                skip, req, reqflag = False, set(reqs), int(args[2])
            elif call == 'MPI_Testany':
                reqs = args[1][1:-1].split(',')
                skip, req, reqflag, tindx = False, reqs, int(args[3]), [args[2]]
            elif call == 'MPI_Testsome':
                reqs = args[1][1:-1].split(',')
                tind = args[3][1:-1].split(',')
                skip, req, reqflag, tindx  = False, reqs, int(args[2]), tind
            ## Collective calls
            elif call == 'MPI_Bcast':
                skip, src, comm = False, args[3], args[4]
            elif call == 'MPI_Ibcast':
                skip, src, comm, req = False, args[3], args[4], args[5]
            elif call == 'MPI_Reduce':
                skip, dst, comm = False, args[5], args[6]
            elif call == 'MPI_Ireduce':
                skip, dst, comm, req = False, args[5], args[6], args[7]
            elif call == 'MPI_Gather':
                skip, dst, comm = False, args[6], args[7]
            elif call == 'MPI_Igather':
                skip, dst, comm, req = False, args[6], args[7], args[8]
            elif call == 'MPI_Gatherv':
                skip, dst, comm = False, args[7], args[8]
            elif call == 'MPI_Igatherv':
                skip, dst, comm, req = False, args[7], args[8], args[9]
            elif call == 'MPI_Barrier':
                skip, comm = False, args[0]
            elif call == 'MPI_Alltoall':
                skip, comm = False, args[6]
            elif call == 'MPI_Allreduce':
                skip, comm = False, args[5]
            elif call == 'MPI_Allgatherv':
                skip, comm = False, args[7]
            elif call == 'MPI_Reduce_scatter':
                skip, comm = False, args[5]
            elif call == 'MPI_File_open':
                skip, comm, req, reqflag = False, args[0], args[1], args[2]
            elif call == 'MPI_File_close':
                skip, req = False, args[0]
            elif call == 'MPI_File_read_at_all':
                skip, req, reqflag = False, args[0], args[1]
            elif call == 'MPI_File_write_at_all':
                skip, req, reqflag = False, args[0], args[1]
            elif call == 'MPI_File_set_size':
                skip, req = False, args[0]
            elif call == 'MPI_File_set_view':
                skip, comm, req, reqflag = False, args[0], args[2], args[4]     # MPI_File fh usually stored in "req" variable but if more arguments need to be matched, can be stored in "comm" variable
            elif call == 'MPI_File_sync':
                skip, req = False, args[0]
            elif call == 'MPI_File_read_all':
                skip, req = False, args[0]
            elif call == 'MPI_File_read_ordered':
                skip, req = False, args[0]
            elif call == 'MPI_File_write_all':
                skip, req = False, args[0]
            elif call == 'MPI_File_write_ordered':
                skip, req = False, args[0]
            elif call == 'MPI_Comm_dup':
                skip, comm = False, args[1]
            elif call == 'MPI_Comm_split':
                skip, comm = False, args[3]
            elif call == 'MPI_Comm_split_type':
                skip, comm = False, args[4]
            elif call == 'MPI_Cart_create':
                skip, comm = False, args[5]
            elif call == 'MPI_Cart_sub':
                skip, comm = False, args[2]
            else:
                if call.startswith("MPI_"):
                    ignored_calls.add(call)

            if not skip:
                mpicall = MPICall(rank, index, call, src, dst, stag, rtag, comm, tindx, req, reqflag)
                mpicall.tstart = records[i].tstart
                mpicall.tend = records[i].tend
                this_rank.append(mpicall)


            index += 1

        nodes.append(this_rank)
    print("Ignored Calls: %s" %ignored_calls)
    return nodes

