#!/usr/bin/env python
# encoding: utf-8
import networkx as nx

class VerifyIONode:
    def __init__(self, rank, seq_id, func):
        self.rank = rank
        self.seq_id = seq_id
        self.func = func
    def graph_key(self):
        return str(self.rank) + "-" + \
               str(self.seq_id) + "-" + \
               str(self.func)
    def __str__(self):
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

    def next_po_node(self, current, funcs):
        index = self.G.nodes[current.graph_key()]['index']
        nodes = self.nodes[current.rank]
        target = None
        for i in range(index+1, len(nodes)):
            if nodes[i].func in funcs:
                target = nodes[i]
                break
        return target

    def prev_po_node(self, current, funcs):
        index = self.G.nodes[current.graph_key()]['index']
        nodes = self.nodes[current.rank]
        target = None
        for i in range(index-1, 0, -1):
            if nodes[i].func in funcs:
                target = nodes[i]
                break
        return target

    def add_edge(self, h, t):
        self.G.add_edge(h.graph_key(), t.graph_key())

    def remove_edge(self, h, t):
        self.G.remove_edge(h.graph_key(), t.graph_key())

    def has_path(self, src, dst):
        return nx.has_path(self.G, src.graph_key(), dst.graph_key())

    def plot_graph(self):
        nx.draw_networkx(self.G)
        import matplotlib.pyplot as plt
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

    # private method to build the networkx DiGraph
    # called only by __init__
    # nodes: per rank of nodes of type VerifyIONode
    # Add neighbouring nodes of the same rank
    # This step will add all nodes
    def __build_graph(self, nodes, mpi_edges, include_vc):
        for rank in range(len(nodes)):
            for i in range(len(nodes[rank]) - 1):
                h = nodes[rank][i]
                t = nodes[rank][i+1]
                self.add_edge(h, t)

                # Include the original index of the `nodes`
                # i.e., not the seq id of the call.
                self.G.nodes[h.graph_key()]['index'] = i
                self.G.nodes[t.graph_key()]['index'] = i+1

                # Include vector clock for each node
                if include_vc:
                    vc1 = [0] * len(nodes)
                    vc2 = [0] * len(nodes)
                    vc1[rank] = i
                    vc1[rank] = i+1
                    self.G.nodes[h.graph_key()]['vc'] = vc1
                    self.G.nodes[t.graph_key()]['vc'] = vc2


        # Before calling this function, we should
        # have added all nodes. We use this function
        # to add edges of matching MPI calls
        ghost_node_index = 0
        for edge in mpi_edges:
            head, tail = edge[0], edge[1]

            # many-to-many, e.g., barrier, alltoall
            if type(head) == list and type(tail) == list:
                if len(head) == len(tail):    
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
                    for idx in range(len(head) - 1):
                        h, t = head[idx], head[idx+1]
                        self.add_edge(h, t)
                        self.add_edge(t, h)
                else:
                    for h in head:
                        for t in tail:
                            self.add_edge(h, t)
            # many-to-one, e.g., reduce
            elif type(head) == list:
                for h in head:
                    self.add_edge(h, tail)
            # one-to-many, e.g., bcast
            elif type(tail) == list:
                for t in tail:
                    self.add_edge(head, t)
            # point-to-point, i.e., send-recv
            else:
                self.add_edge(head, tail)

        # Transitive clousre is too slow for large graphs
        #print("Generate transitive closure")
        #tc = nx.transitive_closure(self.G)

