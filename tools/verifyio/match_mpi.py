import sys
from itertools import repeat
from verifyio_graph import VerifyIONode, MPICallType

ANY_SOURCE = -2
ANY_TAG = -1

class MPIEdge():
    def __init__(self, call_type, head=None, tail=None):
        # Init head/tail according the cal type
        self.call_type = call_type  # enum of MPICallType
        if call_type == MPICallType.ALL_TO_ALL:
            self.head, self.tail = [], []
        if call_type == MPICallType.ONE_TO_MANY:
            self.head, self.tail = None, []
        if call_type == MPICallType.MANY_TO_ONE:
            self.head, self.tail = [], None
        if call_type == MPICallType.POINT_TO_POINT:
            self.head, self.tail = None, None
        # override if supplied
        if head:
            self.head = head        # list/instance of VerifyIONode
        if tail:
            self.tail = tail        # list/instance of VerifyIONode

class MPICall():
    def __init__(self, rank, seq_id, func, src, dst, stag, rtag, comm, tindx, req=None, reqflag=-1):
        self.rank   = int(rank)
        self.seq_id = int(seq_id)             # Sequence Id of the mpi call
        self.func   = str(func)
        self.src    = int(src) if src else None # root of reduce/bcast call
        self.dst    = int(dst) if dst else None
        self.stag   = int(stag) if stag else None
        self.rtag   = int(rtag) if rtag else None
        self.comm   = comm
        self.req    = req                      # MPI_Requests for wait/test calls or MPI_File handle for I/O calls
        self.reqflag = reqflag
        self.tindx   = tindx
        self.matched = False

    def get_key(self):
        # self.comm for calls like MPI_Bcast, MPI_Barier
        # self.req for calls like MPI_File_close
        key = self.func + ";" + str(self.comm) + ";" + str(self.req)
        return key

    def is_blocking_call(self):
        if self.func.startswith("MPI_I"):
            return False
        return True

