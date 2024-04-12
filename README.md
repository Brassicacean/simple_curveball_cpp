This is a simple C++ implementation of [curveball](https://doi.org/10.1038/ncomms5114), an algorithm which generates random bipartite graphs with the same degree sequence as an input graph by exchanging multiple edges (connections) at a time between pairs of vertices in "trades".
This implementation is as bare-bones as I could make it, since I wanted to get a good idea of performance per unit of CPU time required.

Building the binary is simple: Just use `gpp -o curveball main.cpp`. 

The program takes three input command line arguments:
1. An input edgelist file (should represent a bipartite graph)
2. An integre representing the number of trades to attempt.
3. A existing directory to write new edgelist files into.
