import sys, argparse
import networkx as nx
from recorder_viz import RecorderReader
from match_mpi import match_mpi_calls
from read_conflicts import read_conflicting_accesses
from gen_networkx import generate_graph, has_path, graph_node_key, run_vector_clock

def check_posix_semantics(G, pairs):
    properly_synchronized = True

    print("Conflicting pairs: %d" %len(pairs))
    for pair in pairs:
        src = graph_node_key(pair[0])
        dst = graph_node_key(pair[1])
        reachable = has_path(G, src, dst)
        if not reachable: properly_synchronized = False
        print("Conflicting I/O operations: %s --> %s, properly synchronized: %s" %(src, dst, reachable))

    return properly_synchronized

def print_shortest_path(G, src, dst, nodes):
    def key2rank(key):
        return int(key.split('-')[0])
    def key2func(key):
        rank = key2rank(key)
        idx = G.nodes[key]['origin_idx']
        return nodes[rank][idx].func

    path = nx.shortest_path(G, src, dst)
    current_rank = key2rank(src)

    selected_keys = [src]
    for i in range(1, len(path)):
        r = key2rank(path[i])
        if current_rank != r:
            if selected_keys[-1] != path[i-1]:
                selected_keys.append(path[i-1])
            if selected_keys[-1] != path[i]:
                selected_keys.append(path[i])
            current_rank = r
    if selected_keys[-1] != dst:
        selected_keys.append(dst)

    path_str = ""
    for key in selected_keys:
        path_str += " -> (%s)%s" %(key, key2func(key))
    print path
    return path_str


def check_mpi_semantics(G, nodes, pairs):
    properly_synchronized = True

    print("Conflicting pairs: %d" %len(pairs))
    for pair in pairs:
        p1, p2 = pair[0], pair[1]                   # of Call class
        if p1.rank == p2.rank: continue             # Same rank conflicts do no cause a issue on most file systems.

        idx1, idx2 = -1, -1
        for i in range(len(nodes[p1.rank])):
            if p1.index == nodes[p1.rank][i].index:
                idx1 = i
                break
        for i in range(len(nodes[p2.rank])):
            if p2.index == nodes[p2.rank][i].index:
                idx2 = i
                break

        next_sync, prev_sync = None, None
        # Find the next sync() call for first access
        for i in range(idx1+1, len(nodes[p1.rank])):
            record = nodes[p1.rank][i]
            if record.func == 'MPI_File_sync' or record.func == 'MPI_File_open' \
                    or record.func == 'MPI_File_close':
                next_sync = record
                break

        # Find the previous sync() call for second access
        for i in range(idx2-1, 0, -1):
            record = nodes[p2.rank][i]
            if record.func == 'MPI_File_sync' or record.func == 'MPI_File_open' \
                    or record.func == 'MPI_File_close':
                prev_sync = record
                break

        src = graph_node_key(next_sync)
        dst = graph_node_key(prev_sync)
        reachable = (next_sync and prev_sync) and has_path(G, src, dst)
        print("%s --> %s, properly synchronized: %s" %(graph_node_key(pair[0]), graph_node_key(pair[1]), reachable))

        if reachable:
            #path_str = print_shortest_path(G, src, dst, nodes)
            #print("(%s)%s%s -> (%s)%s\n"
            #        %(graph_node_key(pair[0]), pair[0].func, \
            #            path_str, \
            #            graph_node_key(pair[1]), pair[1].func))
            print("\tPath: %s --> %s --> %s --> %s, properly synchronized: true" %(graph_node_key(pair[0]), src, dst, graph_node_key(pair[1])))

    return properly_synchronized


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("traces_folder")
    parser.add_argument("conflicts_file", default=None)
    parser.add_argument("--semantics", type=str, choices=["posix", "mpi"], default="mpi", help="Verify if I/O operations are properly synchronized under the specific semantics")
    args = parser.parse_args()
    sync_calls_only = args.semantics == "mpi"

    reader = RecorderReader(sys.argv[1])
    nodes, edges = match_mpi_calls(reader, sync_calls_only)

    # Add the I/O nodes (conflicting I/O accesses)
    # to the graph. We add them after the match()
    # because they are not used for synchronizaitons
    total_nodes = 0
    if args.conflicts_file:
        conflicting_nodes, pairs = read_conflicting_accesses(args.conflicts_file, reader.GM.total_ranks)

    for rank in range(reader.GM.total_ranks):
        if args.conflicts_file:
            nodes[rank] += conflicting_nodes[rank]
        nodes[rank] = sorted(nodes[rank], key=lambda x: x.index)
        total_nodes += len(nodes[rank])

    G = generate_graph(nodes, edges, include_vc=False)
    print("Nodes: %d, Edges: %d" %(len(G.nodes()), len(G.edges())))

    # plot_graph(G, reader.GM.total_ranks)
    # run_vector_clock(G)

    if args.conflicts_file:
        print("\nUse %s semantics" %args.semantics)
        if sync_calls_only:
            check_mpi_semantics(G, nodes, pairs)
        else:
            check_posix_semantics(G, pairs)