class MPIMatchHelper:
    def __init__(self, reader, mpi_sync_calls):
        self.recorder_reader = reader
        self.num_ranks       = reader.GM.total_ranks
        self.all_mpi_calls   = [[] for i in repeat(None, self.num_ranks)]

        self.recv_calls      = [[[] for i in repeat(None, self.num_ranks)] for j in repeat(None, self.num_ranks)]
        self.send_calls      = [0 for i in repeat(None, self.num_ranks)]
        self.wait_test_calls = [[] for i in repeat(None, self.num_ranks)]
        self.coll_calls      = [{} for i in repeat(None, self.num_ranks)]

        self.send_func_names   = ['MPI_Send','MPI_Ssend', 'MPI_Isend','MPI_Sendrecv']
        self.recv_func_names   = ['MPI_Recv', 'MPI_Irecv', 'MPI_Sendrecv']

        # According to the MPI standard, not all collective calls serve the purpose
        # of synchornzations, i.e., many of them do not impose order
        if mpi_sync_calls:
            self.bcast_func_names  = []
            self.redgat_func_names = ['MPI_Reduce_scatter', 'MPI_Reduce_scatter_block']
            self.alltoall_func_names   = ['MPI_Barrier', 'MPI_Allgather', 'MPI_Alltoall', 'MPI_Alltoallv', 'MPI_Alltoallw', 'MPI_Allreduce']
        else:
            self.bcast_func_names  = ['MPI_Bcast', 'MPI_Ibcast']
            self.redgat_func_names = ['MPI_Reduce', 'MPI_Ireduce', 'MPI_Gather', 'MPI_Igather', 'MPI_Gatherv', 'MPI_Igatherv']
            self.alltoall_func_names = ['MPI_Barrier', 'MPI_Allreduce', 'MPI_Allgatherv', 'MPI_Allgatherv', 'MPI_Alltoall',
                    'MPI_Reduce_scatter', 'MPI_File_open', 'MPI_File_close', 'MPI_File_read_all',
                    'MPI_File_read_at_all', 'MPI_File_read_order', 'MPI_File_write_all', 'MPI_File_write_at_all',
                    'MPI_File_write_ordered', 'MPI_File_set_size', 'MPI_File_set_view', 'MPI_File_sync',
                    'MPI_Comm_dup', 'MPI_Comm_split', 'MPI_Comm_split_type', 'MPI_Cart_create', 'MPI_Cart_sub']

        self.translate_table = self.__generate_translation_table()

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

    def call_type(self, func_name):
        if self.is_send_call(func_name):
            return MPICallType.POINT_TO_POINT
        if func_name in self.alltoall_func_names:
            return MPICallType.ALL_TO_ALL
        if func_name in self.bcast_func_names:
            return MPICallType.ONE_TO_MANY
        if func_name in self.redgat_func_names:
            return MPICallType.MANY_TO_ONE
        return MPICallType.OTHER

    def read_one_mpi_call(self, rank, seq_id, record):

        func = self.recorder_reader.funcs[record.func_id]

        # in python3, record.args are bytes instead of
        # str in like python2. need to decode them to
        # str first
        args = []
        for i in range(record.arg_count):
            arg = record.args[i].decode("utf-8")
            args.append(arg)

        src, dst, stag, rtag = None, None, None, None
        comm, req, reqflag, tindx = None, None, 1, None
        ## Point-to-Point calls
        if func == 'MPI_Send' or func == 'MPI_Ssend' or func == 'MPI_Isend':
            skip, dst, stag, comm = False, args[3], args[4], args[5]
        elif func == 'MPI_Recv':
            skip, src, rtag, comm = False, args[3],args[4], args[5]
            # get the actual source from MPI_Status
            if int(src) == ANY_SOURCE:
                src = args[6][1:-1].split('_')[0]
            if int(rtag) == ANY_TAG:
                rtag = args[6][1:-1].split('_')[1]
        elif func == 'MPI_Sendrecv':
            skip, src, dst, stag, rtag, comm = False, args[8], args[3], args[4], args[9], args[10]
        elif func == 'MPI_Irecv':
            skip, src, rtag, comm, req = False, args[3], args[4], args[5], args[6]
        elif func == 'MPI_Wait':
            skip, req = False, set([args[0]])
            src, rtag = self.mpi_status_to_src_tag(args[1])
        elif func == 'MPI_Waitall':
            reqs = args[1][1:-1].split(',')
            skip, req = False, set(reqs)
        elif func == 'MPI_Waitany':
            reqs = args[1][1:-1].split(',')
            skip, req, tindx = False, reqs, [args[2]]
        elif func == 'MPI_Waitsome':
            reqs = args[1][1:-1].split(',')
            tind = args[3][1:-1].split(',')
            skip, req, reqflag, tindx = False, reqs, int(args[2]), tind
        elif func == 'MPI_Test':
            skip, req, reqflag = False, set([args[0]]), int(args[1])
            src, rtag = self.mpi_status_to_src_tag(args[2])
        elif func == 'MPI_Testall':
            reqs = args[1][1:-1].split(',')
            skip, req, reqflag = False, set(reqs), int(args[2])
        elif func == 'MPI_Testany':
            reqs = args[1][1:-1].split(',')
            skip, req, reqflag, tindx = False, reqs, int(args[3]), [args[2]]
        elif func == 'MPI_Testsome':
            reqs = args[1][1:-1].split(',')
            tind = args[3][1:-1].split(',')
            skip, req, reqflag, tindx  = False, reqs, int(args[2]), tind
        ## Collective calls
        # for one-tomany and many-to-one calls like
        # reduce and gather we use src to store root parameters
        elif func == 'MPI_Bcast':
            skip, src, comm = False, args[3], args[4]
        elif func == 'MPI_Ibcast':
            skip, src, comm, req = False, args[3], args[4], args[5]
        elif func == 'MPI_Reduce':
            skip, src, comm = False, args[5], args[6]
        elif func == 'MPI_Ireduce':
            skip, src, comm, req = False, args[5], args[6], args[7]
        elif func == 'MPI_Gather':
            skip, src, comm = False, args[6], args[7]
        elif func == 'MPI_Igather':
            skip, src, comm, req = False, args[6], args[7], args[8]
        elif func == 'MPI_Gatherv':
            skip, src, comm = False, args[7], args[8]
        elif func == 'MPI_Igatherv':
            skip, src, comm, req = False, args[7], args[8], args[9]
        elif func == 'MPI_Barrier':
            skip, comm = False, args[0]
        elif func == 'MPI_Alltoall':
            skip, comm = False, args[6]
        elif func == 'MPI_Allreduce':
            skip, comm = False, args[5]
        elif func == 'MPI_Allgatherv':
            skip, comm = False, args[7]
        elif func == 'MPI_Reduce_scatter':
            skip, comm = False, args[5]
        elif func == 'MPI_File_open':
            skip, comm, req, reqflag = False, args[0], args[4], args[2]
        elif func == 'MPI_File_close':
            skip, req = False, args[0]
        elif func == 'MPI_File_read_at_all':
            skip, req, reqflag = False, args[0], args[1]
        elif func == 'MPI_File_write_at_all':
            skip, req, reqflag = False, args[0], args[1]
        elif func == 'MPI_File_set_size':
            skip, req = False, args[0]
        elif func == 'MPI_File_set_view':
            skip, comm, req, reqflag = False, args[0], args[2], args[4]     # MPI_File fh usually stored in "req" variable but if more arguments need to be matched, can be stored in "comm" variable
        elif func == 'MPI_File_sync':
            skip, req = False, args[0]
        elif func == 'MPI_File_read_all':
            skip, req = False, args[0]
        elif func == 'MPI_File_read_ordered':
            skip, req = False, args[0]
        elif func == 'MPI_File_write_all':
            skip, req = False, args[0]
        elif func == 'MPI_File_write_ordered':
            skip, req = False, args[0]
        elif func == 'MPI_Comm_dup':
            skip, comm = False, args[1]
        elif func == 'MPI_Comm_split':
            skip, comm = False, args[3]
        elif func == 'MPI_Comm_split_type':
            skip, comm = False, args[4]
        elif func == 'MPI_Cart_create':
            skip, comm = False, args[5]
        elif func == 'MPI_Cart_sub':
            skip, comm = False, args[2]
        else:
            pass

        # MPI_Test calls with a false reqflag is no use for matching and ordering.
        if reqflag:
            mpi_call = MPICall(rank, seq_id, func, src, dst, stag, rtag, comm, tindx, req, reqflag)
            return mpi_call
        else:
            return None

    def mpi_status_to_src_tag(self, status_str):
        if str(status_str).startswith("["):
            return status_str[1:-1].split("_")[0], status_str[1:-1].split("_")[1]
        else:   # MPI_STATUS_IGNORE
            return 0, 0

    # Go through every record in the trace and preprocess
    # the mpi calls, so they can be matched later.
    def read_mpi_calls(self, reader):
        ignored_funcs = set()
        for rank in range(self.num_ranks):
            records = reader.records[rank]
            for seq_id in range(reader.LMs[rank].total_records):

                func_name = reader.funcs[records[seq_id].func_id]
                mpi_call = self.read_one_mpi_call(rank, seq_id, records[seq_id])

                # Not an MPI call or an MPI call that we
                # don't need for sync/ordering
                if not mpi_call:
                    ignored_funcs.add(func_name)
                    continue

                self.all_mpi_calls[rank].append(mpi_call)

                # Note here the index is not the same as
                # seq id. Seq id the index in trace records.
                # The index here is the index of all_mpi_calls
                # without gap.
                index = len(self.all_mpi_calls[rank]) - 1

                if self.is_coll_call(func_name):
                    key = mpi_call.get_key()
                    if key in self.coll_calls[rank]:
                        self.coll_calls[rank][key].append(index)
                    else:
                        self.coll_calls[rank][key] = [index]
                if self.is_send_call(func_name):
                    self.send_calls[rank] += 1
                if self.is_recv_call(func_name):
                    global_src = self.local2global(mpi_call.comm, int(mpi_call.src))
                    self.recv_calls[rank][global_src].append(index)
                if func_name.startswith("MPI_Wait") or func_name.startswith("MPI_Test"):
                    self.wait_test_calls[rank].append(index)

        print("Ignored funcs: %s" %ignored_funcs)

    def __generate_translation_table(self):
        func_list = self.recorder_reader.funcs

        translate = {}
        translate['MPI_COMM_WORLD'] = range(self.num_ranks)

        for rank in range(self.num_ranks):
            records = self.recorder_reader.records[rank]
            for i in range(self.recorder_reader.LMs[rank].total_records):
                record = records[i]
                func = func_list[record.func_id]

                comm_id, local_rank, world_rank = None, rank, rank

                if func == 'MPI_Comm_split':
                    comm_id = record.args[3]
                    local_rank = int(record.args[4])
                if func == 'MPI_Comm_split_type':
                    comm_id = record.args[4]
                    local_rank = int(record.args[5])
                if func == 'MPI_Comm_dup':
                    comm_id = record.args[1]
                    local_rank = int(record.args[2])
                if func == 'MPI_Cart_create':
                    comm_id = record.args[5]
                    local_rank = int(record.args[6])
                if func == 'MPI_Comm_create':
                    comm_id = record.args[2]
                    local_rank = int(record.args[3])
                if func == 'MPI_Cart_sub':
                    comm_id = record.args[2]
                    local_rank = int(record.args[3])

                if comm_id:
                    comm = comm_id.decode() if isinstance(comm_id, bytes) else comm_id
                    if comm not in translate:
                        translate[comm] = list(range(self.num_ranks))
                    translate[comm][local_rank] = world_rank
        return translate

    # Local rank to global rank
    def local2global(self, comm_id, local_rank):
        comm = comm_id.decode() if isinstance(comm_id, bytes) else comm_id
        if local_rank >= 0:
            return self.translate_table[comm][local_rank]
        # ANY_SOURCE
        return local_rank


