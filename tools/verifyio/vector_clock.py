import networkx as nx
import numpy as np

G = nx.DiGraph()

#P1
G.add_node('A',local=[1,0,0])
G.add_node('B',local=[2,0,0])
G.add_node('C',local=[3,0,0])
G.add_node('D',local=[4,0,0])
G.add_node('E',local=[5,0,0])
G.add_node('F',local=[6,0,0])
G.add_node('G',local=[7,0,0])
#P2
G.add_node('H',local=[0,1,0])
G.add_node('I',local=[0,2,0])
G.add_node('J',local=[0,3,0])
#P3
G.add_node('K',local=[0,0,1])
G.add_node('L',local=[0,0,2])
G.add_node('M',local=[0,0,3])

G.add_edge('A', 'B')
G.add_edge('B', 'C')
G.add_edge('C', 'D')
G.add_edge('D', 'E')
G.add_edge('E', 'F')
G.add_edge('F', 'G')
G.add_edge('H', 'I')
G.add_edge('I', 'J')
G.add_edge('K', 'L')
G.add_edge('L', 'M')
G.add_edge('B', 'I')
G.add_edge('D', 'M')
G.add_edge('F', 'J')
G.add_edge('H', 'C')
G.add_edge('L', 'E')

'''
#P1
G.add_node('A',local=[1,0,0])
G.add_node('B',local=[2,0,0])
G.add_node('C',local=[3,0,0])
G.add_node('D',local=[4,0,0])
G.add_node('Z',local=[5,0,0])
#P2
G.add_node('E',local=[0,1,0])
G.add_node('F',local=[0,2,0])
G.add_node('G',local=[0,3,0])
#P3
G.add_node('H',local=[0,0,1])
G.add_node('I',local=[0,0,2])
G.add_node('J',local=[0,0,3])

G.add_edge('A', 'B')
G.add_edge('B', 'C')
G.add_edge('C', 'D')
G.add_edge('D', 'Z')
G.add_edge('E', 'F')
G.add_edge('F', 'G')
G.add_edge('H', 'I')
G.add_edge('I', 'J')
G.add_edge('B', 'F')
G.add_edge('G', 'D')
G.add_edge('H', 'E')
G.add_edge('Z', 'J')
'''



for e in nx.topological_sort(G):
    #print (e, '\t', G._node[e]['local'])
    l1 = G._node[e]['local']
    predecessors = G.predecessors(e)
    for eachpred in predecessors:
        l2 = G._node[eachpred]['local']
        l = list(map(max, zip(l1, l2))) 
        l1 = l
    
    G._node[e]['local'] = l1

for e in G.nodes():
    print (e, G._node[e]['local'])


def causal_order(x,y):
    a = np.array(G._node[x]['local'])
    b = np.array(G._node[y]['local'])
	
    c = a>=b
    d = b>=a

    print c
    print d
    if c.all():
		print y,' happened before ', x

    elif d.all():
        print x,' happened before ', y

    else:
		print 'Concurrent events'




#causal_order('G','K')

 
