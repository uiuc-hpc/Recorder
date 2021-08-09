import sys
from gen_nodes import VerifyIOContext

ANY_SOURCE = -2
ANY_TAG = -1
edges = []

class com_node:
    def __init__(self, name, args):
        self.name = name
        self.args = args

def get_translation_table(reader):
    func_list = reader.funcs

    new_comms = {}

    for rank in range(reader.GM.total_ranks):
        records = reader.records[rank]
        for i in range(reader.LMs[rank].total_records):
            record = records[i]
            call = func_list[record.func_id]

            uid, order, world_rank = None, rank, rank

            if call == 'MPI_Comm_split':
                uid = record.args[3]
                order = int(record.args[2])
            if call == 'MPI_Comm_split_type':
                uid = record.args[4]
                order = int(record.args[2])
            if call == 'MPI_Comm_dup':
                uid = record.args[1]
            if call == 'MPI_Cart_create':
                uid = record.args[5]
            if call == 'MPI_Comm_create':
                uid = record.args[2]
            if call == 'MPI_Cart_sub':
                uid = record.args[2]

            if uid:
                if uid in new_comms:
                    new_comms[uid].append((order, world_rank))
                else:
                    new_comms[uid] = [(order, world_rank)]

    translate = {}
    translate['MPI_COMM_WORLD'] = range(reader.GM.total_ranks)

    for uid, procs in new_comms.items():
        translate[uid] = []
        procs.sort(key = lambda x: x[1])
        for order, world_rank in procs:
            translate[uid].append(world_rank)

    return translate

# Local rank to global rank
def local2global(translate_table, comm_id, local_rank):
    if local_rank == ANY_SOURCE:
        return ANY_SOURCE
    if local_rank >= 0:
        return translate_table[comm_id][local_rank]
    return -1


#####################  DEFINING FUNCTIONS TO MATCH CALLS #########################

def find_wait_test_call(req, rank, context):

    found = None

    for idx in context.wait_test_calls[rank]:
        wait_call = context.all_calls[rank][idx]
        func = wait_call.call

        if (func == 'MPI_Wait' or func == 'MPI_Waitall') or \
           ((func == 'MPI_Test' or func == 'MPI_Testall')):
            if req in wait_call.req:
                found = wait_call
                wait_call.req.remove(req)
                if(len(wait_call.req) == 0):
                    context.wait_test_calls[rank].remove(idx)
                break

        elif func == 'MPI_Waitany' or func == 'MPI_Testany':
            if req in wait_call.req:
                inx = wait_call.req.index(req)
                if str(inx) in wait_call.tindx:
                    found = wait_call
                    context.wait_test_calls[rank].remove(idx)
                    break

        elif func == 'MPI_Waitsome' or func == 'MPI_Testsome':
            if req in wait_call.req:
                inx = wait_call.req.index(req)
                if str(inx) in wait_call.tindx:
                    found = wait_call
                    wait_call.req.remove(inx)
                    wait_call.reqflag = wait_call.reqflag - 1
                    if wait_call.reqflag == 0:
                        context.wait_test_calls[rank].remove(idx)
                    break
    return found

def add_to_edge(h, t, node, is_all_to_all):
    if is_all_to_all:
        h.append((node.rank, node.index, node.func, node.tstart, node.tend))
        t.append((node.rank, node.index, node.func, node.tstart, node.tend))
        pass
    else:
        if node.rank == node.src:
            h.append((node.rank, node.index, node.func, node.tstart, node.tend))
        else:
            t.append((node.rank, node.index, node.func, node.tstart, node.tend))
            pass

def match_collectives(node, context, translate):
    h = []
    t = []
    is_alltoall = context.is_all_to_all_call(node.call)

    for rank in range(context.num_ranks):
        key = node.get_key()

        if key in  context.coll_calls[rank]:
            other_idx = context.coll_calls[rank][key][0]
            other = context.all_calls[rank][other_idx]

            # Non-blocking calls
            if node.call.startswith("MPI_I"):
                # TODO should start from current dst_node
                wait_node = find_wait_test_call(other.req, rank, context)
                if wait_node:
                    add_to_edge(h, t, wait_node, is_alltoall)
                    h.append((wait_node.rank, wait_node.index, wait_node.func))
            # Blocking calls
            else:
                add_to_edge(h, t, other, is_alltoall)

            # If no more call nodes have this key, then remove this key from the dict
            context.coll_calls[rank][key].pop(0)
            if(len(context.coll_calls[rank][key]) == 0):
                context.coll_calls[rank].pop(key)

            other.call = None
            break;

    node.call = None
    edges.append((h, t))

#@profile
def match_pt2pt(send_call, context, translate):

    h = (send_call.rank, send_call.index, send_call.func, send_call.tstart)
    t = None

    comm = send_call.comm
    global_dst =  local2global(translate, comm, send_call.dst)

    for recv_call_idx in context.recv_calls[global_dst]:
        recv_call = context.all_calls[global_dst][recv_call_idx]
        if recv_call.comm != comm:
            continue

        if recv_call.call == 'MPI_Recv' or recv_call.call == 'MPI_Sendrecv':
            if recv_call.src is not None:
                global_src = local2global(translate, comm, recv_call.src)
                if (global_src == send_call.rank or global_src == ANY_SOURCE) and \
                   (recv_call.rtag == send_call.stag or recv_call.rtag == ANY_TAG):
                    t = (recv_call.rank, recv_call.index, recv_call.func, recv_call.tend)
                    context.recv_calls[global_dst].remove(recv_call_idx)
                    break

        elif recv_call.call == 'MPI_Irecv':
            global_src = local2global(translate, comm, recv_call.src)
            if (global_src == send_call.rank or global_src == ANY_SOURCE) and \
               (recv_call.rtag == send_call.stag or recv_call.rtag == ANY_TAG):
                req = recv_call.req
                context.recv_calls[global_dst].remove(recv_call_idx)
                # TODO should start from current node
                wait_call = find_wait_test_call(req, global_dst, context)
                if wait_call:
                    t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)
                    break

    edges.append((h, t))


'''
mpi_sync_calls=True will include only the calls
that guarantee synchronization, this flag is used
for checking MPI semantics
'''
#@profile
def match_mpi_calls(reader, mpi_sync_calls=False):

    translate = get_translation_table(reader)

    context = VerifyIOContext(reader, mpi_sync_calls)
    context.generate_mpi_nodes(reader)

    for rank in range(context.num_ranks):

        for node in context.all_calls[rank]:

            if context.is_coll_call(node.call):
                match_collectives(node, context, translate)

            if context.is_send_call(node.call):
                match_pt2pt(node, context, translate)

    # validate result
    '''
    for rank in range(context.num_ranks):
        if len(context.recv_calls[rank]) != 0:
            print("No!")
        if len(context.coll_calls[rank]) != 0:
            print("No!", context.coll_calls[rank])
    '''
    return context.all_calls, edges