def find_wait_test_call(req, rank, helper, need_match_src_tag=False, src=0, tag=0):

    found = None

    # We don't check the flag here
    # We are certain that calls in wait_test_calls all have completed
    # rquests
    for idx in helper.wait_test_calls[rank]:
        wait_call = helper.all_mpi_calls[rank][idx]
        func = wait_call.func

        if (func == 'MPI_Wait' or func == 'MPI_Waitall') or \
           (func == 'MPI_Test' or func == 'MPI_Testall') :
            if req in wait_call.req:
                if need_match_src_tag:
                    if src==wait_call.src and tag==wait_call.rtag:
                        found = wait_call
                else:
                    found = wait_call

                if found:
                    wait_call.req.remove(req)
                    if(len(wait_call.req) == 0):
                        helper.wait_test_calls[rank].remove(idx)
                    break

        elif func == 'MPI_Waitany' or func == 'MPI_Testany':
            if req in wait_call.req:
                inx = wait_call.req.index(req)
                if str(inx) in wait_call.tindx:
                    found = wait_call
                    helper.wait_test_calls[rank].remove(idx)
                    break

        elif func == 'MPI_Waitsome' or func == 'MPI_Testsome':
            if req in wait_call.req:
                inx = wait_call.req.index(req)
                if str(inx) in wait_call.tindx:
                    found = wait_call
                    wait_call.req.remove(req)
                    wait_call.tindx.remove(str(inx))
                    if len(wait_call.tindx) == 0:
                        helper.wait_test_calls[rank].remove(idx)
                    break
    return found


