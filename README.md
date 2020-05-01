# Hypergraph k-cut algorithms

This repository contains implementations of hypergraph min-cut and hypergraph k-cut algorithms as well as tools to
assess their performance.

## Building

You will need cmake, boost, and a c++17 compiler.

If you don't have boost, you can install it by running `./install-deps.sh`.

```
mkdir build
cd build
cmake ..
make
```

## Installing

You can install all the binaries with `make install`.

## Testing

To run the tests, run `make test`. The tests will run several cut algorithms on small problems and verify that everything is working correctly.

## Usage

This repository is a collection of various tools for computing cuts in hypergraphs:

- `hcut`: A CLI for computing hypergraph cuts. See the [`hcut` README](app/hcut/README.md) for more details.
- `hexperiment`: Collects benchmark data on cut algorithms. See the [`hexperiment` README](app/hexperiment/README.md) for more details.
- `hgen`: A tool for generating hypergraph problem instances. See the [`hgen` README](app/hgen/README.md) for more details.
- `hsparsify`: A tool for generating sparse trimmed certificates (from CX '18).

## Usage

To generate hypergraphs, run `./hgen <num_vertices> <num_edges> <sampling percentage>`. Scroll down to see available generators. 

To run the algorithm, run `./hcut <input file> <k> <algorithm>`. For example, if you want to run the algorithm from [CX'18], you would run `./hcut <input file> 2 CX`. Scroll down to the bottom for a list of algorithms. 

To generate sparse certificates (from CX '18), run `./hsparsify <input file> <c>`, to generate a sparse certificate of the hypergraph in the input file with cut values preserved up to `c`. The output hypergraph will be written to a file with the same name as the input file except with `sparse_` prefixed to it, i.e. `sparse_<input file>`.

The list of algorithms is available below. Note that some algorithms only work for k = 2 and some will prompt for extra
parameters (such as an approximation factor).

## Hypergraph Generators

When benchmarking algorithms it is desirable to have access to hypergraphs with interesting (not-skewed, not-disconnected)
cuts.
The following generators can generate such instances.
To generate these hypergraphs on the command-line, you can use `hgen`.

### Planted

This generator groups the vertices into clusters of near equal size.
It samples some hyperedges from within each cluster (intercluster) and samples some hyperedges from all vertices (intracluster).

It takes the following parameters:
- `n` : number of vertices
- `m1`: number of intercluster hyperedges that lie entirely within each cluster
- `p1`: sampling probability for m1 inter-cluster hyperedges
- `m2`: number of intracluster hyperedges that are sampled from all vertices
- `p2`: sampling probability for m2 hyperedges
- `k` : number of clusters

For example, using `n = 100`, `m1 = 20`, `p1 = 0.3`, `m2 = 10`, `p2 = 0.4`, and `k=3` will generate a hypergraph with 100 vertices,
three near equal sized clusters, 20 hyperedges lying entirely within each cluster that samples within the
cluster with probability 0.3, and 10 hyperedges that are sampled from all vertices with probability 0.4.

### Planted constant rank

This generator groups the vertices into clusters of near equal size.
It samples some hyperedges from within each cluster (intercluster) and samples some from all vertices (intracluster).
The rank of each sampled hyperedge will be constant.

It takes the following parameters:
- `n`: number of vertices
- `k`: number of clusters
- `m1`: number of intercluster hyperedges
- `m2`: number of intracluster hyperedges
- `r`: rank of each hyperedge

### Ring

This generator uniformly distributes vertices on a ring.
It samples hyperedges uniformly sampling a point on the ring and creating a hyperedge that consists of all vertices that lie within a clockwise sweep from that point. 

It takes the following paramters:
- `n`: number of vertices
- `m`: number of hyperedges
- `radius`: the angle to sweep when sampling hyperedges




### Type 6

This generator lays out the vertices in a ring, and creates `n - 1` hyperedges containing `r` adjacent vertices. This creates
hypergraphs with predictable cuts.

It takes the following parameters:
- `n` : number of vertices
- `r` : size of each hyperedge

For example, using `n = 100` and `r = 3` will create a hypergraph with 100 vertices and 99 edges, where each edge is of the form `{v_i, v_{i+1}, ..., v_{i+r-1}}` for vertices 1 through 99.

## Algorithms

KW: The hypergraph min-cut algorithm described in [KW'96]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/order.hpp`.

Q: The hypergraph min-cut algorithm described in [Q'98]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/order.hpp`.

MW: The hypergraph min-cut algorithm described in [MW'00]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/order.hpp`.

CXY: The hypergraph min k-cut algorithm described in [CXY'18]. Based on random contractions. Implemented in `hypergraph/include/hypergraph/cxy.hpp`.

FPZ: The hypergraph min k-cut algorithm described in [FPZ'19]. Based on random contractions. Implemented in `hypergraph/include/hypergraph/fpz.hpp`.

apxCX: The (2+epsilon)-approximate hypergraph min-cut algorithm described in [CX'18]. Based on vertex orderings. Implemented in `hypergraph/include/hypergraph/approx.hpp`.

apxCertCX: A hypergraph min-cut algorithm that combines two ideas in [CX'18]. First, the apxCX algorithm is used to get an approximation of the min cut value.
That value is used to obtain a trimmed certificate, on which we run MW on.

CX: The certificate hypergraph min-cut algorithm described in [CX'18]. Uses t-trimmed certificates and exponential search along with MW.
Implemented in `hypergraph/include/hypergraph/certificate.hpp`.

KK: The min-k-cut algorithm for constant rank hypergraphs described in [KK'14]. Based on random contractions. Implemented in `hypergraph/include/hypergraph/kk.hpp`.


## Input format

The input file containing the hypergraph instance should be in hMETIS hypergraph format.

For unweighted graphs the input should look like:
```
<number of hyperedges> <number of vertices>
<hyperedge>
<hyperedge>
...
```

With real numbers, this would look like:
```
3 6
0 1
1 2 3
2 3 4 5
2 4 5
4 5 3 0
3 2 1 5
```

For weighted graphs the input file should look like:
```
<number of hyperedges> <number of vertices> 1
<hyperedge weight> <hyperedge>
<hyperedge weight> <hyperedge>
<hyperedge weight> <hyperedge>
...
```

Here, the hyperedge weight can be a non-negative decimal number:
```
3 6
0.342 0 1
1.23 1 2 3
10.34 2 3 4 5
2351 2 4 5
5.172 4 5 3 0
3 3 2 1 5
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
