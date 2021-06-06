#!/usr/bin/env python
# encoding: utf-8

import networkx as nx


def graph_node_key(node):
    if type(node) == str:       # already a key string
        return node
    rank, seq_id, func = 0, 0, ""
    if type(node) == tuple:
        rank, seq_id, func = node[0], node[1], node[2]
    else:
        rank, seq_id, func = node.rank, node.index, node.func
    return str(rank) + "-" + str(seq_id) + "-" + func

def add_networkx_edge(G, h_node, t_node):
    h_key = graph_node_key(h_node)
    t_key = graph_node_key(t_node)
    G.add_edge(h_key, t_key)

def remove_networkx_edge(G, h_node, t_node):
    h_key = graph_node_key(h_node)
    t_key = graph_node_key(t_node)
    G.remove_edge(h_key, t_key)

def generate_graph(nodes, edges, include_vc=False):
    print("Build graph...")
    G = nx.DiGraph()

    for rank in range(len(nodes)):
        for i in range(len(nodes[rank]) - 1):
            h_node = nodes[rank][i]
            t_node = nodes[rank][i+1]
            add_networkx_edge(G, h_node, t_node)

            # Include the original index in the `nodes`
            G.nodes[graph_node_key(h_node)]['origin_idx'] = i
            G.nodes[graph_node_key(t_node)]['origin_idx'] = i+1

            # Include vector clock for each node
            if include_vc:
                vc1 = [0] * len(nodes)
                vc2 = [0] * len(nodes)
                vc1[rank] = i
                vc1[rank] = i+1
                G.nodes[graph_node_key(h_node)]['vc'] = vc1
                G.nodes[graph_node_key(t_node)]['vc'] = vc2

    ghost_node_index = 0
    for edge in edges:
        head, tail = edge[0], edge[1]
        if type(head) == tuple and type(tail) == tuple:
            add_networkx_edge(G, head, tail)
        elif type(head) == list and type(tail) == list:
            if head == tail:    # e.g., barrier, alltoall
                # Opt:
                # Add a ghost node and connect all predecessors
                # and successors from all ranks. This prvents the circle
                '''
                ghost_node = (-1, ghost_node_index, "")
                ghost_node_index += 1
                for h in head:
                    origin_h, origin_t = h, None
                    for tmp in G.successors(graph_node_key(h)):
                        origin_t = tmp
                    add_networkx_edge(G, origin_h, ghost_node)
                    if origin_t:
                        remove_networkx_edge(G, origin_h, origin_t)
                        add_networkx_edge(G, ghost_node, origin_t)
                '''

                # Add all-to-all edges will create circle and prevent
                # the use of topological sort
                for idx in range(len(head) - 1):
                    h, t = head[idx], head[idx+1]
                    add_networkx_edge(G, h, t)
                    add_networkx_edge(G, t, h)
            else:
                for h in head:
                    for t in tail:
                        add_networkx_edge(G, h, t)
        elif type(head) == list:
            for h in head:
                add_networkx_edge(G, h, tail)
        elif type(tail) == list:
            for t in tail:
                add_networkx_edge(G, head, t)

    # Transitive clousre is too slow for large graphs
    #print("Generate transitive closure")
    #tc = nx.transitive_closure(G)
    print("Build graph finished")
    return G

def plot_graph(G):
    nx.draw_networkx(G)
    import matplotlib.pyplot as plt
    plt.show()

def run_vector_clock(G):
    print("Run vector clock algorithm...")
    for node_key in nx.topological_sort(G):
        vc = G.nodes[node_key]['vc']
        for eachpred in G.predecessors(node_key):
            pred_vc = G.nodes[eachpred]['vc']
            vc = list(map(max, zip(vc, pred_vc)))

        G.nodes[node_key]['vc'] = vc
    print("Vector clock algorith finished")


def has_path(G, src, dst):
    return nx.has_path(G, src, dst)

