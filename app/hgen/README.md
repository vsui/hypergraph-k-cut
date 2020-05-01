# hgen

This is a CLI for generating hypergraph instances.

## Usage

```
hgen <args> <instance>
```

The generated hypergraph is written to stdout.

Here `<instance>` is either `planted`, `planted_constant_rank`, or `ring`, corresponding to the generators detailed below.
Each generator requires a different set of parameters. 
They can be specified using flags with the name of the parameter.
For example:

```
hgen ring -n 100 -m 200 --radius 10
```

will generate a ring hypergraph with `n=100`, `m=200`, `radius=10`.

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

### Planted uniform rank

This generator groups the vertices into clusters of near equal size.
It samples some hyperedges from within each cluster (intercluster) and samples some from all vertices (intracluster).
The rank of each sampled hyperedge will be constant.

It takes the following parameters:
- `n`: number of vertices
- `k`: number of clusters
- `m1`: number of intercluster hyperedges
- `m2`: number of intracluster hyperedges
- `r`: rank of each hyperedge

By setting `r` low you can generate low-rank instances.

### Ring

This generator uniformly distributes vertices on a ring.
It samples hyperedges uniformly sampling a point on the ring and creating a hyperedge that consists of all vertices that lie within a clockwise sweep from that point. 

It takes the following paramters:
- `n`: number of vertices
- `m`: number of hyperedges
- `radius`: the angle to sweep when sampling hyperedges


