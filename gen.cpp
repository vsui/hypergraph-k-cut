#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <hypergraph/hypergraph.hpp>

/* Generate a type 1 random hypergraph.
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
    vertex_to_cluster[v] = cluster_distribution(e2);
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
      if (edge_spans_clusters) {
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

/* Generate a type 3 random hypergraph
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

/* Agressively plant cut
 *
 * Args:
 *   n: number of vertices
 *   d: number of edges per cluster
 *   k: number of clusters
 */
template<typename HypergraphType>
HypergraphType generate_type_4(size_t n, size_t d, size_t k, double p) {
  std::random_device rd;
  std::mt19937 e2(rd());
  std::uniform_real_distribution<> distribution(0, 1);
  std::vector<std::vector<int>> edges;

  size_t v_per_cluster = n / k + 1;
  // For each cluster, create d hyperedges from points sampled randomly within that cluster
  for (int ki = 0; ki < k; ++ki) {
    for (int e = 0; e < d; ++e) {
      std::vector<int> edge;
      for (int j = ki * v_per_cluster; j < n && j < (ki + 1) * v_per_cluster; ++j) {
        if (distribution(e2) < p) {
          edge.push_back(j);
        }
      }
      edges.emplace_back(std::move(edge));
    }
  }

  std::vector<int> vertices(n);
  std::iota(std::begin(vertices), std::end(vertices), 0);

  auto h = Hypergraph{vertices, edges};
  return HypergraphType(h);
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
  std::cout << "Please input instance type (1, 2, 3, or 4)" << std::endl;
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
    size_t n, d, k;
    double p;
    std::cout << "Input n, d, k, p" << std::endl;
    std::cin >> n >> d >> k >> p;
    h = generate_type_4<HypergraphType>(n, d, k, p);
    filename_stream << n << "_" << d << "_" << k << "_" << p;
    break;
  }
  default: {
    std::cout << "Error: unknown type";
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
