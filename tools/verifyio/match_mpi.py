import sys
import gen_nodes
from gen_nodes import VerifyIOContext

edges = []

class com_node:
    def __init__(self, name, args):
        self.name = name
        self.args = args

def get_translation_table(reader):
    func_list = reader.funcs

    translate = {}
    translate['MPI_COMM_WORLD'] = range(reader.GM.total_ranks)

    for rank in range(reader.GM.total_ranks):
        records = reader.records[rank]
        for i in range(reader.LMs[rank].total_records):
            record = records[i]
            call = func_list[record.func_id]

            comm_id, local_rank, world_rank = None, rank, rank

            if call == 'MPI_Comm_split':
                comm_id = record.args[3]
                local_rank = int(record.args[4])
            if call == 'MPI_Comm_split_type':
                comm_id = record.args[4]
                local_rank = int(record.args[5])
            if call == 'MPI_Comm_dup':
                comm_id = record.args[1]
                local_rank = int(record.args[2])
            if call == 'MPI_Cart_create':
                comm_id = record.args[5]
                local_rank = int(record.args[6])
            if call == 'MPI_Comm_create':
                comm_id = record.args[2]
                local_rank = int(record.args[3])
            if call == 'MPI_Cart_sub':
                comm_id = record.args[2]
                local_rank = int(record.args[3])

            if comm_id:
                if comm_id not in translate:
                    translate[comm_id] = list(range(reader.GM.total_ranks))
                translate[comm_id][local_rank] = world_rank
    return translate

# Local rank to global rank
def local2global(translate_table, comm_id, local_rank):
    comm = comm_id.decode() if isinstance(comm_id, bytes) else comm_id
    if local_rank >= 0:
        return translate_table[comm][local_rank]
    # ANY_SOURCE
    return local_rank


#####################  DEFINING FUNCTIONS TO MATCH CALLS #########################

def find_wait_test_call(req, rank, context, need_match_src_tag=False, src=0, tag=0):

    found = None

    # We don't check the flag here, we checked it at gen_nodes.py
    # We are certain that calls in wait_test_calls all have completed
    # rquests
    for idx in context.wait_test_calls[rank]:
        wait_call = context.all_calls[rank][idx]
        func = wait_call.call

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
                    wait_call.req.remove(req)
                    wait_call.tindx.remove(str(inx))
                    if len(wait_call.tindx) == 0:
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

def match_collectives(node, context, translate):
    h = []
    t = []
    is_alltoall = context.is_all_to_all_call(node.call)

    for rank in range(context.num_ranks):
        key = node.get_key()

        if key in  context.coll_calls[rank]:
            other_idx = context.coll_calls[rank][key][0]
            other = context.all_calls[rank][other_idx]

            # Blocking calls
            if node.is_blocking_call():
                add_to_edge(h, t, other, is_alltoall)
            # Non-blocking calls
            else:
                wait_node = find_wait_test_call(other.req, rank, context)
                if wait_node:
                    add_to_edge(h, t, wait_node, is_alltoall)
                    h.append((wait_node.rank, wait_node.index, wait_node.func))

            # If no more call nodes have this key, then remove this key from the dict
            context.coll_calls[rank][key].pop(0)
            if(len(context.coll_calls[rank][key]) == 0):
                context.coll_calls[rank].pop(key)

            other.call = None


    node.call = None
    edges.append((h, t))

#@profile
def match_pt2pt(send_call, context, translate):

    h = (send_call.rank, send_call.index, send_call.func, send_call.tstart)
    t = None

    comm = send_call.comm
    global_dst =  local2global(translate, comm, send_call.dst)
    global_src = send_call.rank

    for recv_call_idx in context.recv_calls[global_dst][global_src]:
        recv_call = context.all_calls[global_dst][recv_call_idx]

        # Check for comm, src, and tag.
        if recv_call.comm != comm: continue

        if (recv_call.rtag == send_call.stag or recv_call.rtag == gen_nodes.ANY_TAG):

            if recv_call.is_blocking_call():
                t = (recv_call.rank, recv_call.index, recv_call.func, recv_call.tend)
            else:
                if recv_call.rtag == gen_nodes.ANY_TAG or global_src == gen_nodes.ANY_SOURCE:
                    wait_call = find_wait_test_call(recv_call.req, global_dst, context, True, send_call.rank, send_call.stag)
                else:
                    wait_call = find_wait_test_call(recv_call.req, global_dst, context)
                if wait_call:
                    t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)

        if t:
            context.recv_calls[global_dst][global_src].remove(recv_call_idx)
            break

    if t == None:
        print("TODO not possible", h, global_dst, send_call.stag)
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
    context.generate_mpi_nodes(reader, translate)

    for rank in range(context.num_ranks):
        for node in context.all_calls[rank]:

            if context.is_coll_call(node.call):
                match_collectives(node, context, translate)

            if context.is_send_call(node.call):
                match_pt2pt(node, context, translate)

    # validate result
    for rank in range(context.num_ranks):
        recvs_sum = 0
        for i in range(context.num_ranks):
            recvs_sum += len(context.recv_calls[rank][i])
        if recvs_sum:
            print("Rank %d still has unmatched recvs: %d" %(rank, recvs_sum))
        if len(context.coll_calls[rank]) != 0:
            print("Rank %d still has unmatched colls: %d" %(rank, len(context.coll_calls[rank])))
        if len(context.wait_test_calls[rank]) != 0:
            print("Rank %d still has unmatched wait/test: %d" %(rank, len(context.wait_test_calls[rank])))

    return context.all_calls, edges