def match_collective(mpi_call, helper):

    def add_nodes_to_edge(edge, call):
        node = VerifyIONode(call.rank, call.seq_id, call.func)
        # All-to-all (alltoall, barrier, etc.)
        if edge.call_type == MPICallType.ALL_TO_ALL:
            edge.head.append(node)
            edge.tail.append(node)
        # One-to-many (bcast)
        if edge.call_type == MPICallType.ONE_TO_MANY:
            if call.rank == helper.local2global(call.comm, call.src):
                edge.head = node
            else:
                edge.tail.append(node)
        # Many-to-one (reduce)
        if edge.call_type == MPICallType.MANY_TO_ONE:
            if call.rank == helper.local2global(call.comm, call.src):
                edge.tail = node
            else:
                edge.head.append(node)

    # All matching collective calls have the same name
    # and thus the same call type
    call_type = helper.call_type(mpi_call.func)
    edge = MPIEdge(call_type)

    for rank in range(helper.num_ranks):

        key = mpi_call.get_key()

        # this rank has not made this particular
        # collective call
        if key not in helper.coll_calls[rank]:
            continue

        # this rank has made this collective call
        coll_call_index = helper.coll_calls[rank][key][0]
        coll_call       = helper.all_mpi_calls[rank][coll_call_index]

        # blocking vs. non-blocking
        if mpi_call.is_blocking_call():
            add_nodes_to_edge(edge, coll_call)
        else:
            wait_call = find_wait_test_call(coll_call.req, rank, helper)
            if wait_call:
                add_nodes_to_edge(edge, wait_call)

        # If no more collective calls have the same key
        # then remove this key from the dict
        helper.coll_calls[rank][key].pop(0)
        if(len(helper.coll_calls[rank][key]) == 0):
            helper.coll_calls[rank].pop(key)

        # Set this collective call as matched
        # so we don't do repeat work when later
        # exam this call
        coll_call.matched = True

    mpi_call.matched = True
    #print("match collective:", mpi_call.func, "root:", mpi_call.src, mpi_call.comm)
    return edge

