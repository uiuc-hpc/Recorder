from itertools import repeat
import match_mpi

ANY_SOURCE = -2
ANY_TAG = -1


class Call:
    def __init__(self, rank, index, call):
        self.rank = int(rank)
        self.index = int(index) # Sequence Id of the call
        self.call = call
        self.func = call
        self.key = ""

class MPICall(Call):
    def __init__(self, rank, index, call, src, dst, stag, rtag, comm, tindx, req=None, reqflag=-1):
        self.rank = int(rank)
        self.index = int(index)             # Sequence Id of the call
        self.call = str(call)
        self.func = str(call)               # TODO duplicate as some records' call will be set to None
        self.src = int(src) if src else None
        self.dst = int(dst) if dst else None
        self.stag = int(stag) if stag else None
        self.rtag = int(rtag) if rtag else None
        self.comm = comm
        self.req = req                      # MPI_Requests for wait/test calls or MPI_File handle for I/O calls
        self.reqflag = reqflag
        self.tindx = tindx

        self.tstart = 0
        self.tend = 0
        self.key = self.call + ";" + str(self.comm)

    def get_key(self):
        return self.key

    def is_blocking_call(self):
        if self.call and self.call.startswith("MPI_I"):
            return False
        return True

class VerifyIOContext:
    def __init__(self, reader, mpi_sync_calls):
        self.num_ranks = reader.GM.total_ranks
        self.all_calls       = [[] for i in repeat(None, self.num_ranks)]
        self.recv_calls      = [[[] for i in repeat(None, self.num_ranks)] for j in repeat(None, self.num_ranks)]
        self.send_calls      = [0 for i in repeat(None, self.num_ranks)]
        self.wait_test_calls = [[] for i in repeat(None, self.num_ranks)]
        self.coll_calls      = [{} for i in repeat(None, self.num_ranks)]

        self.send_func_names   = ['MPI_Send','MPI_Ssend', 'MPI_Isend','MPI_Sendrecv']
        self.recv_func_names   = ['MPI_Recv', 'MPI_Irecv', 'MPI_Sendrecv']
        self.bcast_func_names  = ['MPI_Bcast', 'MPI_Ibcast']
        self.redgat_func_names = ['MPI_Reduce', 'MPI_Ireduce', 'MPI_Gather', 'MPI_Igather', 'MPI_Gatherv', 'MPI_Igatherv']
        self.alltoall_func_names   = ['MPI_Barrier', 'MPI_Allreduce', 'MPI_Allgatherv', 'MPI_Allgatherv', 'MPI_Alltoall',
                'MPI_Reduce_scatter', 'MPI_File_open', 'MPI_File_close', 'MPI_File_read_all',
                'MPI_File_read_at_all', 'MPI_File_read_order', 'MPI_File_write_all', 'MPI_File_write_at_all',
                'MPI_File_write_ordered', 'MPI_File_set_size', 'MPI_File_set_view', 'MPI_File_sync',
                'MPI_Comm_dup', 'MPI_Comm_split', 'MPI_Comm_split_type', 'MPI_Cart_create', 'MPI_Cart_sub']
        if mpi_sync_calls:
            self.bcast_func_names  = []
            self.redgat_func_names = ['MPI_Reduce_scatter', 'MPI_Reduce_scatter_block']
            self.alltoall_func_names   = ['MPI_Barrier', 'MPI_Allgather', 'MPI_Alltoall', 'MPI_Alltoallv', 'MPI_Alltoallw', 'MPI_Allreduce']

    def is_send_call(self, func_name):
        if func_name in self.send_func_names:
            return True
        return False

    def is_recv_call(self, func_name):
        if func_name in self.recv_func_names:
            return True
        return False

    def is_coll_call(self, func_name):
        if func_name in self.alltoall_func_names or func_name in self.bcast_func_names or func_name in self.redgat_func_names:
            return True
        return False

    def is_all_to_all_call(self, func_name):
        if func_name in self.alltoall_func_names:
            return True
        return False

    def generate_mpi_nodes(self, reader, translate):
        def mpi_status_to_src_tag(status_str):
            if str(status_str).startswith("["):
                return status_str[1:-1].split("_")[0], status_str[1:-1].split("_")[1]
            else:   # MPI_STATUS_IGNORE
                return 0, 0

        ignored_calls = set()

        func_list = reader.funcs
        for rank in range(self.num_ranks):
            records = reader.records[rank]

            for i in range(reader.LMs[rank].total_records):
                call = func_list[records[i].func_id]
                args = records[i].args

                skip = True
                src, dst, stag, rtag, comm, req, reqflag, tindx = None, None, None, None, None, None, 1, None

                ## Point-to-Point calls
                if call == 'MPI_Send' or call == 'MPI_Ssend' or call == 'MPI_Isend':
                    skip, dst, stag, comm = False, args[3], args[4], args[5]
                elif call == 'MPI_Recv':
                    skip, src, rtag, comm = False, args[3],args[4], args[5]
                    # get the actual source from MPI_Status
                    if int(src) == ANY_SOURCE:
                        src = args[6][1:-1].split("_")[0]
                    if int(rtag) == ANY_TAG:
                        rtag = args[6][1:-1].split("_")[1]
                elif call == 'MPI_Sendrecv':
                    skip, src, dst, stag, rtag, comm = False, args[8], args[3], args[4], args[9], args[10]
                elif call == 'MPI_Irecv':
                    skip, src, rtag, comm, req = False, args[3], args[4], args[5], args[6]
                elif call == 'MPI_Wait':
                    skip, req = False, set([args[0]])
                    src, rtag = mpi_status_to_src_tag(args[1])
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
                    src, rtag = mpi_status_to_src_tag(args[2])
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
                    skip, src, comm = False, args[5], args[6]
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

                if (not skip) and (reqflag):    # MPI_Test calls with a false reqflag is no use for matching and ordering.
                    mpicall = MPICall(rank, i, call, src, dst, stag, rtag, comm, tindx, req, reqflag)
                    mpicall.tstart = records[i].tstart
                    mpicall.tend = records[i].tend
                    self.all_calls[rank].append(mpicall)

                    idx = len(self.all_calls[rank]) - 1

                    if self.is_coll_call(call):
                        key = mpicall.get_key()
                        if key in self.coll_calls[rank]:
                            self.coll_calls[rank][key].append(idx)
                        else:
                            self.coll_calls[rank][key] = [idx]
                    if self.is_send_call(call):
                        self.send_calls[rank] += 1
                    if self.is_recv_call(call):
                        global_src = match_mpi.local2global(translate, comm, int(src))
                        self.recv_calls[rank][global_src].append(idx)
                    if call.startswith("MPI_Wait") or call.startswith("MPI_Test"):
                        self.wait_test_calls[rank].append(idx)

        #print("Ignored Calls: %s" %ignored_calls)
