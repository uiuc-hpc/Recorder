import sys
from gen_nodes import generate_mpi_nodes

ANY_SOURCE = -2
ANY_TAG = -1
edges = []
max_drift = 0   # maximum clock drift

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

def match_collectives(node, nodes, translate):

    participants = [(node.rank, node.index, node.func, node.tstart, node.tend)]
    max_tstart = node.tstart
    min_tend = node.tend

    for rank in range(len(nodes)):
        if rank == node.rank: continue
        for other in nodes[rank]:
            if other.call == node.call and other.comm == node.comm and other.req == node.req and other.reqflag == node.reqflag:
                participants.append((other.rank, other.index, other.func, other.tstart, other.tend))
                other.call = None

                max_tstart = max(max_tstart, other.tstart)
                min_tend = min(min_tend, other.tend)
                break

    if max_tstart >= min_tend:
        global max_drift
        max_drift = max(max_drift, (max_tstart-min_tend))
        #print("%s, drift:%f, max: %f us" %(node.call, max_tstart-min_tend, max_drift*1000000))

    #print("Match %s: %s" %(node.call, participants))
    node.call = None
    edges.append((participants, participants))


def match_redgat(root_node, nodes):
    h = []
    t = (root_node.rank, root_node.index, root_node.func)

    call_to_match = root_node.call

    if call_to_match == 'MPI_Reduce':
        alt_dest = 'MPI_Ireduce'

    elif call_to_match == 'MPI_Ireduce':
        alt_dest = 'MPI_Reduce'

    elif call_to_match == 'MPI_Gather':
        alt_dest = 'MPI_Igather'

    elif call_to_match == 'MPI_Igather':
        alt_dest = 'MPI_Gather'

    elif call_to_match == 'MPI_Gatherv':
        alt_dest = 'MPI_Igatherv'

    elif call_to_match == 'MPI_Igatherv':
        alt_dest = 'MPI_Gatherv'


    for rank in range(len(nodes)):
        if root_node.rank == rank:
            continue

        for src_node in nodes[rank]:
            if root_node.comm != src_node.comm:
                continue

            if src_node.call == root_node.call and src_node.dst == root_node.dst:
                h.append((src_node.rank, src_node.index, src_node.func))
                src_node.call = None
                break

            if src_node.call == alt_dest and src_node.dst == root_node.dst:
                req = src_node.req
                src_node.call = None

                # TODO should start from current dst_node
                for wait_call in nodes[rank]:
                    if wait_call.call == 'MPI_Wait' or wait_call.call == 'MPI_Waitall':
                        if req in wait_call.req:
                            h.append((wait_call.rank, wait_call.index, wait_call.func))
                            wait_call.req.remove(req)
                            break

                    elif wait_call.call == 'MPI_Waitany':
                        if req in wait_call.req:
                            inx = wait_call.req.index(req)
                            if str(inx) in wait_call.tindx:
                                h.append = ((wait_call.rank, wait_call.index, wait_call.func))
                                wait_call.call = None
                                break

                    elif (wait_call.call == 'MPI_Waitsome' or wait_call.call == 'MPI_Testsome') and (wait_call.reqflag > 0):
                        if req in wait_call.req:
                            inx = wait_call.req.index(req)
                            if str(inx) in wait_call.tindx:
                                h.append = ((wait_call.rank, wait_call.index, wait_call.func))
                                wait_call.req[inx] = 0
                                wait_call.reqflag = wait_call.reqflag - 1
                                break

                    elif (wait_call.call == 'MPI_Test' or wait_call.call == 'MPI_Testall') and wait_call.reqflag:
                        if req in wait_call.req:
                            h.append((wait_call.rank, wait_call.index, wait_call.func))
                            wait_call.call = None
                            break

                    elif wait_call.call == 'MPI_Testany' and wait_call.reqflag:
                        if req in wait_call.req:
                            inx = wait_call.req.index(req)
                            if str(inx) in wait_call.tindx:
                                h.append = ((wait_call.rank, wait_call.index, wait_call.func))
                                wait_call.call = None
                                break

    #print("Match %s: %s --> %s" %(root_node.call, h, t))
    root_node.call = None
    edges.append((h, t))

