import graphviz

def rank_of_node(node_key):
    if(node_key.startswith("-1")):
        return -1

    return int(node_key.split("-")[0])

def plot_graph2(G, mpi_size):

    edges_of_rank = {}
    other_edges = []

    for rank in [-1]+range(mpi_size):
        edges_of_rank[rank] = []

    for (h, t) in G.edges():
        h_rank = rank_of_node(h)
        t_rank = rank_of_node(t)
        if h_rank == t_rank:
            edges_of_rank[h_rank].append((h,t))
        else:
            other_edges.append((h, t))

    vizG = graphviz.Digraph('G', filename='test.gv', format="pdf")
    for rank in [-1]+range(mpi_size):
        with vizG.subgraph(name='cluster_'+str(rank)) as sg:
            if rank %2 == 0:
                sg.attr(style='filled', color='lightgrey')
                sg.node_attr.update(style='filled', color='white')
            else:
                #sg.attr(color='blue')
                sg.node_attr['style'] = 'filled'

            sg.edges(edges_of_rank[rank])
            sg.attr(label='Proc '+str(rank))

    for (h, t) in other_edges:
        h_rank = rank_of_node(h)
        t_rank = rank_of_node(t)
        constraint = "true"
        if h_rank == -1 or t_rank == -1:
            constraint = "false"
        vizG.edge(h, t, constraint=constraint)

    vizG.view()

def plot_graph(G, mpi_size):

    nodes_of_rank = {}
    for rank in [-1]+range(mpi_size):
        nodes_of_rank[rank] = []

    for n in G.nodes():
        rank = rank_of_node(n)
        nodes_of_rank[rank].append(n)

    vizG = graphviz.Digraph('G', filename='test.gv', format="pdf")
    for level in range(len(nodes_of_rank[0])):
        with vizG.subgraph() as sg:
            sg.attr(rank=str(level))
            for rank in range(mpi_size):
                if level < len(nodes_of_rank[rank]):
                    sg.node(nodes_of_rank[rank][level])

    for (h, t) in G.edges():
        h_rank = rank_of_node(h)
        t_rank = rank_of_node(t)
        #if h_rank != -1 and t_rank != -1:
        vizG.edge(h, t)

    vizG.view()
