#!/usr/bin/env python
# encoding: utf-8
from enum import Enum
import networkx as nx

class MPICallType(Enum):
    ALL_TO_ALL     = 1
    ONE_TO_MANY    = 2
    MANY_TO_ONE    = 3
    POINT_TO_POINT = 4
    OTHER          = 5  # e.g., wait/test calls

class VerifyIONode:
    def __init__(self, rank, seq_id, func, fd = -1, mpifh = None):
        self.rank = rank
        self.seq_id = seq_id
        self.func = func
        # An integer maps to the filename
        self.fd = fd
        # Store the MPI-IO file handle so we can match
        # I/O calls with sync/commit calls during
        # the verification.
        self.mpifh = mpifh
    def graph_key(self):
        return str(self.rank) + "-" + \
               str(self.seq_id) + "-" + \
               str(self.func)
    def __str__(self):
        if "write" in self.func or "read" in self.func or \
            self.func.startswith("MPI_File_"):
                return "Rank %d: %dth %s(%s)" %(self.rank, self.seq_id, self.func, self.mpifh)
        else:
            return "Rank %d: %dth %s" %(self.rank, self.seq_id, self.func)


'''
Essentially a wrapper for networkx DiGraph
'''
class VerifyIOGraph:
    def __init__(self, nodes, edges, include_vc=False):
        self.G = nx.DiGraph()
        self.nodes = nodes
        self.__build_graph(nodes, edges, include_vc)

    def num_nodes(self):
        return len(self.G.nodes)

    # next (program-order) node of funcs in the sam rank
    def next_po_node(self, current, funcs):
        index = self.G.nodes[current.graph_key()]['index']
        nodes = self.nodes[current.rank]
        target = None
        for i in range(index+1, len(nodes)):
            if nodes[i].func in funcs:
                target = nodes[i]
                break
        return target

    # previous (program-order) node of funcs in the sam rank
    def prev_po_node(self, current, funcs):
        index = self.G.nodes[current.graph_key()]['index']
        nodes = self.nodes[current.rank]
        target = None
        for i in range(index-1, 0, -1):
            if nodes[i].func in funcs:
                target = nodes[i]
                break
        return target

    # next (happens-beofre) node of funcs in the target rank
    def next_hb_node(self, current, funcs, target_rank):
        target = None
        nodes = self.nodes[target_rank]
        for target in nodes:
            if (target.func in funcs) and (self.has_path(current, target)):
                targest.append(target)
                break
        return target

    def add_edge(self, h, t):
        self.G.add_edge(h.graph_key(), t.graph_key())

    def remove_edge(self, h, t):
        self.G.remove_edge(h.graph_key(), t.graph_key())

    def has_path(self, src, dst):
        return nx.has_path(self.G, src.graph_key(), dst.graph_key())

    def plot_graph(self, fname):
        import matplotlib.pyplot as plt
        nx.draw_networkx(self.G)
        #plt.savefig(fname)
        plt.show()

    def run_vector_clock(self):
        print("Run vector clock algorithm...")
        for node_key in nx.topological_sort(self.G):
            vc = self.G.nodes[node_key]['vc']
            for eachpred in self.G.predecessors(node_key):
                pred_vc = self.G.nodes[eachpred]['vc']
                vc = list(map(max, zip(vc, pred_vc)))

            self.G.nodes[node_key]['vc'] = vc
        print("Vector clock algorith finished")

    def shortest_path(self, src, dst):
        # Retrive rank from node key
        def key2rank(key):
            return (int)(key.split('-')[0])

        if (not src) or (not dst):
            print("shortest_path Error: must specify src and dst (VerifyIONode)")
            return []

        # nx.shortest_path will return a list of nodes in
        # keys. we then retrive the real VerifyIONode and return a
        # list of them
        path_in_keys = nx.shortest_path(self.G, src.graph_key(), dst.graph_key())
        path = []
        for key in path_in_keys:
            rank = key2rank(key)
            index = self.G.nodes[key]['index']
            path.append(self.nodes[rank][index])
        return path


    # private method to build the networkx DiGraph
    # called only by __init__
    # nodes: per rank of nodes of type VerifyIONode
    # Add neighbouring nodes of the same rank
    # This step will add all nodes
    def __build_graph(self, all_nodes, mpi_edges, include_vc):
        for rank in range(len(all_nodes)):
            for i in range(len(all_nodes[rank]) - 1):
                h = all_nodes[rank][i]
                t = all_nodes[rank][i+1]
                self.add_edge(h, t)

                # Include the original index of the `nodes`
                # i.e., not the seq id of the call.
                self.G.nodes[h.graph_key()]['index'] = i
                self.G.nodes[t.graph_key()]['index'] = i+1

                # Include vector clock for each node
                if include_vc:
                    vc1 = [0] * len(all_nodes)
                    vc2 = [0] * len(all_nodes)
                    vc1[rank] = i
                    vc1[rank] = i+1
                    self.G.nodes[h.graph_key()]['vc'] = vc1
                    self.G.nodes[t.graph_key()]['vc'] = vc2


        # Before calling this function, we should
        # have added all nodes. We use this function
        # to add edges of matching MPI calls
        ghost_node_index = 0
        for edge in mpi_edges:
            head, tail = edge.head, edge.tail

            # all-to-all
            # TODO is ther a many-to-many MPI call that
            # different senders and receivers?
            if edge.call_type == MPICallType.ALL_TO_ALL:
                # Opt:
                # Add a ghost node and connect all predecessors
                # and successors from all ranks. This prvents the circle
                '''
                ghost_node = (-1, ghost_node_index, "")
                ghost_node_index += 1
                for h in head:
                    origin_h, origin_t = h, None
                    for tmp in self.G.successors(graph_node_key(h)):
                        origin_t = tmp
                    self.add_edge(origin_h, ghost_node)
                    if origin_t:
                        remove_networkx_edge(self.G, origin_h, origin_t)
                        self.add_edge(ghost_node, origin_t)
                '''

                # Add all-to-all edges will create circle and prevent
                # the use of topological sort
                for i in range(len(head)):
                    for j in range(len(head)):
                        if i != j:
                            self.add_edge(head[i], head[j])
                            if head[i].rank == 0 and head[j].rank == 5:
                                print("add edge", head[i], head[j])
            # many-to-one, e.g., reduce
            elif edge.call_type == MPICallType.MANY_TO_ONE:
                for h in head:
                    self.add_edge(h, tail)
            # one-to-many, e.g., bcast
            elif edge.call_type == MPICallType.ONE_TO_MANY:
                for t in tail:
                    self.add_edge(head, t)
            # point-to-point, i.e., send-recv
            elif edge.call_type == MPICallType.POINT_TO_POINT:
                self.add_edge(head, tail)

        # Transitive clousre is too slow for large graphs
        #print("Generate transitive closure")
        #tc = nx.transitive_closure(self.G)

