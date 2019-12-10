#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <hypergraph/hypergraph.hpp>

// Helper for logic with cluster operations
class Cluster {
public:
  Cluster(size_t n, size_t k, size_t ki) : n_(n), k_(k), ki_(ki) {}

  struct ClusterIt {
    ClusterIt(size_t v) : v(v) {}
    ClusterIt &operator++() {
      v++;
      return *this;
    }
    size_t operator*() { return v; }
    bool operator!=(const ClusterIt &other) { return other.v != v; }

    size_t v;
  };

  ClusterIt begin() { return ClusterIt(vertices_before()); }
  ClusterIt end() { return ClusterIt(vertices_before() + size()); }
  Cluster operator*() { return *this; }
  Cluster &operator++() {
    ki_++;
    return *this;
  }
  bool operator!=(const Cluster &other) {
    return other.n_ != n_ || other.k_ != k_ || other.ki_ != ki_;
  }

private:
  size_t vertices_before() { return size_() * ki_; }
  size_t size_() { return n_ / k_; }
  size_t size() { return size_() + (ki_ == k_ - 1 ? (n_ % k_) : 0); }

  size_t n_;
  size_t k_;
  size_t ki_;
};

class Clusters {
public:
  Clusters(size_t n, size_t k) : n_(n), k_(k) {}
  Cluster begin() { return {n_, k_, 0}; }
  Cluster end() { return {n_, k_, k_}; }
private:
  size_t n_;
  size_t k_;
};

/* Generate a type 1 random hypergraph.
 *
 * Samples m random hyperedges. For each random hyperedge, samples every
 * vertex with probability p.
 *
 * Args:
 *   n: number of vertices
 *   m: number of hyperedges
 *   p: sampling probability
 */
template<typename HypergraphType>
HypergraphType generate_type_1(size_t n, size_t m, double p) {
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> distribution(0, 1);

  std::vector<std::vector<int>> edges;
  for (int i = 0; i < m; ++i) {
    std::vector<int> edge;
    for (int v = 0; v < n; ++v) {
      if (distribution(e2) < p) {
        edge.push_back(v);
      }
    }
    edges.emplace_back(std::move(edge));
  }

  std::vector<int> vertices(n);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  if constexpr (!is_weighted<HypergraphType>) {
    return Hypergraph{vertices, edges};
  } else {
    std::vector<std::pair<std::vector<int>, typename HypergraphType::EdgeWeight>> weighted_edges;
    for (std::vector<int> &edge : edges) {
      typename HypergraphType::EdgeWeight sampled_weight = distribution(e2) * 100 + 1;
      weighted_edges.emplace_back(std::move(edge), sampled_weight);
    }
    return HypergraphType{vertices, weighted_edges};
  }
}

/* Generate a type 2 random hypergraph.
 *
 * Similar to type 1 except that in addition to sampling random hyperedges,
 * it creates k uniform clusters of points. When a hyperedge is sampled, if
 * it is completely contained within a cluster, its weight is multiplied by
 * P.
 *
 * Args:
 *   n: number of vertices
 *   m: number of hyperedges
 *   p: sampling probability
 *   k: number of clusters
 *   P: weight multiplication factor
 */
template<typename HypergraphType>
HypergraphType generate_type_2(size_t n, size_t m, double p, size_t k, size_t P) {
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> distribution(0, 1);

  std::vector<std::vector<int>> edges;
  for (int i = 0; i < m; ++i) {
    std::vector<int> edge;
    for (int v = 0; v < n; ++v) {
      if (distribution(e2) < p) {
        edge.push_back(v);
      }
    }
    edges.emplace_back(std::move(edge));
  }

  std::vector<int> vertices(n);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  // Cluster vertices
  std::uniform_int_distribution<int> cluster_distribution(0, k - 1);
  std::map<int, int> vertex_to_cluster;
  for (int v = 0; v < n; ++v) {
    vertex_to_cluster[v] = v % k;
  }

  if constexpr (!is_weighted<HypergraphType>) {
    std::vector<std::vector<int>> repeated_edges;
    for (auto &edge : edges) {
      bool edge_spans_clusters = std::any_of(std::begin(edge), std::end(edge), [&vertex_to_cluster, &edge](auto v) {
        return vertex_to_cluster.at(v) != vertex_to_cluster.at(edge.front());
      });
      if (edge_spans_clusters) {
        for (int i = 0; i < P - 1; ++i) {
          repeated_edges.emplace_back(edge);
        }
      }
      repeated_edges.emplace_back(edge);
    }
    return {vertices, repeated_edges};
  } else {
    // Get weights
    std::vector<std::pair<std::vector<int>, double>> weighted_edges;
    for (auto &edge: edges) {
      bool edge_spans_clusters = std::any_of(std::begin(edge), std::end(edge), [&vertex_to_cluster, &edge](auto v) {
        return vertex_to_cluster.at(v) != vertex_to_cluster.at(edge.front());
      });
      double sampled_weight = (distribution(e2) * 100 + 1);
      if (!edge_spans_clusters) {
        sampled_weight *=
            P;
      }
      weighted_edges.
          emplace_back(edge, sampled_weight
      );
    }
    return {vertices, weighted_edges};
  }
}

