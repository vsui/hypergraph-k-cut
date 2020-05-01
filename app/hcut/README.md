# hcut

A CLI for running hypergraph cut algorithms on hypergraphs.

## Usage

```
hcut <filename> <k> <algorithm>
```

where `<filename>` is an hMETIS file, `<algorithm>` is one of the algorithms below, and `k` specifies to find the min-k-cut.

## Algorithms

The algorithms can be classified into the following categories.
Each category supports additional flag parameters.

### Contraction algorithms

The random contraction algorithms are CXY, FPZ, and KK.
They support the following additional flags.

- `-s, --seed`: A random seed.
- `-v, --verbosity`: Controls how much logging is done. Higher values mean more logs.
- `-d, --discover`: The discovery value. The contraction algorithm will terminate once a cut with the discovery value is
found. The default is 0.
- `-r, --runs`: The number of runs. The contraction algorithm will repeat its contraction method at most this number of times.
The default is specific to the algorithm, but is the value specified by the paper to give the algorithm high chance of success.

### Ordering based min-cut

The ordering based min cut algorithms are MW, Q, KW, and CX.
They support no additional flags.
They also only support `k=2`.

### Min-cuts with approximation factor

These are min-cut algorithms that take in an epsilon argument.
The epsilon argument can be specified as a flag `-a, --epsilon`.
The first such algorithm is apxCX, which is a (2+epsilon)-approximate hypergraph min-cut algorithm.
The second such algorithm is apxCertCX, which is an exact algorithm that uses the approximate algorithm to obtain an 
estimation of the min cut value to sparsify the hypergraph before running MW on it to get the cut.

The only supported additional flag is required

- `-a, --epsilon`: The approximation factor

