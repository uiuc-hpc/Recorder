import argparse, time
from recorder_viz import RecorderReader
from read_nodes import read_mpi_nodes, read_io_nodes
from match_mpi import match_mpi_calls
from verifyio_graph import VerifyIONode, VerifyIOGraph

'''
def verify_proper_synchronization(G, S, R, pairs):
    O = [0 * len(S)]

    # for every conflict pair
    for pair in pairs:
        n1 = graph_node_key(pair[0])
        n2 = graph_node_key(pair[1])

        # from left to right, for every S[i]
        # narrow it down to a single op O[i]
        n_left = n1
        for i in range(0, len(S), 1):
            if R[i] == "po":
                O[i] = next_op(n_left, S[i])
                # Todo: if O[i] is empty
                # then the conflict must be
                # not properly synchornized
                n_left = O[i]
            else:
                break
    
        # from right to left, for every S[i]
        # narrow it down to a single op O[i]
        n_right = n2
        for i in range(len(S), 0, -1):
            if R[i+1] == "po":
                O[i] = perv_op(n_right, S[i])
                n_right = O[i]
            else:
                break

        # then do the final pass
        n_left = n1
        for i in range(0, len(S), 1):
            # already narrowed down
            if O[i]:
                n_left = O[i]
                continue
            # hb
            for o1 in n_left:
                for every rank:
                    o2 = next_op(None, S[i])
                    if tc[o1, o2]:
                        O[i] += o2
            if not O[i]:
                return

'''


def verify_posix_semantics(G, conflict_pairs):
    properly_synchronized = True

    for pair in conflict_pairs:
        n1 = pair[0]
        n2 = pair[1]
        reachable = G.has_path(n1, n2) or G.has_path(n1, n2)
        if reachable:
            print(get_shortest_path(G, n1, n2))
        else:
            properly_synchronized = False
        print("%s <--> %s, properly synchronized: %s" %(n1, n2, reachable))

    return properly_synchronized

def get_shortest_path(G, src, dst):
    path = G.shortest_path(src, dst)
    path_str = ""
    for i in range(len(path)):
        node = path[i]
        path_str += str(node) 
        if i != len(path) - 1:
            path_str += "->"
    return path_str

def verify_session_semantics(G, conflict_pairs, 
                             close_ops=["close", "fclose"],
                             open_ops=["open", "fopen"]):
    for pair in conflict_pairs:
        n1, n2 = pair[0], pair[1]                   # of VerifyIONode class
        next_sync = G.next_po_node(n1, close_ops)
        prev_sync = G.prev_po_node(n2, open_ops)
        reachable = (bool) ( (next_sync and prev_sync) and G.has_path(next_sync, prev_sync) )
        if reachable:
            path_str = get_shortest_path(G, next_sync, prev_sync)
            print("%s -> %s -> %s" %(n1, path_str, n2))
        else:
            properly_synchronized = False
        print("%s <--> %s, properly synchronized: %s" %(n1, n2, reachable))
    return properly_synchronized

def verify_mpi_semantics(G, conflict_pairs):
    return verify_session_semantics(G, conflict_pairs,
                             close_ops = ["MPI_File_sync", "MPI_File_close"], \
                             open_ops  = ["MPI_File_sync", "MPI_File_open"])

def verify_commit_semantics(G, conflict_pairs):
    properly_synchronized = True
    for pair in conflict_pairs:
        n1, n2 = pair[0], pair[1]                   # of VerifyIONode class

        this_pair_ok = False
        # TODO need to consider all ranks
        ranks = [n1.rank, n2.rank]
        for rank in ranks:
            next_commit = G.next_hb_node(n1, ["fsync", "close"], rank)
            if (next_commit) and G.has_path(next_commit, n2):
                this_pair_ok = True
                break
        if not this_pair_ok: properly_synchronized = False
        print("%s <--> %s, properly synchronized: %s" %(n1, n2, this_pair_ok))

    return properly_synchronized

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("traces_folder")
    #parser.add_argument("conflicts_file")
    parser.add_argument("--semantics", type=str, choices=["POSIX", "MPI-IO", "Commit", "Session"],
                        default="MPI-IO", help="Verify if I/O operations are properly synchronized under the specific semantics")
    args = parser.parse_args()

    reader = RecorderReader(args.traces_folder)

    mpi_nodes = read_mpi_nodes(reader)
    io_nodes, conflict_pairs = read_io_nodes(reader, args.traces_folder+"/conflicts.txt")

    all_nodes = mpi_nodes
    for rank in range(reader.GM.total_ranks):
        all_nodes[rank] += io_nodes[rank]
        all_nodes[rank] = sorted(all_nodes[rank], key=lambda x: x.seq_id)

    # get mpi calls and matched edges
    t1 = time.time()
    mpi_edges = match_mpi_calls(reader)
    t2 = time.time()
    print("match mpi calls: %.3f secs, mpi edges: %d" %((t2-t1),len(mpi_edges)))

    t1 = time.time()
    G = VerifyIOGraph(all_nodes, mpi_edges, include_vc=False)
    t2 = time.time()
    print("build happens-before graph: %.3f secs, nodes: %d" %((t2-t1), G.num_nodes()))

    # G.plot_graph()
    # G.run_vector_clock()

    if args.semantics == "POSIX":
        p = verify_posix_semantics(G, conflict_pairs)
    elif args.semantics == "MPI-IO":
        p = verify_mpi_semantics(G, conflict_pairs)
    elif args.semantics == "Commit":
        p = verify_commit_semantics(G, conflict_pairs)
    elif args.semantics == "Session":
        p = verify_session_semantics(G, conflict_pairs)

    if p:
        print("\nProperly synchronized under %s semantics" %args.semantics)
    else:
        print("\nNot properly synchronized under %s semantics" %args.semantics)
