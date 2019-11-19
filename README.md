# Hypergraph partitioning algorithms

## Building

You will need cmake, boost, and a c++17 compiler.

```
mkdir build
cd build
cmake ..
make
```

## Usage

To generate hypergraphs, run `./hgen <num_vertices> <num_edges> <sampling percentage>`

To run the algorithm, run `./hcut <input file> <k> <algorithm>`

To run the tests, run `./hypergraph/test/hypergraph_test`

## Algorithms

KW: The hypergraph min-cut algorithm described in [KW'96]. Based on vertex orderings.

Q: The hypergraph min-cut algorithm described in [Q'98]. Based on vertex orderings.

MW: The hypergraph min-cut algorithm described in [MW'00]. Based on vertex orderings.

CXY: The hypergraph k-cut algorithm described in [CXY'18]. Based on random contractions.

FPZ: The hypergraph k-cut algorithm described in [FPZ'19]. Based on random contractions.

CX: The (2+epsilon) approximate hypergraph min-cut algorithm described in [CX'18]. Based on vertex orderings.

KK: The (1+epsilon) approximate min-cut algorithm for uniform (constant rank) hypergraphs described in [KK'14]. Based on random contractions.

## Input

The input file should be in the hMETIS hypergraph format.

For unweighted graphs the input should look like:
```
<number of hyperedges> <number of vertices>
<edge>
<edge>
...
```

With real numbers, this would look like:
```
3 6
0 1
1 2 3
2 3 4 5 6
```

For weighted graphs the input file should look like:
```
<number of hyperedges> <number of vertices> 1
<edge weight> <edge>
<edge weight> <edge>
<edge weight> <edge>
...
```

Here, the edge weight can be a non-negative decimal number:
```
3 6
0.342 0 1
1.23 1 2 3
10.34 2 3 4 5 6
```

## References

[KW'96] Klimmek, R. and Wagner, F., 1996. A Simple Hypergraph Min Cut Algorithm

[Q'98] Queyranne, M., 1998. Minimizing symmetric submodular functions

[MW'00] Mak, W. and Wong, D., 2000. A fast hypergraph min-cut algorithm for circuit partitioning

[M'05] Mak, W., 2005, Faster Min-Cut Computation in Unweighted Hypergraphs/Circuit Netlists

[KK'14] Kogan, D. and Krauthgamer, R., 2014. Sketching Cuts in Graphs and Hypergraphs

[CXY'18] Chandrasekaran, K., Xu, C., and Yu, X., 2018, Hypergraph k-Cut in Randomized Polynomial Time

[CX'18] Chekuri, C. and Xu, C., 2018. Minimum Cuts and Sparsification in Hypergraphs.

[FPZ'19] Fox, K., Panigrahi, D., and Zhang, F., 2019, Minimum Cut and Minimum k-Cut in Hypergraphs via Branching Contractions
