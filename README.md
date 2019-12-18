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

To run the algorithm, run `./hcut <input file> <k> <algorithm>`. For example, if you want to run the approximate
algorithm from [CX'18], you would run `./hcut <input file> 2 CX`.

To generate sparse certificates, run `./hsparsify <input file> <k>`, to generate a sparse certificate of the hypergraph in the input file
with cuts preserved up to `k`.

The list of algorithms is available below. Note that some algorithms only work for k = 2 and some will prompt for extra
parameters (such as an approximation factor).

## Testing

To run the tests, run `make test`. The tests will run several cut algorithms on small
problems and verify that everything is working correctly.

## Problem Families

The following problem families can be generated with `./hgen`:

### Type 1

This generator produces a hypergraph with hyperedges and edge weights chosen
in a uniform way. It takes the following parameters:
- `n`: the number of vertices in the hypergraph
- `m`: the number of hyperedges in the hypergraph
- `p`: the sampling probability

This generator chooses a hyperedge by sampling each vertex with probability `p`.
It chooses `m` hyperedges this way. Hyperedge weights are sampled uniformly in
[1, 100].

For example, with `n = 100`, `m = 200`, and `p = 0.5` will create a hypergraph with 100 vertices and 200 hyperedges,
where each hyperedge has a 50% chance of being in each edge.

### Type 2

This generator attempts to produce strongly-connected clusters with interesting
cuts. It takes the following parameters:
- `n`: number of vertices
- `m`: number of hyperedges
- `p`: sampling probability
- `k`: number of clusters
- `P`: weight multiplication factor.

In this generator the vertices are divided into `k` uniform clusters.

The process for generation is similar to Type 1, except that there is an additional
check for whether or not a hyperedge is completely contained within some cluster.
If it is, then its edge weight is multiplied by `P`.

For example, for `n = 99`, `m = 200`, `p = 0.3`, `k = 3`, and `P = 100` will generate a hypergraph with 100 vertices,
200 edges, where the vertices are partitioned into three clusters of 33 vertices each, each vertex has a 30% chance of
being in any hyperedge, and the weight of an edge that is entirely within a cluster is multiplied by 100.

### Type 3

This generator produces constant rank hypergraphs. It takes the following parameters:
- `n`: number of vertices
- `m`: number of hyperedges
- `r`: rank

A hyperedge is generated by sampling `r` vertices without replacement. This process
is repeated for `m` hyperedges. Note that hyperedges may contain self-loops.

For `n = 100`, `m = 200`, and `r = 3`, this will generate a hypergraph of rank 3 with 100 vertices and 200 edges.

### Type 4

Creates a hypergraph with uniform clusters such that no hyperedges span the clusters.
It takes the following parameters:
- `n`: number of vertices
- `d`: number of hyperedges per cluster
- `k`: number of clusters
- `p`: sampling probability
- `P`: edge weights in range [1, 100P]

Generation is similar to type 1, except that for each hyperedge vertices are only
sampled from one cluster of `n/k` vertices.

For `n = 100`, `d = 200`, `k = 3`, `p = 0.5`, `P = 10` will create a hypergraph with 100 vertices, 600 edges, and 3 clusters,
with 200 edges in each cluster. The chance that any vertex appears within an edge in the same cluster is 50%, and edge
weights are in range [1, 1000].

### Type 5

This generator groups every vertex into components, and samples some hyperedges from within each component (so each
hyperedge is contained completely by a component) and sample some hyperedges from all vertices. The generator assigns
higher edge weights to hyperedges that are completely contained in components compared to hyperedges that span components.
This encourages non-trivial cuts.

It takes the following parameters:
- `n` : number of vertices
- `m1`: number of hyperedges that lie entirely within each cluster
- `p1`: sampling probability for m1 edges
- `m2`: number of hyperedges that are sampled from all vertices
- `p2`: sampling probability for m2 edges
- `k` : number of clusters
- `P` : weight multiplier for edges that are entirely contained within a single component

As of now this is the most straight-forward way to generate interesting hypergraphs with non-skewed cuts. Some care in
the choice of parameters must be taken though, specifically the expected number of hyperedges that a vertex is in should
be much greater than the weight of an edge crossing a component.
That is, `E[# hyperedges that contain v] * P >> weight of hyperedge that crosses between components`.

For `n = 100`, `m1 = 20`, `p1 = 0.3`, `m2 = 10`, `p2 = 0.4`, `k=3`, `P = 10` will generate a hypergraph with 100 vertices,
20 edges lying entirely in each cluster that samples within the cluster with 0.3 weight, 10 edges that are sampled from
all vertices with 0.4 weight, three clusters, and all edges that are entirely within a cluster have weight multiplied by 10.

## Algorithms

KW: The hypergraph min-cut algorithm described in [KW'96]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/order.hpp`.

Q: The hypergraph min-cut algorithm described in [Q'98]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/order.hpp`.

MW: The hypergraph min-cut algorithm described in [MW'00]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/order.hpp`.

CXY: The hypergraph k-cut algorithm described in [CXY'18]. Based on random contractions. Implemented in `hypergraph/include/hypergraph/cxy.hpp`.

FPZ: The hypergraph k-cut algorithm described in [FPZ'19]. Based on random contractions. Implemented in `hypergraph/include/hypergraph/fpz.hpp`.

CX: The (2+epsilon) approximate hypergraph min-cut algorithm described in [CX'18]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/approx.hpp`.

KK: The (1+epsilon) approximate min-cut algorithm for uniform (constant rank) hypergraphs described in [KK'14]. Based on random contractions. Implemented in `hypergraph/include/hypergraph/kk.hpp`.

Hypergraph min-cut on unweighted hypergraphs may use the sparse certificate detailed in [CX'18]. It is implemented in `hypergraph/include/hypergraph/certificate.hpp`.

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
