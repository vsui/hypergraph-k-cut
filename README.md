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

## References

[CX'18] Chekuri, C. and Xu, C., 2018. Minimum Cuts and Sparsification in Hypergraphs.

[M'05] Mak, W., 2005, Faster Min-Cut Computation in Unweighted Hypergraphs/Circuit Netlists

[CXY'18] Chandrasekaran, K., Xu, C., and Yu, X., 2018, Hypergraph k-Cut in Randomized Polynomial Time

[FPZ'19] Fox, K., Panigrahi, D., and Zhang, F., 2019, Minimum Cut and Minimum k-Cut in Hypergraphs via Branching Contractions