/* Generate a type 3 random hypergraph.
 *
 * Generate m random hyperedges by sampling r vertices without replacement
 * from each hyperedge. This creates constant rank hypergraphs. Note that
 * hyperedges may contain self-loops.
 *
 * Args:
 *   n: Number of vertices
 *   m: Number of hyperedges
 *   r: Rank
 */
template<typename HypergraphType>
HypergraphType generate_type_3(size_t n, size_t m, size_t r) {
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> distribution(0, 1);

  std::vector<int> vertices_bucket;
  for (int v = 0; v < n; ++v) {
    for (int i = 0; i < m * r; ++i) {
      vertices_bucket.emplace_back(v);
    }
  }

  std::vector<std::vector<int>> edges;
  for (int i = 0; i < m; ++i) {
    // Sample an edge
    std::vector<int> edge;
    for (int j = 0; j < r; ++j) {
      std::uniform_int_distribution<int> bucket(0, vertices_bucket.size() - 1);
      size_t sampled = bucket(e2);
      edge.emplace_back(vertices_bucket.at(sampled));
      vertices_bucket.erase(std::begin(vertices_bucket) + sampled);
    }
    edges.emplace_back(std::move(edge));
  }

  std::vector<int> vertices(n);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  if constexpr (!is_weighted<HypergraphType>) {
    return Hypergraph{vertices, edges};
  } else {
    std::vector<std::pair<std::vector<int>, typename HypergraphType::EdgeWeight>> weighted_edges;
    for (std::vector<int> &edge : edges) {
      typename HypergraphType::EdgeWeight sampled_weight = distribution(e2) * 100 + 1;
      weighted_edges.emplace_back(std::move(edge), sampled_weight);
    }
    return {vertices, weighted_edges};
  }
}

/* Generate a type 4 random hypergraph
 *
 * Generate d*k hyperedges by partitioning the vertices into k uniform
 * clusters and sample d hyperedges from the vertices in each cluster.
 * Each hyperedge is sampled by randomly sampling each vertex with uniform
 * probability p.
 *
 * This aggressively plants a cut by ensuring no hyperedges cross clusters.
 *
 * Args:
 *   n: number of vertices
 *   d: number of edges per cluster
 *   k: number of clusters
 *   p: sampling probability
 *   P: edge weight ranges from [1, 100P]
 */
template<typename HypergraphType>
HypergraphType generate_type_4(size_t n, size_t d, size_t k, double p, size_t P) {
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> distribution(0, 1);
  std::vector<std::vector<int>> edges;

  Clusters clusters(n, k);
  // For each cluster, create d hyperedges from points sampled randomly within that cluster
  for (auto cluster : clusters) {
    for (int e = 0; e < d; ++e) {
      std::vector<int> edge;
      for (size_t v : cluster) {
        if (distribution(e2) < p) {
          edge.push_back(v);
        }
      }
      edges.emplace_back(std::move(edge));
    }
  }

  std::vector<int> vertices(n);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  auto h = HypergraphType(Hypergraph{vertices, edges});

  if constexpr (is_weighted<HypergraphType>) {
    const auto sample_edge_weight = [P, &e2, &distribution]() {
      return distribution(e2) * 100 * (P - 1) + 1;
    };
    h.resample_edge_weights(sample_edge_weight);
  }

  return h;
}