def match_bcast(root_node, nodes):

    h = (root_node.rank, root_node.index, root_node.func)
    t = []

    for rank in range(len(nodes)):
        if root_node.rank == rank:
            continue

        for dst_node in nodes[rank]:
            if root_node.comm != dst_node.comm:
                continue

            if dst_node.call == 'MPI_Bcast' and dst_node.src == root_node.src:
                # done
                t.append((dst_node.rank, dst_node.index, dst_node.func))
                dst_node.call = None
                break

            elif dst_node.call == 'MPI_Ibcast' and dst_node.src == root_node.src:
                req = dst_node.req
                dst_node.call = None

                # TODO should start from current dst_node
                for wait_call in nodes[rank]:
                    if wait_call.call == 'MPI_Wait' or wait_call.call == 'MPI_Waitall':
                        if req in wait_call.req:
                            t.append((wait_call.rank, wait_call.index, wait_call.func))
                            wait_call.req.remove(req)
                            break

                    elif wait_call.call == 'MPI_Waitany':
                        if req in wait_call.req:
                            inx = wait_call.req.index(req)
                            if str(inx) in wait_call.tindx:
                                t.append = ((wait_call.rank, wait_call.index, wait_call.func))
                                wait_call.call = None
                                break

                    elif (wait_call.call == 'MPI_Waitsome' or wait_call.call == 'MPI_Testsome') and (wait_call.reqflag > 0):
                        if req in wait_call.req:
                            inx = wait_call.req.index(req)
                            if str(inx) in wait_call.tindx:
                                h.append = ((wait_call.rank, wait_call.index, wait_call.func))
                                wait_call.req[inx] = 0
                                wait_call.reqflag = wait_call.reqflag - 1
                                break

                    elif (wait_call.call == 'MPI_Test' or wait_call.call == 'MPI_Testall') and wait_call.reqflag:
                        if req in wait_call.req:
                            t.append((wait_call.rank, wait_call.index, wait_call.func))
                            wait_call.call = None
                            break

                    elif wait_call.call == 'MPI_Testany' and wait_call.reqflag:
                        if req in wait_call.req:
                            inx = wait_call.req.index(req)
                            if str(inx) in wait_call.tindx:
                                t.append = ((wait_call.rank, wait_call.index, wait_call.func))
                                wait_call.call = None
                                break
                    break

    #print("Match %s: %s --> %s" %(root_node.call, h, t))
    root_node.call = None
    edges.append((h, t))