#@profile
def match_pt2pt(send_call, helper):

    head_node = VerifyIONode(send_call.rank, send_call.seq_id, send_call.func)
    tail_node = None

    comm = send_call.comm
    global_dst =  helper.local2global(comm, send_call.dst)
    global_src = send_call.rank

    for recv_call_idx in helper.recv_calls[global_dst][global_src]:
        recv_call = helper.all_mpi_calls[global_dst][recv_call_idx]

        # Check for comm, src, and tag.
        if recv_call.comm != comm: continue

        if (recv_call.rtag == send_call.stag or recv_call.rtag == ANY_TAG):
            if recv_call.is_blocking_call():
                # we don't really need to set this because 
                # we always start matching from send calls
                # and we use helper.recv_calls[][] to key
                # track of unmatched recv calls.
                recv_call.matched = True
                tail_node = VerifyIONode(recv_call.rank, recv_call.seq_id, recv_call.func)
            else:
                if recv_call.rtag == ANY_TAG or global_src == ANY_SOURCE:
                    wait_call = find_wait_test_call(recv_call.req, global_dst, helper, True, send_call.rank, send_call.stag)
                else:
                    wait_call = find_wait_test_call(recv_call.req, global_dst, helper)
                if wait_call:
                    tail_node = VerifyIONode(wait_call.rank, wait_call.seq_id, wait_call.func)

        if tail_node:
            helper.recv_calls[global_dst][global_src].remove(recv_call_idx)
            break

    if tail_node :
        send_call.matched = True
        edge = MPIEdge(MPICallType.POINT_TO_POINT, head_node, tail_node)
        #print("match pt2pt: %s --> %s" %(edge.head, edge.tail))
        return edge
    else:
        print("Warnning: unmatched send call:", head_node, global_dst, send_call.stag)
        return None


'''
mpi_sync_calls=True will include only the calls
that guarantee synchronization, this flag is used
for checking MPI semantics
'''
#@profile
def match_mpi_calls(reader, mpi_sync_calls=False):
    edges = []
    helper = MPIMatchHelper(reader, mpi_sync_calls)
    helper.read_mpi_calls(reader)

    for rank in range(helper.num_ranks):
        for mpi_call in helper.all_mpi_calls[rank]:
            edge = None
            if mpi_call.matched:
                continue
            if helper.is_coll_call(mpi_call.func):
                edge = match_collective(mpi_call, helper)
            if helper.is_send_call(mpi_call.func):
                edge = match_pt2pt(mpi_call, helper)
            if edge:
                edges.append(edge)

    # validate result
    for rank in range(helper.num_ranks):
        recvs_sum = 0
        for i in range(helper.num_ranks):
            recvs_sum += len(helper.recv_calls[rank][i])
        if recvs_sum:
            print("Rank %d has %d unmatched recvs" %(rank, recvs_sum))
        if len(helper.coll_calls[rank]) != 0:
            print("Rank %d has %d unmatched colls" %(rank, len(helper.coll_calls[rank])))
        # No need to report unmatched test/wait calls. For example,
        # test calls with some MPI_REQUEST_NULL as input requrests may
        # not be set to matched and removed from the list
        #if len(helper.wait_test_calls[rank]) != 0:
        #    print("Rank %d has %d unmatched wait/test" %(rank, len(helper.wait_test_calls[rank])))

    return edges