/* Generate a type 5 random hypergraph
 *
 * Generate d*k hyperedges by partitioning the vertices into k uniform
 * clusters and sample d hyperedges from the vertices in each cluster.
 * Each hyperedge is sampled by randomly sampling each vertex with uniform
 * probability p.
 *
 * This aggressively plants a cut by ensuring no hyperedges cross clusters.
 *
 * Args:
 *   n:  number of vertices
 *   m1: number of hyperedges that lie entirely within each cluster
 *   p1: sampling probability for m1 edges
 *   m2: number of hyperedges that are sampled from all vertices
 *   p2: sampling probability for m2 edges
 *   k:  number of clusters
 *   P:  weight multiplier for edges that are entirely contained within a single components
 */
template<typename HypergraphType>
HypergraphType generate_type_5(size_t n, size_t m1, double p1, size_t m2, double p2, size_t k, size_t P) {
  // This is just a combo of a type 2 and type 4 hypergraph
  auto h1 = generate_type_2<HypergraphType>(n, m2, p2, k, P);
  auto h2 = generate_type_4<HypergraphType>(n, m1, k, p1, P);
  for (const auto &[edge_id, vertices] : h1.edges()) {
    if constexpr (is_weighted<HypergraphType>) {
      // TODO we should actually check if it spans and if it does not then multiply by P
      h2.add_hyperedge(std::begin(vertices), std::end(vertices), edge_weight(h1, edge_id));
    } else {
      h2.add_hyperedge(std::begin(vertices), std::end(vertices));
    }
  }
  return h2;
}

template<typename HypergraphType>
void prompt() {
  HypergraphType h;
  std::stringstream filename_stream;

  if constexpr(is_weighted<HypergraphType>) {
    filename_stream << "w_";
  } else {
    filename_stream << "u_";
  }

  int type;
  std::cout << "Please input instance type (1, 2, 3, 4, or 5)" << std::endl;
  std::cin >> type;
  filename_stream << type << "_";

  switch (type) {
  case 1: {
    size_t n;
    size_t m;
    double p;
    std::cout << "Input n, m, and p" << std::endl;
    std::cin >> n >> m >> p;
    h = generate_type_1<HypergraphType>(n, m, p);
    filename_stream << n << "_" << m << "_" << p;
    break;
  }
  case 2: {
    size_t n;
    size_t m;
    double p;
    size_t k;
    size_t P;
    std::cout << "Input n, m, p, k, P" << std::endl;
    std::cin >> n >> m >> p >> k >> P;
    h = generate_type_2<HypergraphType>(n, m, p, k, P);
    filename_stream << n << "_" << m << "_" << p << "_" << k << "_" << P;
    break;
  }
  case 3: {
    size_t n;
    size_t m;
    size_t r;
    std::cout << "Input n, m, r" << std::endl;
    std::cin >> n >> m >> r;
    h = generate_type_3<HypergraphType>(n, m, r);
    filename_stream << n << "_" << m << "_" << r;
    break;
  }
  case 4: {
    size_t n, d, k, P;
    double p;
    std::cout << "Input n, d, k, p, P" << std::endl;
    std::cin >> n >> d >> k >> p >> P;
    h = generate_type_4<HypergraphType>(n, d, k, p, P);
    filename_stream << n << "_" << d << "_" << k << "_" << p;
    break;
  }
  case 5: {
    size_t n, m1, m2, P, k;
    double p1, p2;
    std::cout << "Input n, m1, p1, m2, p2, k, P" << std::endl;
    std::cin >> n >> m1 >> p1 >> m2 >> p2 >> k >> P;
    h = generate_type_5<HypergraphType>(n, m1, p1, m2, p2, k, P);
    filename_stream << n << "_" << m1 << "_" << p1 << "_" << m2 << "_" << p2 << "_" << k << "_" << P;
    break;
  }
  default: {
    std::cout << "Error: unknown type" << std::endl;
    exit(1);
  }
  }

  filename_stream << ".hmetis";
  std::string filename = filename_stream.str();
  std::ofstream output;
  output.open(filename);
  output << h;
  output.close();

  std::cout << filename << std::endl;
}

int main(int argc, char *argv[]) {
  int input;
  std::cout << "Enter \"1\" for an unweighted graph, \"2\" for a weighted graph" << std::endl;
  std::cin >> input;
  if (input != 1 && input != 2) {
    std::cerr << "Bad input" << std::endl;
    return 1;
  }

  if (input == 1) {
    prompt<Hypergraph>();
  } else {
    prompt<WeightedHypergraph<double>>();
  }
}