def find_recv(send_call, nodes, translate):

    h = (send_call.rank, send_call.index, send_call.func, send_call.tstart)
    t = None
    found = False

    comm = send_call.comm
    global_dst =  local2global(translate, comm, send_call.dst)

    for recv_call in nodes[global_dst]:
        if recv_call.comm != comm:
            continue

        if recv_call.call == 'MPI_Recv' or recv_call.call == 'MPI_Sendrecv':
            if recv_call.src is not None:
                global_src = local2global(translate, comm, recv_call.src)

                if global_src == send_call.rank or global_src == ANY_SOURCE:
                    if (recv_call.rtag == send_call.stag) or (recv_call.rtag == ANY_TAG):
                        # find match!
                        t = (recv_call.rank, recv_call.index, recv_call.func, recv_call.tend)
                        edges.append((h, t))

                        # Remove this call
                        # TODO needs a better way
                        recv_call.src = None
                        found = True

        elif recv_call.call == 'MPI_Irecv':
            global_src = local2global(translate, comm, recv_call.src)
            if global_src == send_call.rank or global_src == ANY_SOURCE:
                if (recv_call.rtag == send_call.stag) or (recv_call.rtag == ANY_TAG):
                    req = recv_call.req
                    recv_call.call = None

                    # TODO should start from current node
                    for wait_call in nodes[global_dst]:
                        if wait_call.call == 'MPI_Wait' or wait_call.call == 'MPI_Waitall':
                            if req in wait_call.req:
                                t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)
                                edges.append((h, t))
                                wait_call.req.remove(req)
                                found = True
                                break
                        elif wait_call.call == 'MPI_Waitany':
                            if req in wait_call.req:
                                inx = wait_call.req.index(req)
                                if str(inx) in wait_call.tindx:
                                    t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)
                                    wait_call.call = None
                                    found = True
                                    break
                        elif (wait_call.call == 'MPI_Waitsome' or wait_call.call == 'MPI_Testsome'):
                            if req in wait_call.req:
                                inx = wait_call.req.index(req)
                                if str(inx) in wait_call.tindx:
                                    t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)
                                    wait_call.req[inx] = 0
                                    wait_call.reqflag = wait_call.reqflag - 1
                                    found = True
                                    break
                        elif (wait_call.call == 'MPI_Test' or wait_call.call == 'MPI_Testall') and wait_call.reqflag:
                            if req in wait_call.req:
                                t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)
                                wait_call.call = None
                                found = True
                                break
                        elif wait_call.call == 'MPI_Testany' and wait_call.reqflag:
                            if req in wait_call.req:
                                inx = wait_call.req.index(req)
                                if str(inx) in wait_call.tindx:
                                    t = (wait_call.rank, wait_call.index, wait_call.func, wait_call.tend)
                                    wait_call.call = None
                                    found = True
                                    break

        # Out-most for loop
        if found:
            if h[3] >= t[3]:
                global max_drift
                max_drift = max(max_drift, h[3]-t[3])
                #print("Match pt2pt: %s --> %s; Drift: %f, max: %f us" %(h, t, h[2]-t[2], max_drift*1000000))
            return


'''
mpi_sync_calls=True will include only the calls
that guarantee synchronization, this flag is used
for checking MPI semantics
'''
def match_mpi_calls(reader, mpi_sync_calls=False):

    bcast_calls =  ['MPI_Bcast', 'MPI_Ibcast']
    redgat_calls = ['MPI_Reduce', 'MPI_Ireduce', 'MPI_Gather', 'MPI_Igather', 'MPI_Gatherv', 'MPI_Igatherv']
    coll_calls =   ['MPI_Barrier', 'MPI_Allreduce', 'MPI_Allgatherv', 'MPI_Allgatherv', 'MPI_Alltoall', 'MPI_Reduce_scatter',
                    'MPI_File_open', 'MPI_File_close', 'MPI_File_read_all', 'MPI_File_read_at_all', 'MPI_File_read_order',
                    'MPI_File_write_all', 'MPI_File_write_at_all', 'MPI_File_write_ordered', 'MPI_File_set_size',
                    'MPI_File_set_view', 'MPI_File_sync', 'MPI_Comm_dup', 'MPI_Comm_split', 'MPI_Comm_split_type',
                    'MPI_Cart_create', 'MPI_Cart_sub']

    if mpi_sync_calls:
        bcast_calls =  []
        redgat_calls = ['MPI_Reduce_scatter', 'MPI_Reduce_scatter_block']
        coll_calls =   ['MPI_Barrier', 'MPI_Allgather', 'MPI_Alltoall', 'MPI_Alltoallv', 'MPI_Alltoallw', 'MPI_Allreduce']

    translate = get_translation_table(reader)
    nodes = generate_mpi_nodes(reader)

    for rank in range(len(nodes)):
        for node in nodes[rank]:
            if node.call in bcast_calls:
                root = local2global(translate, node.comm, node.src)
                if root == rank:
                    match_bcast(node, nodes)

            if node.call in redgat_calls:
                root = local2global(translate, node.comm, node.dst)
                if root == rank:
                    match_redgat(node, nodes)

            if node.call in coll_calls:
                match_collectives(node, nodes, translate)

            if node.call in ['MPI_Send','MPI_Ssend','MPI_Isend','MPI_Sendrecv']:
                find_recv(node, nodes, translate)

    return nodes, edges
